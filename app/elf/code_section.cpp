#include <elf/code_section.hpp>

#include <utility>

using namespace analyzer;

analyzer::elf::CodeSection::CodeSection(std::size_t size)
    : ElfSection(".text", size) {}

void analyzer::elf::CodeSection::setFunctions(FunctionTable functions) {
  m_functions = std::move(functions);
}

auto analyzer::elf::CodeSection::functions() const
    -> std::ranges::subrange<FunctionTable::const_iterator> {
  return {m_functions.cbegin(), m_functions.cend()};
}
