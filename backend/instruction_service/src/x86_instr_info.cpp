#include "x86_instr_info.hpp"

#include <llvm/BinaryFormat/ELF.h>

#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCExpr.h>
#include <llvm/MC/MCFixup.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSchedule.h>
#include <llvm/MC/MCSectionELF.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/TargetRegistry.h>

#include <llvm/MCA/Instruction.h>

#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

#include <json/config.h>
#include <json/json.h>
#include <json/value.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>

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

std::string DescribeOperand(const llvm::MCOperand &operand, const llvm::MCRegisterInfo &regInfo) {
  if (operand.isReg()) {
    return std::string("register: ") + regInfo.getName(operand.getReg());
  }
  if (operand.isImm()) {
    return "immediate: " + std::to_string(operand.getImm());
  }
  if (operand.isSFPImm()) {
    return "singlie float immediate: " + std::to_string(operand.getSFPImm());
  }
  if (operand.isDFPImm()) {
    return "double float immediate: " + std::to_string(operand.getDFPImm());
  }
  if (operand.isExpr()) {
    return "relocatible immediate";
  }
  return "unknown";
}

// Special streamer used to emit only one instruction
class OneInstructionStreamer : public llvm::MCStreamer {
public:
  OneInstructionStreamer(llvm::MCContext &ctx) : MCStreamer(ctx) {
    auto *text = ctx.getELFSection(".text", llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_EXECINSTR);
    switchSection(text);
  }

  void emitInstruction(const llvm::MCInst &instr, const llvm::MCSubtargetInfo &subtargetInfo) override {
    if (!contains_ && valid_) {
      // First emitted instruction is recorded and `streamer` is still valid
      instr_ = instr;
      contains_ = true;
    } else {
      // After trying to emit more than one instruction `stramer` goes invalid
      valid_ = false;
      instr_ = llvm::MCInst();
    }
  }

  bool hasInstruction() const { return contains_ && valid_; }
  const llvm::MCInst &getInstruction() const { return instr_; }

  bool emitSymbolAttribute(llvm::MCSymbol *symbol, llvm::MCSymbolAttr attribute) override { return false; }
  void emitZerofill(llvm::MCSection *section, llvm::MCSymbol *symbol, uint64_t size, llvm::Align alignment,
                    llvm::SMLoc loc = llvm::SMLoc()) override {}
  void emitCommonSymbol(llvm::MCSymbol *symbol, uint64_t size, llvm::Align alignment) override {}

private:
  llvm::MCInst instr_;

  bool contains_{false};
  bool valid_{true};
};

class AnalyzeError : public std::exception {
public:
  AnalyzeError(std::string what) : what_(std::move(what)) {}
  AnalyzeError(const char *what) : what_(what) {}

  const char *what() const noexcept override { return what_.c_str(); };

private:
  std::string what_;
};

template <typename T> void Assert(T &&value, const std::string &message) {
  if (!(value)) {
    throw AnalyzeError{message};
  }
}
} // namespace

X86InstrInfo::X86InstrInfo() {
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

X86InstrInfo::~X86InstrInfo() = default;

X86InstrInfo::JsonResult X86InstrInfo::brief(const std::string &cpu, const std::string &instruction) const {
  try {
    return analyze(cpu, instruction, false);
  } catch (const AnalyzeError &e) {
    return MakeError(e.what());
  } catch (...) {
    return MakeError("An error occured!");
  }
}

X86InstrInfo::JsonResult X86InstrInfo::full(const std::string &cpu, const std::string &instruction) const {
  try {
    return analyze(cpu, instruction, true);
  } catch (const AnalyzeError &e) {
    return MakeError(e.what());
  } catch (...) {
    return MakeError("An error occured!");
  }
}

X86InstrInfo::JsonResult X86InstrInfo::analyze(const std::string &cpu, const std::string &instrText, bool full) const {
  std::unique_ptr<llvm::MCSubtargetInfo> subtargetInfo(target_->createMCSubtargetInfo(DefaultTargetTriple, cpu, ""));
  Assert(subtargetInfo, "Can not create `MCSubtargetInfo`!");

  auto ctx = llvm::MCContext(llvm::Triple(DefaultTargetTriple), asmInfo_.get(), regInfo_.get(), subtargetInfo.get());

  std::unique_ptr<llvm::MemoryBuffer> buffer(llvm::MemoryBuffer::getMemBuffer(instrText, "input.s", false));

  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());

  auto streamer = OneInstructionStreamer(ctx);

  std::unique_ptr<llvm::MCAsmParser> parser(llvm::createMCAsmParser(sourceMgr, ctx, streamer, *asmInfo_));
  Assert(parser, "Can not create `MCAsmParser`!");

  std::unique_ptr<llvm::MCTargetAsmParser> targetParser(
      target_->createMCAsmParser(*subtargetInfo, *parser, *instrInfo_, llvm::MCTargetOptions()));
  Assert(targetParser, "Can not create `MCTargetAsmParser`!");

  parser->setTargetParser(*targetParser);
  parser->Run(true);

  Assert(streamer.hasInstruction(), "Can not parse instruction");

  const auto &schedModel = subtargetInfo->getSchedModel();

  const auto &instr = streamer.getInstruction();
  const auto &instrDesc = instrInfo_->get(instr.getOpcode());

  auto schedClass = instrDesc.getSchedClass();
  const auto cpuid = schedModel.getProcessorID();

  while ((schedClass != 0) && schedModel.getSchedClassDesc(schedClass)->isVariant()) {
    schedClass = subtargetInfo->resolveVariantSchedClass(schedClass, &instr, instrInfo_.get(), cpuid);
  }

  const auto *schedClassDesc = schedModel.getSchedClassDesc(schedClass);

  auto result = Json::Value(Json::objectValue);

  result["uops"] = Json::UInt(schedClassDesc->NumMicroOps);
  result["latency"] = Json::UInt(llvm::MCSchedModel::computeInstrLatency(*subtargetInfo, *schedClassDesc));
  result["rthroughput"] = Json::Value(llvm::MCSchedModel::getReciprocalThroughput(*subtargetInfo, *schedClassDesc));

  if (full) {
    std::unique_ptr<llvm::MCCodeEmitter> codeEmitter(target_->createMCCodeEmitter(*instrInfo_, ctx));
    Assert(codeEmitter, "Can not create `MCCodeEmitter`!");

    auto fixups = llvm::SmallVector<llvm::MCFixup, MaxFixups>();
    auto encoding = llvm::SmallVector<char, MaxEncodingSize>();

    codeEmitter->encodeInstruction(instr, encoding, fixups, *subtargetInfo);

    auto bytes = Json::Value(Json::arrayValue);
    std::for_each(encoding.begin(), encoding.end(),
                  [&bytes](char byte) { bytes.append(Json::UInt(static_cast<unsigned char>(byte))); });

    // TODO: fix displaying of operands
    //
    // auto operands = Json::Value(Json::arrayValue);
    // for (unsigned i = 0; i < instr.getNumOperands(); ++i) {
    //   operands.append(Json::String(DescribeOperand(instr.getOperand(i), *regInfo_)));
    // }

    result["bytes"] = std::move(bytes);
    result["opcode"] = Json::String(instrInfo_->getName(instr.getOpcode()));
  }

  return result;
}
