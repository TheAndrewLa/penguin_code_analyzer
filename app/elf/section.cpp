#include "elf/section.hpp"

#include <utility>

namespace analyzer {

ElfSection::ElfSection(std::string name, std::size_t size)
    : m_name(std::move(name)), m_memory(size ? std::make_unique<std::byte[]>(size) : nullptr),
      m_size(size) {}

const std::byte *ElfSection::cbegin() const { return m_memory.get(); }

const std::byte *ElfSection::cend() const { return m_memory.get() + m_size; }

std::size_t ElfSection::size() const { return m_size; }

std::string_view ElfSection::name() const { return m_name; }

std::byte *ElfSection::data() { return m_memory.get(); }

} // namespace analyzer
