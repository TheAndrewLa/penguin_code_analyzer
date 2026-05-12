#ifndef APP_ELF_SECTION_HPP
#define APP_ELF_SECTION_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace analyzer {

class ElfSection {
public:
  virtual ~ElfSection() = default;

  const std::byte *cbegin() const;
  const std::byte *cend() const;
  std::size_t size() const;
  std::string_view name() const;

protected:
  ElfSection(std::string name, std::size_t size);

  std::byte *data();

friend class ElfFile;

private:
  std::string m_name;
  std::unique_ptr<std::byte[]> m_memory;
  std::size_t m_size{};
};

} // namespace analyzer

#endif
