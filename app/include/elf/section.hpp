#ifndef APP_ELF_SECTION_HPP
#define APP_ELF_SECTION_HPP

#include <types.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace analyzer::elf {
class ElfSection {
public:
  virtual ~ElfSection() = default;

  usize size() const;
  std::string_view name() const;

protected:
  friend class ElfParser;

  ElfSection(std::string_view name, usize size);

  std::byte *data();

private:
  std::string m_name;
  std::unique_ptr<std::byte[]> m_memory;
  usize m_size{};
};
} // namespace analyzer::elf

#endif
