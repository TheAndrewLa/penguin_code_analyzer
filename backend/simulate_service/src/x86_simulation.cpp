#include "x86_simulation.hpp"

#include <llvm/BinaryFormat/ELF.h>

#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCParser/MCAsmParser.h>
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSchedule.h>
#include <llvm/MC/MCSectionELF.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/TargetRegistry.h>

#include <llvm/MCA/Context.h>
#include <llvm/MCA/CustomBehaviour.h>
#include <llvm/MCA/InstrBuilder.h>
#include <llvm/MCA/Instruction.h>
#include <llvm/MCA/Pipeline.h>
#include <llvm/MCA/SourceMgr.h>
#include <llvm/MCA/Support.h>

#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {
void InitializeLLVM() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86Disassembler();
  });
}

llvm::Error MakeError(const llvm::Twine &message) { return llvm::createStringError(message); }

class AnalyzeError : public std::exception {
public:
  explicit AnalyzeError(std::string what) : what_(std::move(what)) {}
  explicit AnalyzeError(const char *what) : what_(what) {}
  const char *what() const noexcept override { return what_.c_str(); }

private:
  std::string what_;
};

template <typename T> void Assert(T &&value, const std::string &message) {
  if (!(value)) {
    throw AnalyzeError{message};
  }
}

// Streamer that collects every emitted instruction instead of producing output.
class CollectStreamer : public llvm::MCStreamer {
public:
  explicit CollectStreamer(llvm::MCContext &ctx) : MCStreamer(ctx) {
    auto *text = ctx.getELFSection(".text", llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_EXECINSTR);
    switchSection(text);
  }

  void emitInstruction(const llvm::MCInst &instr, const llvm::MCSubtargetInfo &subtargetInfo) override {
    mcInsts_.push_back(instr);
  }

  bool hasInstructions() const { return !mcInsts_.empty(); }
  const std::vector<llvm::MCInst> &instructions() const { return mcInsts_; }

  bool emitSymbolAttribute(llvm::MCSymbol *symbol, llvm::MCSymbolAttr attribute) override { return false; }
  void emitZerofill(llvm::MCSection *section, llvm::MCSymbol *symbol, uint64_t size, llvm::Align alignment,
                    llvm::SMLoc loc = llvm::SMLoc()) override {}
  void emitCommonSymbol(llvm::MCSymbol *symbol, uint64_t size, llvm::Align alignment) override {}

private:
  std::vector<llvm::MCInst> mcInsts_;
};

unsigned getDispatchWidth(const llvm::MCSchedModel &model) {
  const auto width = model.IssueWidth;
  return (width == 0) ? 1 : width;
}
} // namespace

X86Simulation::X86Simulation() {
  InitializeLLVM();

  llvm::StringRef tripleName("x86_64-unknown-linux-gnu");
  llvm::Triple targetTripe(tripleName);

  std::string error;

  target_ = llvm::TargetRegistry::lookupTarget(tripleName, error);

  if (target_ == nullptr) {
    llvm::report_fatal_error("Can not find x86-64 target!");
  }

  regInfo_.reset(target_->createMCRegInfo(tripleName));
  Assert(regInfo_, "Can not create `MCRegisterInfo`!");

  llvm::MCTargetOptions MCOptions;

  asmInfo_.reset(target_->createMCAsmInfo(*regInfo_, tripleName, MCOptions));
  Assert(asmInfo_, "Can not create `MCAsmInfo`!");

  instrInfo_.reset(target_->createMCInstrInfo());
  Assert(instrInfo_, "Can not create `MCInstrInfo`!");
}

X86Simulation::~X86Simulation() = default;

std::string X86Simulation::apply(const std::string &cpu, const std::vector<std::string> &instructions) const {
  std::string token = makeToken();

  SimulationData &data = data_[token];

  try {
    auto sti = std::unique_ptr<llvm::MCSubtargetInfo>(target_->createMCSubtargetInfo(DefaultTargetTriple, cpu, ""));
    Assert(sti, "Failed to create `MCSubtargetInfo`!");
    Assert(sti->getSchedModel().hasInstrSchedModel(), "No scheduling information for CPU!");

    auto insts = parseInstructions(*sti, instructions);
    auto lowered = lowerInstructions(*sti, insts);
    unsigned totalCycles = runPipeline(*sti, lowered);
    data = collectResults(*sti, insts, lowered, totalCycles, instructions);
    data.success = true;
  } catch (const AnalyzeError &error) {
    data.success = false;
    data.error = error.what();
  } catch (...) {
    data.success = false;
    data.error = "Unknown simulation error";
  }

  return token;
}

std::vector<llvm::MCInst> X86Simulation::parseInstructions(const llvm::MCSubtargetInfo &subtargetInfo,
                                                           const std::vector<std::string> &instructions) const {
  llvm::SourceMgr sourceMgr;
  std::string program;

  std::for_each(instructions.cbegin(), instructions.cend(), [&program](const auto &instr) {
    program += instr;
    program += '\n';
  });

  sourceMgr.AddNewSourceBuffer(llvm::MemoryBuffer::getMemBuffer(program), llvm::SMLoc());

  auto ctx = llvm::MCContext(llvm::Triple(DefaultTargetTriple), asmInfo_.get(), regInfo_.get(), &subtargetInfo);

  auto streamer = CollectStreamer(ctx);
  auto parser = std::unique_ptr<llvm::MCAsmParser>(llvm::createMCAsmParser(sourceMgr, ctx, streamer, *asmInfo_));

  Assert(parser, "Failed to create `MCAsmParser`!");

  auto targetParser = std::unique_ptr<llvm::MCTargetAsmParser>(
      target_->createMCAsmParser(subtargetInfo, *parser, *instrInfo_, llvm::MCTargetOptions()));

  Assert(targetParser, "Failed to create `MCTargetAsmParser`!");

  parser->setTargetParser(*targetParser);
  parser->Run(true);

  Assert(streamer.hasInstructions(), "Failed to parse instructions!");

  return streamer.instructions();
}

std::vector<std::unique_ptr<llvm::mca::Instruction>>
X86Simulation::lowerInstructions(const llvm::MCSubtargetInfo &subtargetInfo,
                                 const std::vector<llvm::MCInst> &instructions) const {
  auto instrAnalysis = std::unique_ptr<llvm::MCInstrAnalysis>(target_->createMCInstrAnalysis(instrInfo_.get()));

  auto instrumentMgr = llvm::mca::InstrumentManager(subtargetInfo, *instrInfo_);
  auto instrBuilder = llvm::mca::InstrBuilder(subtargetInfo, *instrInfo_, *regInfo_, instrAnalysis.get(), instrumentMgr,
                                              DefaultCallLatency);

  std::vector<std::unique_ptr<llvm::mca::Instruction>> lowered;
  llvm::SmallVector<llvm::mca::Instrument *> instruments;

  lowered.reserve(instructions.size());

  for (const auto &instr : instructions) {
    auto loweredInstr = instrBuilder.createInstruction(instr, instruments);
    Assert(loweredInstr, "Failed to lower instruction!");
    lowered.emplace_back(std::move(*loweredInstr));
  }

  return lowered;
}

unsigned X86Simulation::runPipeline(const llvm::MCSubtargetInfo &subtargetInfo,
                                    const std::vector<std::unique_ptr<llvm::mca::Instruction>> &lowered) const {
  const auto &schedModel = subtargetInfo.getSchedModel();
  const auto dispatchWidth = getDispatchWidth(schedModel);

  llvm::mca::CircularSourceMgr src(lowered, DefaultIterations);

  llvm::mca::PipelineOptions options(/*MicroOpQueueSize=*/0, /*DecodersThroughput=*/0,
                                     /*DispatchWidth=*/dispatchWidth, /*RegisterFileSize=*/0,
                                     /*LoadQueueSize=*/0, /*StoreQueueSize=*/0,
                                     /*AssumeNoAlias=*/true);

  auto ctx = llvm::mca::Context(*regInfo_, subtargetInfo);
  auto customBehaviour = std::make_unique<llvm::mca::CustomBehaviour>(subtargetInfo, src, *instrInfo_);
  auto pipeline = ctx.createDefaultPipeline(options, src, *customBehaviour);
  auto cycles = pipeline->run();

  Assert(cycles, "Failed to run pipeline");

  return *cycles;
}

X86Simulation::SimulationData
X86Simulation::collectResults(const llvm::MCSubtargetInfo &subtargetInfo, const std::vector<llvm::MCInst> &instructions,
                              const std::vector<std::unique_ptr<llvm::mca::Instruction>> &lowered, unsigned totalCycles,
                              const std::vector<std::string> &instructionStrings) {
  const auto &schedModel = subtargetInfo.getSchedModel();
  const auto dispatchWidth = getDispatchWidth(schedModel);

  llvm::SmallVector<uint64_t, 8> procResourceMasks;
  llvm::SmallVector<unsigned, 8> resIdx2ProcResID;

  procResourceMasks.resize(schedModel.getNumProcResourceKinds());
  resIdx2ProcResID.resize(schedModel.getNumProcResourceKinds(), 0);

  llvm::mca::computeProcResourceMasks(schedModel, procResourceMasks);

  auto resourceKinds = schedModel.getNumProcResourceKinds();

  for (unsigned i = 1; i < resourceKinds; ++i) {
    resIdx2ProcResID[llvm::mca::getResourceStateIndex(procResourceMasks[i])] = i;
  }

  llvm::SmallVector<unsigned, 8> blockResourceUsage;
  blockResourceUsage.resize(schedModel.getNumProcResourceKinds(), 0);

  auto blockMicroOps = unsigned(0);

  SimulationData data;

  data.general.iterations = DefaultIterations;
  data.general.instructions = instructions.size();
  data.general.totalCycles = totalCycles;

  for (size_t i = 0; i < instructions.size(); ++i) {
    const auto &instr = lowered[i];
    const auto &desc = instr->getDesc();
    const auto *schedClassDesc = schedModel.getSchedClassDesc(desc.SchedClassID);

    Assert(schedClassDesc != nullptr, "Can not create sched class desc for instruction!");

    auto info = InstructionInfo{};
    info.uOps = desc.NumMicroOps;
    info.latency = llvm::MCSchedModel::computeInstrLatency(subtargetInfo, *schedClassDesc);
    info.rThroughput = llvm::MCSchedModel::getReciprocalThroughput(subtargetInfo, *schedClassDesc);
    info.mayLoad = instr->getMayLoad();
    info.mayStore = instr->getMayStore();
    info.sideFx = instr->getHasSideEffects();

    ResourceUsage usage;
    usage.assign(schedModel.getNumProcResourceKinds(), 0.0F);
    for (const auto &resource : desc.Resources) {
      unsigned idx = resIdx2ProcResID[llvm::mca::getResourceStateIndex(resource.first)];
      usage[idx] = resource.second.size();
      blockResourceUsage[idx] += resource.second.size();
    }
    blockMicroOps += desc.NumMicroOps;

    const auto &key = (i < instructionStrings.size()) ? instructionStrings[i] : std::to_string(i);

    data.instructionInfo[key] = info;
    data.resourceUsage[key] = std::move(usage);
  }

  data.general.totalMicroOps = blockMicroOps * DefaultIterations;
  data.general.uOpsPerCycle =
      (totalCycles > 0) ? static_cast<float>(data.general.totalMicroOps) / static_cast<float>(totalCycles) : 0.0F;
  data.general.instructionsPerCycle =
      (totalCycles > 0)
          ? static_cast<float>(data.general.instructions * DefaultIterations) / static_cast<float>(totalCycles)
          : 0.0F;
  data.general.blockRThroughput =
      llvm::mca::computeBlockRThroughput(schedModel, dispatchWidth, blockMicroOps, blockResourceUsage);

  return data;
}

X86Simulation::JsonResult X86Simulation::getGeneralResults(const std::string &token) {
  auto iter = data_.find(token);
  if (iter == data_.end()) {
    return MakeError("Token not found");
  }
  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  const auto &info = iter->second.general;

  auto result = Json::Value(Json::objectValue);
  result["iterations"] = info.iterations;
  result["instructions"] = info.instructions;
  result["totalCycles"] = info.totalCycles;
  result["totalMicroOps"] = info.totalMicroOps;
  result["uOpsPerCycle"] = info.uOpsPerCycle;
  result["instructionsPerCycle"] = info.instructionsPerCycle;
  result["blockRThroughput"] = info.blockRThroughput;
  return result;
}

X86Simulation::JsonResult X86Simulation::getInstructionInfo(const std::string &token) {
  auto iter = data_.find(token);
  if (iter == data_.end()) {
    return MakeError("Token not found");
  }
  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  auto result = Json::Value(Json::objectValue);

  for (const auto &[key, info] : iter->second.instructionInfo) {
    auto entry = Json::Value(Json::objectValue);
    entry["uOps"] = info.uOps;
    entry["latency"] = info.latency;
    entry["rThroughput"] = info.rThroughput;
    entry["mayLoad"] = info.mayLoad;
    entry["mayStore"] = info.mayStore;
    entry["sideFx"] = info.sideFx;
    result[key] = std::move(entry);
  }

  return result;
}

X86Simulation::JsonResult X86Simulation::getResourceUsage(const std::string &token) {
  auto iter = data_.find(token);
  if (iter == data_.end()) {
    return MakeError("Token not found");
  }
  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  auto result = Json::Value(Json::objectValue);

  for (const auto &[key, usage] : iter->second.resourceUsage) {
    auto array = Json::Value(Json::arrayValue);
    std::for_each(usage.begin(), usage.end(), [&array](const auto value) { array.append(value); });
    result[key] = array;
  }
  return result;
}

std::string X86Simulation::makeToken() {
  const auto time = std::chrono::steady_clock::now().time_since_epoch();
  return std::to_string(time.count());
}
