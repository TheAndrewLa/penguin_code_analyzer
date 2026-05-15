#include <elf/parser.hpp>

#include <elf/section.hpp>
#include <elf/utils.hpp>
#include <io.hpp>

#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>

using namespace analyzer;

namespace {
std::string typeBySize(usize size) {
  switch (size) {
  case 1:
    return "uint8";
  case 2:
    return "uint16";
  case 4:
    return "uint32";
  case 8:
    return "uint64";
  default:
    return std::format("uint8[{}]", size);
  }
}
} // namespace

analyzer::elf::UnknownSection::UnknownSection(std::string_view name, usize size)
    : ElfSection(name, size) {}

analyzer::elf::ElfParser::ElfParser(const std::filesystem::path &path)
    : m_file(path, std::ios::binary) {
  if (!m_file.is_open()) {
    throw std::invalid_argument{
        std::format("Could not open ELF file: {}", path.string())};
  }
  parse();
}

void analyzer::elf::ElfParser::parse() {
  elf::Header header{};
  io::ReadStruct(m_file, 0, header);

  if (!Header::ValidMagic(header)) {
    throw std::invalid_argument("File is not an ELF binary");
  }

  if (header.ident[elf::Header::INDEX_CLASS] != elf::Header::CLASS_64 ||
      header.ident[elf::Header::INDEX_DATA] != elf::Header::DATA_LSB) {
    throw std::invalid_argument{
        "Only ELF64 little-endian binaries are supported"};
  }

  elf::SectionHeader shstrSection{};

  io::ReadStruct(m_file,
                 static_cast<std::streamoff>(
                     header.sectionHeaderOffset +
                     static_cast<offset>(header.sectionHeaderStringTable) *
                         header.sectionHeaderEntrySize),
                 shstrSection);

  for (u16 i = 0; i < header.sectionHeaderNum; ++i) {
    elf::SectionHeader shdr{};
    io::ReadStruct(m_file,
                   static_cast<std::streamoff>(
                       header.sectionHeaderOffset +
                       static_cast<offset>(i) * header.sectionHeaderEntrySize),
                   shdr);

    const auto name = io::ReadString(
        m_file,
        static_cast<std::streamoff>(shstrSection.offsetInFile + shdr.name));

    m_sectionOffsets[name] =
        SectionDescriptor{shdr.offsetInFile, shdr.size, shdr.type};

    if (shdr.type == SectionHeader::TYPE_SYMTAB) {
      m_symbolTableOffset = shdr.offsetInFile;
      m_symbolTableSize = shdr.size;
      m_symbolEntrySize = shdr.entrySize;

      elf::SectionHeader symstrHdr{};

      io::ReadStruct(
          m_file,
          static_cast<std::streamoff>(header.sectionHeaderOffset +
                                      static_cast<offset>(shdr.link) *
                                          header.sectionHeaderEntrySize),
          symstrHdr);

      m_symbolStringTableOffset = symstrHdr.offsetInFile;
    }
  }
}

void analyzer::elf::ElfParser::fillSectionPayload(
    ElfSection &section, const SectionDescriptor &descriptor) {
  if (descriptor.size == 0 ||
      descriptor.type == elf::SectionHeader::TYPE_NOBITS) {
    return;
  }

  m_file.seekg(static_cast<std::streamoff>(descriptor.offsetInFile));
  m_file.read(reinterpret_cast<char *>(section.data()),
              static_cast<std::streamsize>(descriptor.size));

  if (!m_file) {
    throw std::runtime_error{
        std::format("Unable to read section payload: {}", section.name())};
  }
}

void analyzer::elf::ElfParser::enrichSymbols(CodeSection *codeSection,
                                             DataSection *dataSection) {
  if (m_symbolTableOffset == 0 || m_symbolEntrySize == 0 ||
      m_symbolStringTableOffset == 0) {
    return;
  }

  CodeSection::FunctionTable functions;
  DataSection::Globals globals;

  const auto symbolsCount = m_symbolTableSize / m_symbolEntrySize;

  for (u64 i = 0; i < symbolsCount; ++i) {
    elf::Symbol sym{};
    io::ReadStruct(m_file,
                   static_cast<std::streamoff>(m_symbolTableOffset +
                                               i * m_symbolEntrySize),
                   sym);

    if (sym.name == 0 ||
        sym.sectionHeaderIndex == Symbol::SECTION_INDEX_UNDEF) {
      continue;
    }

    const auto symbolType = Symbol::Type(sym);
    const auto symbolName = io::ReadString(
        m_file,
        static_cast<std::streamoff>(m_symbolStringTableOffset + sym.name));

    if (codeSection != nullptr && symbolType == Symbol::TYPE_FUNCTION) {
      functions[symbolName] = sym.value;
    }

    if (dataSection != nullptr && symbolType == Symbol::TYPE_OBJECT) {
      globals.push_back(DataSection::GlobalVariable{
          symbolName, typeBySize(sym.size), sym.size,
          sym.value == 0 ? u64(1) : (sym.value & (~sym.value + u64(1)))});
    }
  }

  if (codeSection != nullptr) {
    codeSection->m_functions = std::move(functions);
  }

  if (dataSection != nullptr) {
    dataSection->m_globals = std::move(globals);
  }
}

std::unique_ptr<elf::ElfSection>
analyzer::elf::ElfParser::getSection(std::string_view sectionName) {
  const auto iter = m_sectionOffsets.find(std::string(sectionName));
  if (iter == m_sectionOffsets.end()) {
    throw std::invalid_argument{
        std::format("Section not found {}", std::string(sectionName))};
  }

  const auto &descriptor = iter->second;
  std::unique_ptr<ElfSection> section;

  if (sectionName == ".text") {
    auto codeSection = std::make_unique<CodeSection>(descriptor.size);
    fillSectionPayload(*codeSection, descriptor);
    enrichSymbols(codeSection.get(), nullptr);
    section = std::move(codeSection);
  } else if (sectionName == ".data") {
    auto dataSection =
        std::make_unique<InitializedDataSection>(descriptor.size);
    fillSectionPayload(*dataSection, descriptor);
    enrichSymbols(nullptr, dataSection.get());
    section = std::move(dataSection);
  } else if (sectionName == ".bss") {
    auto bssSection =
        std::make_unique<UninitializedDataSection>(descriptor.size);
    fillSectionPayload(*bssSection, descriptor);
    enrichSymbols(nullptr, bssSection.get());
    section = std::move(bssSection);
  } else {
    section = std::make_unique<UnknownSection>(sectionName, descriptor.size);
    fillSectionPayload(*section, descriptor);
  }

  return section;
}
