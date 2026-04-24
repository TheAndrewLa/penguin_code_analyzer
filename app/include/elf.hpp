#ifndef APP_ELF_HPP
#define APP_ELF_HPP

#include <cstddef>

#include <filesystem>
#include <memory>
#include <string_view>

namespace analyzer {
class ElfFile;

class ElfSection {
public:
  std::byte *cbegin() const;
  std::byte *cend() const;

private:
  ElfSection() = default;

  friend class ElfFile;

  std::unique_ptr<std::byte[]> m_memory;
};

class ElfFile {
public:
  /// Opens executable file with given `path`
  explicit ElfFile(const std::filesystem::path &path);

  /// Fills the section of ELF file by given name
  ElfSection getSection(std::string_view name);
};
} // namespace analyzer

#endif
