#ifndef X86_SIMULATION_HPP
#define X86_SIMULATION_HPP

#include <json/json.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Error.h>

#include <cstddef>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {
class Target;
class MCInstrInfo;
class MCRegisterInfo;
class MCAsmInfo;
class MCSubtargetInfo;
class MCInst;
namespace mca {
class Instruction;
class InstrBuilder;
} // namespace mca
} // namespace llvm

class X86Simulation {
public:
  using JsonResult = llvm::Expected<Json::Value>;

  X86Simulation();
  X86Simulation(const X86Simulation &) = delete;
  X86Simulation(X86Simulation &&) = delete;

  ~X86Simulation();

  X86Simulation &operator=(const X86Simulation &) = delete;
  X86Simulation &operator=(X86Simulation &&) = delete;

  std::string start(const std::string &cpu, const std::vector<std::string> &instructions);
  void end(const std::string &session);

  JsonResult getGeneralResults(const std::string &session);
  JsonResult getInstructionInfo(const std::string &session);
  JsonResult getTimeline(const std::string &session);

private:
  static constexpr auto DefaultTargetTriple = "x86_64-unknown-linux-gnu";

  static constexpr auto DefaultIterations = 100;
  static constexpr auto DefaultTimelineSize = 10U;

  static constexpr auto DefaultCallLatency = 100;

  struct GeneralInfo {
    unsigned iterations;
    unsigned instructions;
    unsigned totalCycles;
    unsigned totalMicroOps;
    double uOpsPerCycle;
    double instructionsPerCycle;
    double blockRThroughput;
  };

  struct InstructionEntry {
    std::size_t instrIndex;
    unsigned uOps;
    unsigned latency;
    double rThroughput;
    bool mayLoad;
    bool mayStore;
    bool sideFx;
  };

  struct TimelineEntry {
    std::size_t iteration;
    std::size_t instrIndex;
    llvm::SmallVector<std::size_t> dispatchCycles;
    llvm::SmallVector<std::size_t> waitQueueCycles;
    llvm::SmallVector<std::size_t> executeCycles;
    llvm::SmallVector<std::size_t> executeEndCycles;
    llvm::SmallVector<std::size_t> waitRetireCycles;
    llvm::SmallVector<std::size_t> retireCycles;
  };

  struct Simulation {
    GeneralInfo general{};
    std::vector<InstructionEntry> instructionInfo;
    std::vector<TimelineEntry> timeline;
    std::string error;
    bool success = false;
  };

  using SimulationResult = llvm::Expected<Simulation>;

  using ParsedInstructions = std::vector<llvm::MCInst>;
  using LoweredInstructions = std::vector<std::unique_ptr<llvm::mca::Instruction>>;

  struct LoweredProgram {
    std::unique_ptr<llvm::mca::InstrBuilder> builder;
    LoweredInstructions instructions;
  };

  static std::string newSession();

  static Simulation collectResults(const llvm::MCSubtargetInfo &subtargetInfo, const ParsedInstructions &instructions,
                                   const LoweredInstructions &lowered, unsigned totalCycles);

  ParsedInstructions parse(const llvm::MCSubtargetInfo &subtargetInfo,
                           const std::vector<std::string> &instructions) const;

  LoweredProgram lower(const llvm::MCSubtargetInfo &subtargetInfo, const ParsedInstructions &instructions) const;

  unsigned run(const llvm::MCSubtargetInfo &subtargetInfo, const LoweredInstructions &lowered,
               std::vector<TimelineEntry> &timeline) const;

  const llvm::Target *target_;

  std::unique_ptr<llvm::MCRegisterInfo> regInfo_;
  std::unique_ptr<llvm::MCAsmInfo> asmInfo_;
  std::unique_ptr<llvm::MCInstrInfo> instrInfo_;

  std::unordered_map<std::string, Simulation> results_;

  mutable std::shared_mutex resultsMutex_;
};

#endif // X86_SIMULATION_HPP
