#include "elf/code_section.hpp"

#include <utility>

namespace analyzer {

CodeSection::CodeSection(std::size_t size) : ElfSection(".text", size) {}

void CodeSection::setFunctions(FunctionTable functions) { m_functions = std::move(functions); }

auto CodeSection::functions() const -> std::ranges::subrange<FunctionTable::const_iterator> {
  return {m_functions.cbegin(), m_functions.cend()};
}

} // namespace analyzer
