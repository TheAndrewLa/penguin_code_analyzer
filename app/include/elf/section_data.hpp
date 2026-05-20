#ifndef APP_ELF_SECTION_DATA_HPP
#define APP_ELF_SECTION_DATA_HPP

#include <elf/utils.hpp>
#include <string>
#include <string_view>
#include <utils/memory_buffer.hpp>

namespace analyzer::elf {
class SectionData {
public:
  SectionData() = default;

  SectionData(std::string_view name, SectionHeader header,
              utils::MemoryView memory)
      : m_name(name), m_header(header), m_memory(memory) {}

  std::string_view name() const noexcept { return m_name; }

  const SectionHeader &header() const noexcept { return m_header; }

  utils::MemoryView memory() const noexcept { return m_memory; }

private:
  std::string m_name;
  SectionHeader m_header{};
  utils::MemoryView m_memory;
};
} // namespace analyzer::elf

#endif
