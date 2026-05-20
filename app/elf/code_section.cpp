#include <elf/code_section.hpp>

#include <stdexcept>

using namespace analyzer;
using namespace analyzer::elf;

analyzer::elf::CodeSection::Iterator::Iterator(const SectionData *section,
                                               std::size_t index) noexcept
    : m_section(section), m_index(index) {}

CodeSection::Iterator::value_type
analyzer::elf::CodeSection::Iterator::operator*() const {
  return {m_section->name(), m_section->memory()};
}

CodeSection::Iterator &analyzer::elf::CodeSection::Iterator::operator++() {
  ++m_index;
  return *this;
}

CodeSection::Iterator
analyzer::elf::CodeSection::Iterator::operator++(int) const {
  auto copy = *this;
  ++copy.m_index;
  return copy;
}

analyzer::elf::CodeSection::CodeSection(const SectionData *textSection)
    : m_textSection(textSection) {
  if (m_textSection == nullptr || m_textSection->name() != ".text") {
    throw std::invalid_argument{"Provided data of section is invalid"};
  }
}
