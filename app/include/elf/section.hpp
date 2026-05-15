#ifndef APP_ELF_SECTION_HPP
#define APP_ELF_SECTION_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace analyzer::elf {
class ElfSection {
public:
  virtual ~ElfSection() = default;

  std::size_t size() const;
  std::string_view name() const;

protected:
  ElfSection(std::string_view name, std::size_t size);

  std::byte *data();

  friend class ElfFile;

private:
  std::string m_name;
  std::unique_ptr<std::byte[]> m_memory;
  std::size_t m_size{};
};
} // namespace analyzer::elf

#endif
