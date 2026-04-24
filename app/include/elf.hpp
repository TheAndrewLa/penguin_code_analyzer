#ifndef APP_ELF_HPP
#define APP_ELF_HPP

#include <cstddef>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>

namespace analyzer {
class ElfFile;

class ElfSection {
public:
  /// Returns pointer to the start of section's content
  std::byte *cbegin() const;

  /// Returns pointer to the end of section's content
  std::byte *cend() const;

private:
  ElfSection() = default;

  friend class ElfFile;

  std::unique_ptr<std::byte[]> m_memory;
};

class ElfFile {
public:
  /// Opens executable file with given `path`
  ///
  /// Throws `std::invalid argument` if file was not opened.
  explicit ElfFile(const std::filesystem::path &path);

  /// Returns the section of executable file (see `ElfSection` class) by given
  /// `sectionName`
  ///
  /// Throws `std::invalid_argument` if section doesn't exist
  ElfSection getSection(std::string_view sectionName);

private:
  std::ifstream m_file;
};
} // namespace analyzer

#endif
