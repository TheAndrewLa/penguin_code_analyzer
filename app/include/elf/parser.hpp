#ifndef APP_ELF_PARSER_HPP
#define APP_ELF_PARSER_HPP

#include "code_section.hpp"
#include "data_section.hpp"
#include "section.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace analyzer {

class GenericSection final : public ElfSection {
public:
  GenericSection(std::string name, std::size_t size);
};

class ElfFile {
public:
  explicit ElfFile(const std::filesystem::path &path);

  std::unique_ptr<ElfSection> getSection(std::string_view sectionName);

private:
  struct SectionDescriptor {
    std::uint64_t offset{};
    std::uint64_t size{};
    std::uint32_t type{};
  };

  void parse();
  void fillSectionPayload(ElfSection &section, const SectionDescriptor &descriptor);
  void enrichSymbols(CodeSection *codeSection, DataSection *dataSection);

  std::ifstream m_file;
  std::unordered_map<std::string, SectionDescriptor> m_sectionOffsets;

  std::uint64_t m_symbolTableOffset{};
  std::uint64_t m_symbolTableSize{};
  std::uint64_t m_symbolEntrySize{};
  std::uint64_t m_symbolStringTableOffset{};
};

} // namespace analyzer

#endif
