#ifndef APP_ELF_FILE_HPP
#define APP_ELF_FILE_HPP

#include <elf/code_section.hpp>
#include <elf/data_section.hpp>
#include <elf/platform.hpp>
#include <elf/section_data.hpp>
#include <filesystem>
#include <utility>
#include <utils/memory_buffer.hpp>

namespace analyzer::elf {
/// Represents an ELF file.
/// Provides functionality for getting sections, symbols, and other ELF file
/// metadata.
class File {
public:
  File() = delete;

  /// Constructs a Parser from the given file path.
  /// Performs a validation: checks magic bytes, platform, type.
  ///
  /// `path` - The path to the ELF file to parse.
  explicit File(const std::filesystem::path &path);

  File(const File &) = delete;
  File(File &&) = default;

  ~File() noexcept = default;

  File &operator=(const File &) = delete;
  File &operator=(File &&) = default;

  Platform platform() const noexcept;

  SectionData getSection(std::string_view name) const;

  std::pair<SectionData, CodeSection> codeSection() const;
  std::pair<SectionData, DataSection> dataSection() const;
  std::pair<SectionData, RodataSection> rodataSection() const;
  std::pair<SectionData, DataSectionTLS> dataSectionTLS() const;

private:
  utils::MemoryBuffer m_fileContent;
};
} // namespace analyzer::elf

#endif
