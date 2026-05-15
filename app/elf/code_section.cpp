#include <elf/code_section.hpp>

#include <utility>

using namespace analyzer;

analyzer::elf::CodeSection::CodeSection(usize size)
    : ElfSection(".text", size) {}

void analyzer::elf::CodeSection::setFunctions(FunctionTable &&functions) {
  m_functions = std::move(functions);
}

elf::CodeSection::FunctionsRange analyzer::elf::CodeSection::functions() const {
  return {m_functions.cbegin(), m_functions.cend()};
}
