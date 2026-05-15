#ifndef APP_ELF_PARSER_HPP
#define APP_ELF_PARSER_HPP

#include <elf/code_section.hpp>
#include <elf/data_section.hpp>
#include <elf/section.hpp>
#include <elf/utils.hpp>

#include <filesystem>
#include <fstream>

namespace analyzer::elf {
class UnknownSection final : public ElfSection {
public:
  UnknownSection(std::string_view name, usize size);
};

class ElfParser {
public:
  explicit ElfParser(const std::filesystem::path &path);

  std::unique_ptr<ElfSection> getSection(std::string_view sectionName);

private:
  struct SectionDescriptor {
    offset offsetInFile{};
    u64 size{};
    u32 type{};
  };

  void parse();
  void fillSectionPayload(ElfSection &section,
                          const SectionDescriptor &descriptor);
  void enrichSymbols(CodeSection *codeSection, DataSection *dataSection);

  std::ifstream m_file;
  std::unordered_map<std::string, SectionDescriptor> m_sectionOffsets;

  offset m_symbolTableOffset{};
  offset m_symbolStringTableOffset{};

  u64 m_symbolTableSize{};
  u64 m_symbolEntrySize{};
};
} // namespace analyzer::elf

#endif
