#ifndef X86_INSTR_INFO_HPP
#define X86_INSTR_INFO_HPP

#include <json/json.h>
#include <llvm/Support/Error.h>

#include <string>

namespace llvm {
class Target;
class MCInstrInfo;
class MCRegisterInfo;
class MCAsmInfo;
} // namespace llvm

class X86InstrInfo {
public:
  using JsonResult = llvm::Expected<Json::Value>;

  X86InstrInfo();
  X86InstrInfo(const X86InstrInfo &) = delete;
  X86InstrInfo(X86InstrInfo &&) = delete;

  ~X86InstrInfo();

  X86InstrInfo &operator=(const X86InstrInfo &) = delete;
  X86InstrInfo &operator=(X86InstrInfo &&) = delete;

  JsonResult brief(const std::string &cpu, const std::string &instruction) const;
  JsonResult full(const std::string &cpu, const std::string &instruction) const;

private:
  static constexpr auto DefaultTargetTriple = "x86_64-unknown-linux-gnu";

  static constexpr auto MaxFixups = 4;
  static constexpr auto MaxEncodingSize = 16;

  const llvm::Target *target_;

  std::unique_ptr<llvm::MCRegisterInfo> regInfo_;
  std::unique_ptr<llvm::MCAsmInfo> asmInfo_;
  std::unique_ptr<llvm::MCInstrInfo> instrInfo_;

  JsonResult analyze(const std::string &cpu, const std::string &instrText, bool full) const;
};

#endif // X86_INSTR_INFO_HPP
