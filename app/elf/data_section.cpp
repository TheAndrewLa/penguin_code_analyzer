#include <elf/data_section.hpp>

#include <utility>

using namespace analyzer;

analyzer::elf::DataSection::DataSection(std::string name, std::size_t size)
    : ElfSection(std::move(name), size) {}

void analyzer::elf::DataSection::setGlobals(Globals globals) {
  m_globals = std::move(globals);
}

auto analyzer::elf::DataSection::globals() const
    -> std::ranges::subrange<Globals::const_iterator> {
  return {m_globals.cbegin(), m_globals.cend()};
}

analyzer::elf::InitializedDataSection::InitializedDataSection(std::size_t size)
    : DataSection(".data", size) {}

analyzer::elf::UninitializedDataSection::UninitializedDataSection(
    std::size_t size)
    : DataSection(".bss", size) {}
