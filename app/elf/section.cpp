#include <elf/section.hpp>

analyzer::elf::ElfSection::ElfSection(std::string_view name, std::size_t size)
    : m_name(name),
      m_memory((size != 0) ? std::make_unique<std::byte[]>(size) : nullptr),
      m_size(size) {}

std::size_t analyzer::elf::ElfSection::size() const { return m_size; }

std::string_view analyzer::elf::ElfSection::name() const { return m_name; }

std::byte *analyzer::elf::ElfSection::data() { return m_memory.get(); }
