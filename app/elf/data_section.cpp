#include "elf/data_section.hpp"

#include <utility>

namespace analyzer {

DataSection::DataSection(std::string name, std::size_t size)
    : ElfSection(std::move(name), size) {}

void DataSection::setGlobals(Globals globals) { m_globals = std::move(globals); }

auto DataSection::globals() const -> std::ranges::subrange<Globals::const_iterator> {
  return {m_globals.cbegin(), m_globals.cend()};
}

InitializedDataSection::InitializedDataSection(std::size_t size) : DataSection(".data", size) {}

UninitializedDataSection::UninitializedDataSection(std::size_t size) : DataSection(".bss", size) {}

} // namespace analyzer
