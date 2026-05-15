#include <elf/data_section.hpp>

#include <utility>

using namespace analyzer;

analyzer::elf::DataSection::DataSection(std::string_view name, usize size)
    : ElfSection(name, size) {}

void analyzer::elf::DataSection::setGlobals(Globals &&globals) {
  m_globals = std::move(globals);
}

elf::DataSection::GlobalsRange analyzer::elf::DataSection::globals() const {
  return {m_globals.cbegin(), m_globals.cend()};
}

analyzer::elf::InitializedDataSection::InitializedDataSection(usize size)
    : DataSection(".data", size) {}

analyzer::elf::UninitializedDataSection::UninitializedDataSection(
    std::size_t size)
    : DataSection(".bss", size) {}
