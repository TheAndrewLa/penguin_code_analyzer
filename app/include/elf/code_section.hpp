#ifndef APP_ELF_CODE_SECTION_HPP
#define APP_ELF_CODE_SECTION_HPP

#include <elf/section.hpp>

#include <cstdint>
#include <ranges>
#include <string>
#include <unordered_map>

namespace analyzer::elf {
class CodeSection final : public ElfSection {
public:
  using FunctionTable = std::unordered_map<std::string, std::uint64_t>;

  explicit CodeSection(std::size_t size);

  void setFunctions(FunctionTable functions);
  auto
  functions() const -> std::ranges::subrange<FunctionTable::const_iterator>;

private:
  FunctionTable m_functions;
};
} // namespace analyzer::elf

#endif
