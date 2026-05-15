#ifndef APP_ELF_CODE_SECTION_HPP
#define APP_ELF_CODE_SECTION_HPP

#include <elf/section.hpp>
#include <types.hpp>

#include <ranges>
#include <string>
#include <unordered_map>

namespace analyzer::elf {
class ElfParser;

class CodeSection final : public ElfSection {
public:
  using FunctionTable = std::unordered_map<std::string, u64>;
  using FunctionsRange = std::ranges::subrange<FunctionTable::const_iterator>;

  explicit CodeSection(usize size);

  FunctionsRange functions() const;

private:
  friend class ElfParser;

  void setFunctions(FunctionTable &&functions);

  FunctionTable m_functions;
};
} // namespace analyzer::elf

#endif
