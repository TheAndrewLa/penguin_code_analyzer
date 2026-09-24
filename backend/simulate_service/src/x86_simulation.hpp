#ifndef X86_SIMULATION_HPP
#define X86_SIMULATION_HPP

#include <json/json.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Error.h>

#include <map>
#include <memory>
#include <string>
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
} // namespace mca
} // namespace llvm

class X86Simulation {
  static constexpr auto AproximateResourcesCount = 16;
  static constexpr auto AproximateInstructionsCount = 8;

public:
  struct GeneralInfo {
    unsigned iterations;
    unsigned instructions;
    unsigned totalCycles;
    unsigned totalMicroOps;
    float uOpsPerCycle;
    float instructionsPerCycle;
    float blockRThroughput;
  };

  struct InstructionInfo {
    unsigned uOps;
    unsigned latency;
    double rThroughput;
    bool mayLoad;
    bool mayStore;
    bool sideFx;
  };

  using ResourceUsage = llvm::SmallVector<float, AproximateResourcesCount>;

  using JsonResult = llvm::Expected<Json::Value>;

  X86Simulation();
  X86Simulation(const X86Simulation &) = delete;
  X86Simulation(X86Simulation &&) = delete;

  ~X86Simulation();

  X86Simulation &operator=(const X86Simulation &) = delete;
  X86Simulation &operator=(X86Simulation &&) = delete;

  std::string start(const std::string &cpu, const std::vector<std::string> &instructions);
  void end(const std::string &session);

  JsonResult getGeneralResults(const std::string &token);
  JsonResult getInstructionInfo(const std::string &token);
  JsonResult getResourceUsage(const std::string &token);

private:
  static constexpr auto DefaultTargetTriple = "x86_64-unknown-linux-gnu";

  static constexpr auto DefaultIterations = 100;
  static constexpr auto DefaultCallLatency = 100;

  struct SimulationData {
    GeneralInfo general{};
    std::map<std::string, InstructionInfo> instructionInfo;
    std::map<std::string, ResourceUsage> resourceUsage;
    std::string error;
    bool success = false;
  };

  static std::string newSession();

  static SimulationData collectResults(const llvm::MCSubtargetInfo &subtargetInfo,
                                       const std::vector<llvm::MCInst> &instructions,
                                       const std::vector<std::unique_ptr<llvm::mca::Instruction>> &lowered,
                                       unsigned totalCycles, const std::vector<std::string> &instructionStrings);

  std::vector<llvm::MCInst> parse(const llvm::MCSubtargetInfo &subtargetInfo,
                                  const std::vector<std::string> &instructions) const;

  std::vector<std::unique_ptr<llvm::mca::Instruction>> lower(const llvm::MCSubtargetInfo &subtargetInfo,
                                                             const std::vector<llvm::MCInst> &instructions) const;

  unsigned run(const llvm::MCSubtargetInfo &subtargetInfo,
               const std::vector<std::unique_ptr<llvm::mca::Instruction>> &lowered) const;

  const llvm::Target *target_;

  std::unique_ptr<llvm::MCRegisterInfo> regInfo_;
  std::unique_ptr<llvm::MCAsmInfo> asmInfo_;
  std::unique_ptr<llvm::MCInstrInfo> instrInfo_;

  std::unordered_map<std::string, SimulationData> results_;
};

#endif // X86_SIMULATION_HPP
