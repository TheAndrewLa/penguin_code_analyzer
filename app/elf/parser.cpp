#include <elf/parser.hpp>
#include <elf/section.hpp>
#include <elf/utils.hpp>

#include <cstring>
#include <format>
#include <stdexcept>

using namespace analyzer;

namespace {
template <typename T>
void readStruct(std::ifstream &file, std::streamoff offset, T &out) {
  file.seekg(offset);
  file.read(reinterpret_cast<char *>(&out),
            static_cast<std::streamsize>(sizeof(T)));
  if (!file) {
    throw std::runtime_error("Unexpected EOF while reading ELF structure");
  }
}

std::string readNullTerminatedString(std::ifstream &file, std::uint64_t base,
                                     std::uint32_t offset) {
  std::string result;
  file.seekg(static_cast<std::streamoff>(base + offset));
  for (char ch = 0; file.get(ch) && ch != '\0';) {
    result.push_back(ch);
  }
  if (!file && !file.eof()) {
    throw std::runtime_error("Failed while reading ELF string table");
  }
  return result;
}

std::string typeBySize(std::uint64_t size) {
  switch (size) {
  case 1:
    return "uint8_t";
  case 2:
    return "uint16_t";
  case 4:
    return "uint32_t";
  case 8:
    return "uint64_t";
  default:
    return std::format("byte[{}]", size);
  }
}
} // namespace

analyzer::elf::GenericSection::GenericSection(std::string name,
                                              std::size_t size)
    : ElfSection(std::move(name), size) {}

analyzer::elf::ElfFile::ElfFile(const std::filesystem::path &path)
    : m_file(path, std::ios::binary) {
  if (!m_file.is_open()) {
    throw std::invalid_argument{
        std::format("Could not open ELF file: {}", path.string())};
  }

  parse();
}

void analyzer::elf::ElfFile::parse() {
  elf::Header header{};
  readStruct(m_file, 0, header);

  if (std::memcmp(header.ident, Header::MAGIC, Header::MAGIC_SIZE) != 0) {
    throw std::invalid_argument("File is not an ELF binary");
  }

  if (header.ident[elf::Header::INDEX_CLASS] != elf::Header::CLASS_64 ||
      header.ident[elf::Header::INDEX_DATA] != elf::Header::DATA_LSB) {
    throw std::invalid_argument{
        "Only ELF64 little-endian binaries are supported"};
  }

  elf::SectionHeader shstrSection{};

  readStruct(m_file,
             static_cast<std::streamoff>(
                 header.sectionHeaderOffset +
                 static_cast<std::uint64_t>(header.sectionHeaderStringTable) *
                     header.sectionHeaderEntrySize),
             shstrSection);

  for (std::uint16_t i = 0; i < header.sectionHeaderNum; ++i) {
    elf::SectionHeader shdr{};
    readStruct(m_file,
               static_cast<std::streamoff>(header.sectionHeaderOffset +
                                           static_cast<std::uint64_t>(i) *
                                               header.sectionHeaderEntrySize),
               shdr);

    const auto name =
        readNullTerminatedString(m_file, shstrSection.offset, shdr.name);

    m_sectionOffsets[name] =
        SectionDescriptor{shdr.offset, shdr.size, shdr.type};

    if (shdr.type == SectionHeader::TYPE_SYMTAB) {
      m_symbolTableOffset = shdr.offset;
      m_symbolTableSize = shdr.size;
      m_symbolEntrySize = shdr.entrySize;

      elf::SectionHeader symstrHdr{};

      readStruct(
          m_file,
          static_cast<std::streamoff>(header.sectionHeaderOffset +
                                      static_cast<std::uint64_t>(shdr.link) *
                                          header.sectionHeaderEntrySize),
          symstrHdr);
      m_symbolStringTableOffset = symstrHdr.offset;
    }
  }
}

void analyzer::elf::ElfFile::fillSectionPayload(
    ElfSection &section, const SectionDescriptor &descriptor) {
  if (descriptor.size == 0 ||
      descriptor.type == elf::SectionHeader::TYPE_NOBITS) {
    return;
  }

  m_file.seekg(static_cast<std::streamoff>(descriptor.offset));
  m_file.read(reinterpret_cast<char *>(section.data()),
              static_cast<std::streamsize>(descriptor.size));

  if (!m_file) {
    throw std::runtime_error{std::format("Unable to read section payload: {}",
                                         std::string(section.name()))};
  }
}

void analyzer::elf::ElfFile::enrichSymbols(CodeSection *codeSection,
                                           DataSection *dataSection) {
  if (m_symbolTableOffset == 0 || m_symbolEntrySize == 0 ||
      m_symbolStringTableOffset == 0) {
    return;
  }

  CodeSection::FunctionTable functions;
  DataSection::Globals globals;

  const auto symbolsCount = m_symbolTableSize / m_symbolEntrySize;
  for (std::uint64_t i = 0; i < symbolsCount; ++i) {
    elf::Symbol sym{};
    readStruct(m_file,
               static_cast<std::streamoff>(m_symbolTableOffset +
                                           i * m_symbolEntrySize),
               sym);

    if (sym.name == 0 ||
        sym.sectionHeaderIndex == Symbol::SECTION_INDEX_UNDEF) {
      continue;
    }

    const auto symbolName =
        readNullTerminatedString(m_file, m_symbolStringTableOffset, sym.name);
    const auto symbolType = Symbol::Type(sym);

    if (codeSection != nullptr && symbolType == Symbol::TYPE_FUNCTION) {
      functions[symbolName] = sym.value;
    }

    if (dataSection != nullptr && symbolType == Symbol::TYPE_OBJECT) {
      globals.push_back(DataSection::GlobalVariable{
          symbolName, typeBySize(sym.size), sym.size,
          sym.value == 0 ? std::uint64_t(1)
                         : (sym.value & (~sym.value + std::uint64_t(1)))});
    }
  }

  if (codeSection != nullptr) {
    codeSection->setFunctions(std::move(functions));
  }

  if (dataSection != nullptr) {
    dataSection->setGlobals(std::move(globals));
  }
}

std::unique_ptr<elf::ElfSection>
analyzer::elf::ElfFile::getSection(std::string_view sectionName) {
  const auto iter = m_sectionOffsets.find(std::string(sectionName));
  if (iter == m_sectionOffsets.end()) {
    throw std::invalid_argument{
        std::format("Section not found {}", std::string(sectionName))};
  }

  const auto &descriptor = iter->second;
  std::unique_ptr<ElfSection> section;

  if (sectionName == ".text") {
    auto typedSection = std::make_unique<CodeSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(typedSection.get(), nullptr);
    section = std::move(typedSection);
  } else if (sectionName == ".data") {
    auto typedSection =
        std::make_unique<InitializedDataSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(nullptr, typedSection.get());
    section = std::move(typedSection);
  } else if (sectionName == ".bss") {
    auto typedSection =
        std::make_unique<UninitializedDataSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(nullptr, typedSection.get());
    section = std::move(typedSection);
  } else {
    section = std::make_unique<GenericSection>(std::string(sectionName),
                                               descriptor.size);
    fillSectionPayload(*section, descriptor);
  }

  return section;
}
