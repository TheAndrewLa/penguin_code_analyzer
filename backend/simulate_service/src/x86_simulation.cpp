#include "x86_simulation.hpp"

#include <llvm/BinaryFormat/ELF.h>

#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
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
#include <llvm/MCA/HWEventListener.h>
#include <llvm/MCA/InstrBuilder.h>
#include <llvm/MCA/Instruction.h>
#include <llvm/MCA/Pipeline.h>
#include <llvm/MCA/SourceMgr.h>
#include <llvm/MCA/Support.h>

#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
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

class InstructionStreamer : public llvm::MCStreamer {
public:
  explicit InstructionStreamer(llvm::MCContext &ctx) : MCStreamer(ctx) {
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

// Records, for every instruction instance, the cycle in which it was
// dispatched, issued, executed and retired.
class TimelineRecorder : public llvm::mca::HWEventListener {
public:
  struct Stamps {
    int dispatch = -1;
    int ready = -1;
    int issued = -1;
    int executed = -1;
    int retired = -1;
  };

  void onEvent(const llvm::mca::HWInstructionEvent &event) override {
    const unsigned index = event.IR.getSourceIndex();
    auto &stamp = stamps_[index];

    switch (event.Type) {
    case llvm::mca::HWInstructionEvent::Dispatched:
      if (stamp.dispatch < 0) {
        stamp.dispatch = currentCycle_;
      }
      break;
    case llvm::mca::HWInstructionEvent::Ready:
      stamp.ready = currentCycle_;
      break;
    case llvm::mca::HWInstructionEvent::Issued:
      stamp.issued = currentCycle_;
      break;
    case llvm::mca::HWInstructionEvent::Executed:
      stamp.executed = currentCycle_;
      break;
    case llvm::mca::HWInstructionEvent::Retired:
      stamp.retired = currentCycle_;
      break;
    default:
      break;
    }
  }

  void onCycleEnd() override { ++currentCycle_; }

  const std::map<unsigned, Stamps> &stamps() const { return stamps_; }

private:
  unsigned currentCycle_ = 0;
  std::map<unsigned, Stamps> stamps_;
};
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

std::string X86Simulation::start(const std::string &cpu, const std::vector<std::string> &instructions) {
  const auto session = newSession();
  Simulation &data = results_[session];

  try {
    auto subtargetInfo =
        std::unique_ptr<llvm::MCSubtargetInfo>(target_->createMCSubtargetInfo(DefaultTargetTriple, cpu, ""));

    Assert(subtargetInfo, "Failed to create `MCSubtargetInfo`!");
    Assert(subtargetInfo->getSchedModel().hasInstrSchedModel(), "No scheduling information for CPU!");

    auto parsed = parse(*subtargetInfo, instructions);
    auto lowered = lower(*subtargetInfo, parsed);
    unsigned totalCycles = run(*subtargetInfo, lowered.instructions, data.timeline);
    data = collectResults(*subtargetInfo, parsed, lowered.instructions, totalCycles);
    data.success = true;
  } catch (const AnalyzeError &error) {
    data.success = false;
    data.error = error.what();
  } catch (...) {
    data.success = false;
    data.error = "Unknown simulation error";
  }

  return session;
}

void X86Simulation::end(const std::string &session) { results_.erase(session); }

X86Simulation::JsonResult X86Simulation::getGeneralResults(const std::string &session) {
  auto iter = results_.find(session);

  if (iter == results_.end()) {
    return MakeError("Can not find general info!");
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

X86Simulation::JsonResult X86Simulation::getInstructionInfo(const std::string &session) {
  auto iter = results_.find(session);

  if (iter == results_.end()) {
    return MakeError("Can not find instruction info!");
  }

  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  auto result = Json::Value(Json::arrayValue);

  for (const auto &info : iter->second.instructionInfo) {
    auto entry = Json::Value(Json::objectValue);
    entry["instrIndex"] = info.instrIndex;
    entry["uOps"] = info.uOps;
    entry["latency"] = info.latency;
    entry["rThroughput"] = info.rThroughput;
    entry["mayLoad"] = info.mayLoad;
    entry["mayStore"] = info.mayStore;
    entry["sideFx"] = info.sideFx;
    result.append(std::move(entry));
  }

  return result;
}

X86Simulation::JsonResult X86Simulation::getResourceUsage(const std::string &session) {
  auto iter = results_.find(session);

  if (iter == results_.end()) {
    return MakeError("Can not find resource usage!");
  }

  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  const auto &resourceUsage = iter->second.resourceUsage;

  auto result = Json::Value(Json::objectValue);
  auto resources = Json::Value(Json::arrayValue);
  std::for_each(resourceUsage.resourceNames.begin(), resourceUsage.resourceNames.end(),
                [&resources](const auto &name) { resources.append(name); });

  result["resources"] = std::move(resources);
  result["dispatchWidth"] = resourceUsage.dispatchWidth;

  auto usage = Json::Value(Json::arrayValue);
  for (const auto &entry : resourceUsage.usageEntries) {
    auto item = Json::Value(Json::objectValue);
    item["instrIndex"] = entry.instrIndex;

    auto cycles = Json::Value(Json::arrayValue);
    std::for_each(entry.resources.begin(), entry.resources.end(),
                  [&cycles](const auto &value) { cycles.append(value); });

    item["cycles"] = std::move(cycles);

    usage.append(std::move(item));
  }
  result["usage"] = std::move(usage);

  return result;
}

X86Simulation::JsonResult X86Simulation::getTimeline(const std::string &session) {
  auto iter = results_.find(session);

  if (iter == results_.end()) {
    return MakeError("Can not find timeline!");
  }

  if (!iter->second.success) {
    return MakeError(iter->second.error);
  }

  auto result = Json::Value(Json::arrayValue);

  for (const auto &entry : iter->second.timeline) {
    auto item = Json::Value(Json::objectValue);
    item["iteration"] = entry.iteration;
    item["instrIndex"] = entry.instrIndex;

    auto append = [&item](const char *key, const llvm::SmallVector<std::size_t> &values) {
      auto array = Json::Value(Json::arrayValue);
      for (auto value : values) {
        array.append(value);
      }
      item[key] = std::move(array);
    };

    append("dispatchCycles", entry.dispatchCycles);
    append("waitQueueCycles", entry.waitQueueCycles);
    append("executeCycles", entry.executeCycles);
    append("executeEndCycles", entry.executeEndCycles);
    append("waitRetireCycles", entry.waitRetireCycles);
    append("retireCycles", entry.retireCycles);

    result.append(std::move(item));
  }

  return result;
}

std::string X86Simulation::newSession() {
  const auto time = std::chrono::steady_clock::now().time_since_epoch();
  return std::to_string(time.count());
}

X86Simulation::Simulation X86Simulation::collectResults(const llvm::MCSubtargetInfo &subtargetInfo,
                                                        const ParsedInstructions &instructions,
                                                        const LoweredInstructions &lowered, unsigned totalCycles) {
  Assert(totalCycles > 0, "Simulation produced no cycles");

  const auto &schedModel = subtargetInfo.getSchedModel();
  const auto dispatchWidth = getDispatchWidth(schedModel);
  const auto resourceKinds = schedModel.getNumProcResourceKinds();

  llvm::SmallVector<std::uint64_t, 8> procResourceMasks;
  llvm::SmallVector<unsigned, 8> resIdx2ProcResID;

  procResourceMasks.resize(resourceKinds);
  resIdx2ProcResID.resize(resourceKinds, 0);

  llvm::mca::computeProcResourceMasks(schedModel, procResourceMasks);

  for (unsigned i = 1; i < resourceKinds; ++i) {
    resIdx2ProcResID[llvm::mca::getResourceStateIndex(procResourceMasks[i])] = i;
  }

  llvm::SmallVector<unsigned, 8> blockResourceUsage;
  blockResourceUsage.resize(resourceKinds, 0);

  unsigned blockMicroOps = 0;

  Simulation data;

  data.general.iterations = DefaultIterations;
  data.general.instructions = instructions.size();
  data.general.totalCycles = totalCycles;

  data.resourceUsage.dispatchWidth = dispatchWidth;
  data.resourceUsage.resourceNames.reserve(resourceKinds - 1);
  for (unsigned i = 1; i < resourceKinds; ++i) {
    data.resourceUsage.resourceNames.push_back(schedModel.getProcResource(i)->Name);
  }

  data.instructionInfo.reserve(instructions.size());
  data.resourceUsage.usageEntries.reserve(instructions.size());

  for (size_t i = 0; i < instructions.size(); ++i) {
    const auto &instr = lowered[i];
    const auto &desc = instr->getDesc();
    const auto *schedClassDesc = schedModel.getSchedClassDesc(desc.SchedClassID);

    Assert(schedClassDesc != nullptr, "Can not create sched class desc for instruction!");

    auto info = InstructionEntry{};
    info.instrIndex = i;
    info.uOps = desc.NumMicroOps;
    info.latency = llvm::MCSchedModel::computeInstrLatency(subtargetInfo, *schedClassDesc);
    info.rThroughput = llvm::MCSchedModel::getReciprocalThroughput(subtargetInfo, *schedClassDesc);
    info.mayLoad = instr->getMayLoad();
    info.mayStore = instr->getMayStore();
    info.sideFx = instr->getHasSideEffects();

    llvm::SmallVector<float> usage;
    usage.assign(resourceKinds, 0.0F);
    for (const auto &resource : desc.Resources) {
      unsigned idx = resIdx2ProcResID[llvm::mca::getResourceStateIndex(resource.first)];
      usage[idx] = resource.second.size();
      blockResourceUsage[idx] += resource.second.size();
    }
    blockMicroOps += desc.NumMicroOps;

    data.instructionInfo.push_back(info);
    data.resourceUsage.usageEntries.push_back(ResourceUsageEntry{i, std::move(usage)});
  }

  data.general.totalMicroOps = blockMicroOps * DefaultIterations;
  data.general.uOpsPerCycle = static_cast<double>(data.general.totalMicroOps) / static_cast<double>(totalCycles);
  data.general.instructionsPerCycle =
      static_cast<double>(data.general.instructions * DefaultIterations) / static_cast<double>(totalCycles);
  data.general.blockRThroughput =
      llvm::mca::computeBlockRThroughput(schedModel, dispatchWidth, blockMicroOps, blockResourceUsage);

  return data;
}

std::vector<llvm::MCInst> X86Simulation::parse(const llvm::MCSubtargetInfo &subtargetInfo,
                                               const std::vector<std::string> &instructions) const {
  llvm::SourceMgr sourceMgr;
  std::string program;

  std::for_each(instructions.cbegin(), instructions.cend(), [&program](const auto &instr) {
    program += instr;
    program += '\n';
  });

  sourceMgr.AddNewSourceBuffer(llvm::MemoryBuffer::getMemBuffer(program), llvm::SMLoc());

  auto ctx = llvm::MCContext(llvm::Triple(DefaultTargetTriple), asmInfo_.get(), regInfo_.get(), &subtargetInfo);
  auto objectFileInfo = std::unique_ptr<llvm::MCObjectFileInfo>(target_->createMCObjectFileInfo(ctx, false));
  ctx.setObjectFileInfo(objectFileInfo.get());

  auto streamer = InstructionStreamer(ctx);

  llvm::MCTargetOptions options;
  options.PreserveAsmComments = false;

  auto parser = std::unique_ptr<llvm::MCAsmParser>(llvm::createMCAsmParser(sourceMgr, ctx, streamer, *asmInfo_));

  Assert(parser, "Failed to create `MCAsmParser`!");

  auto targetParser = std::unique_ptr<llvm::MCTargetAsmParser>(
      target_->createMCAsmParser(subtargetInfo, *parser, *instrInfo_, options));

  Assert(targetParser, "Failed to create `MCTargetAsmParser`!");

  parser->setTargetParser(*targetParser);
  parser->Run(true);

  Assert(streamer.hasInstructions(), "Failed to parse instructions!");

  return streamer.instructions();
}

X86Simulation::LoweredProgram X86Simulation::lower(const llvm::MCSubtargetInfo &subtargetInfo,
                                                   const std::vector<llvm::MCInst> &instructions) const {
  auto instrAnalysis = std::unique_ptr<llvm::MCInstrAnalysis>(target_->createMCInstrAnalysis(instrInfo_.get()));
  auto instrumentMgr = std::make_unique<llvm::mca::InstrumentManager>(subtargetInfo, *instrInfo_);

  Assert(instrAnalysis, "Failed to create `MCInstrAnalysis`!");
  Assert(instrumentMgr, "Failed to create `InstrumentManager`!");

  auto instrBuilder = std::make_unique<llvm::mca::InstrBuilder>(
      subtargetInfo, *instrInfo_, *regInfo_, instrAnalysis.get(), *instrumentMgr, DefaultCallLatency);

  LoweredProgram program;
  program.builder = std::move(instrBuilder);
  program.instructions.reserve(instructions.size());

  llvm::SmallVector<llvm::mca::Instrument *> instruments;

  for (const auto &instr : instructions) {
    auto loweredInstr = program.builder->createInstruction(instr, instruments);
    Assert(loweredInstr, "Failed to lower instruction!");
    program.instructions.emplace_back(std::move(*loweredInstr));
  }

  return program;
}

unsigned X86Simulation::run(const llvm::MCSubtargetInfo &subtargetInfo, const LoweredInstructions &lowered,
                            std::vector<TimelineEntry> &timeline) const {
  const auto &schedModel = subtargetInfo.getSchedModel();
  const auto dispatchWidth = getDispatchWidth(schedModel);

  auto src = llvm::mca::CircularSourceMgr(lowered, DefaultIterations);
  auto options = llvm::mca::PipelineOptions(/*MicroOpQueueSize=*/0, /*DecodersThroughput=*/0,
                                            /*DispatchWidth=*/dispatchWidth, /*RegisterFileSize=*/0,
                                            /*LoadQueueSize=*/0, /*StoreQueueSize=*/0,
                                            /*AssumeNoAlias=*/true);

  auto ctx = llvm::mca::Context(*regInfo_, subtargetInfo);
  auto customBehaviour = std::make_unique<llvm::mca::CustomBehaviour>(subtargetInfo, src, *instrInfo_);

  Assert(customBehaviour, "Failed to create `CustomBehaviour`!");

  auto pipeline = ctx.createDefaultPipeline(options, src, *customBehaviour);

  TimelineRecorder recorder;
  pipeline->addEventListener(&recorder);

  auto cycles = pipeline->run();
  Assert(cycles, "Failed to run pipeline");

  const auto numInstructions = lowered.size();
  for (const auto &[sourceIndex, stamp] : recorder.stamps()) {
    TimelineEntry entry;
    entry.iteration = sourceIndex / numInstructions;
    entry.instrIndex = sourceIndex % numInstructions;

    entry.dispatchCycles.push_back(stamp.dispatch >= 0 ? static_cast<std::size_t>(stamp.dispatch) : 0);
    entry.waitQueueCycles.push_back(
        stamp.issued >= 0 && stamp.dispatch >= 0 ? static_cast<std::size_t>(stamp.issued - stamp.dispatch) : 0);
    entry.executeCycles.push_back(
        stamp.executed >= 0 && stamp.issued >= 0 ? static_cast<std::size_t>(stamp.executed - stamp.issued) : 0);
    entry.executeEndCycles.push_back(stamp.executed >= 0 ? static_cast<std::size_t>(stamp.executed) : 0);
    entry.waitRetireCycles.push_back(
        stamp.retired >= 0 && stamp.executed >= 0 ? static_cast<std::size_t>((stamp.retired - 1) - stamp.executed) : 0);
    entry.retireCycles.push_back(stamp.retired >= 0 ? static_cast<std::size_t>(stamp.retired) : 0);

    timeline.push_back(std::move(entry));
  }

  return *cycles;
}
