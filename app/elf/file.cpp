#include <elf/file.hpp>

#include <elf/utils.hpp>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace analyzer;
using namespace analyzer::elf;

namespace {
Architecture parseArchitecture(std::uint16_t machine) {
  switch (machine) {
  case Header::MACHINE_X86:
    return Architecture::X86;
  case Header::MACHINE_X86_64:
    return Architecture::X86_64;
  case Header::MACHINE_ARM:
    return Architecture::ARM;
  case Header::MACHINE_ARM64:
    return Architecture::ARM64;
  case Header::MACHINE_POWERPC:
    return Architecture::POWERPC;
  default:
    throw std::invalid_argument{"Unsupported ELF architecture"};
  }
}

Endianness parseEndianness(unsigned char data) {
  switch (data) {
  case Header::DATA_LSB:
    return Endianness::LITTLE;
  case Header::DATA_MSB:
    return Endianness::BIG;
  default:
    throw std::invalid_argument{"Unsupported ELF endianness"};
  }
}

ABI parseAbi(unsigned char abi) {
  switch (abi) {
  case Header::ABI_LINUX:
    return ABI::LINUX;
  case Header::ABI_FREEBSD:
    return ABI::FREEBSD;
  case Header::ABI_HPUX:
    return ABI::HPUX;
  case Header::ABI_SOLARIS:
    return ABI::SOLARIS;
  case Header::ABI_AIX:
    return ABI::AIX;
  default:
    throw std::invalid_argument{"Unsupported ELF ABI"};
  }
}

std::vector<SectionHeader> readSectionHeaders(const utils::MemoryBuffer &file,
                                              const Header &header) {
  std::vector<SectionHeader> sections;
  sections.reserve(header.sectionHeaderNum);
  for (std::size_t i = 0; i < header.sectionHeaderNum; ++i) {
    SectionHeader section{};
    file.readStruct(header.sectionHeaderOffset +
                        i * header.sectionHeaderEntrySize,
                    section);
    sections.push_back(section);
  }
  return sections;
}

std::string readSectionName(const utils::MemoryBuffer &file,
                            const SectionHeader &stringTable,
                            const SectionHeader &target) {
  return file.readString(stringTable.offset + target.name);
}
} // namespace

analyzer::elf::File::File(const std::filesystem::path &path)
    : m_fileContent([&]() {
        std::ifstream input{path, std::ios::binary};
        if (!input.is_open()) {
          throw std::invalid_argument{"Failed to open ELF file"};
        }
        return utils::MemoryBuffer{input};
      }()) {
  Header header{};
  m_fileContent.readStruct(0, header);

  if (!Header::ValidMagic(header)) {
    throw std::invalid_argument{"Invalid ELF magic bytes"};
  }

  if (header.ident[Header::INDEX_CLASS] != Header::CLASS_64) {
    throw std::invalid_argument{"Invalid ELF class"};
  }

  if (header.ident[Header::INDEX_VERSION] != Header::VERSION_CURRENT ||
      header.version != Header::VERSION_CURRENT) {
    throw std::invalid_argument{"Invalid ELF version"};
  }

  // Calling getters to validate the header fields

  parseArchitecture(header.machine);
  parseEndianness(header.ident[Header::INDEX_DATA]);
  parseAbi(header.ident[Header::INDEX_OSABI]);
}

Platform analyzer::elf::File::platform() const noexcept {
  Header header{};
  m_fileContent.readStruct(0, header);
  return {parseArchitecture(header.machine),
          parseEndianness(header.ident[Header::INDEX_DATA]),
          parseAbi(header.ident[Header::INDEX_OSABI])};
}

SectionData analyzer::elf::File::getSection(std::string_view name) const {
  Header header{};
  m_fileContent.readStruct(0, header);
  const auto sections = readSectionHeaders(m_fileContent, header);

  if (header.sectionHeaderStringTable >= sections.size()) {
    throw std::invalid_argument{"Invalid section header string table index"};
  }

  const auto &stringTable = sections[header.sectionHeaderStringTable];

  for (const auto &section : sections) {
    const auto sectionName =
        readSectionName(m_fileContent, stringTable, section);
    if (sectionName == name) {
      return SectionData{sectionName, section,
                         m_fileContent.toView(section.offset, section.size)};
    }
  }

  throw std::invalid_argument{"Section is not presented in file"};
}

std::pair<SectionData, CodeSection> analyzer::elf::File::codeSection() const {
  std::pair<SectionData, CodeSection> result{getSection(".text"),
                                             CodeSection{}};
  result.second = CodeSection{&result.first};
  return result;
}

std::pair<SectionData, DataSection> analyzer::elf::File::dataSection() const {
  std::pair<SectionData, DataSection> result{getSection(".data"),
                                             DataSection{}};
  result.second = DataSection{&result.first};
  return result;
}

std::pair<SectionData, RodataSection>
analyzer::elf::File::rodataSection() const {
  std::pair<SectionData, RodataSection> result{getSection(".rodata"),
                                               RodataSection{}};
  result.second = RodataSection{&result.first};
  return result;
}

std::pair<SectionData, DataSectionTLS>
analyzer::elf::File::dataSectionTLS() const {
  std::pair<SectionData, DataSectionTLS> result{getSection(".tdata"),
                                                DataSectionTLS{}};
  result.second = DataSectionTLS{&result.first};
  return result;
}
