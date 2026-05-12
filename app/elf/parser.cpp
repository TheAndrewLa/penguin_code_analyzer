#include "elf/parser.hpp"

#include <cstring>
#include <elf.h>
#include <stdexcept>

namespace analyzer {
namespace {

template <typename T>
void readStruct(std::ifstream &file, std::streamoff offset, T &out) {
  file.seekg(offset);
  file.read(reinterpret_cast<char *>(&out), static_cast<std::streamsize>(sizeof(T)));
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

std::string guessTypeBySize(std::uint64_t size) {
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
    return "byte[" + std::to_string(size) + "]";
  }
}

} // namespace

GenericSection::GenericSection(std::string name, std::size_t size)
    : ElfSection(std::move(name), size) {}

ElfFile::ElfFile(const std::filesystem::path &path) : m_file(path, std::ios::binary) {
  if (!m_file.is_open()) {
    throw std::invalid_argument("Could not open ELF file: " + path.string());
  }

  parse();
}

void ElfFile::parse() {
  Elf64_Ehdr header{};
  readStruct(m_file, 0, header);

  if (std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
    throw std::invalid_argument("File is not an ELF binary");
  }
  if (header.e_ident[EI_CLASS] != ELFCLASS64 || header.e_ident[EI_DATA] != ELFDATA2LSB) {
    throw std::invalid_argument("Only ELF64 little-endian binaries are supported");
  }

  Elf64_Shdr shstrSection{};
  readStruct(m_file,
             static_cast<std::streamoff>(header.e_shoff +
                                         static_cast<std::uint64_t>(header.e_shstrndx) *
                                             header.e_shentsize),
             shstrSection);

  for (std::uint16_t i = 0; i < header.e_shnum; ++i) {
    Elf64_Shdr shdr{};
    readStruct(m_file,
               static_cast<std::streamoff>(header.e_shoff + static_cast<std::uint64_t>(i) *
                                           header.e_shentsize),
               shdr);

    const auto name = readNullTerminatedString(m_file, shstrSection.sh_offset, shdr.sh_name);
    m_sectionOffsets[name] = SectionDescriptor{shdr.sh_offset, shdr.sh_size, shdr.sh_type};

    if (shdr.sh_type == SHT_SYMTAB) {
      m_symbolTableOffset = shdr.sh_offset;
      m_symbolTableSize = shdr.sh_size;
      m_symbolEntrySize = shdr.sh_entsize;

      Elf64_Shdr symstrHdr{};
      readStruct(m_file,
                 static_cast<std::streamoff>(header.e_shoff +
                                             static_cast<std::uint64_t>(shdr.sh_link) *
                                                 header.e_shentsize),
                 symstrHdr);
      m_symbolStringTableOffset = symstrHdr.sh_offset;
    }
  }
}

void ElfFile::fillSectionPayload(ElfSection &section, const SectionDescriptor &descriptor) {
  if (descriptor.size == 0 || descriptor.type == SHT_NOBITS) {
    return;
  }

  m_file.seekg(static_cast<std::streamoff>(descriptor.offset));
  m_file.read(reinterpret_cast<char *>(section.data()), static_cast<std::streamsize>(descriptor.size));
  if (!m_file) {
    throw std::runtime_error("Unable to read section payload: " + std::string(section.name()));
  }
}

void ElfFile::enrichSymbols(CodeSection *codeSection, DataSection *dataSection) {
  if (m_symbolTableOffset == 0 || m_symbolEntrySize == 0 || m_symbolStringTableOffset == 0) {
    return;
  }

  CodeSection::FunctionTable functions;
  DataSection::Globals globals;

  const auto symbolsCount = m_symbolTableSize / m_symbolEntrySize;
  for (std::uint64_t i = 0; i < symbolsCount; ++i) {
    Elf64_Sym sym{};
    readStruct(m_file, static_cast<std::streamoff>(m_symbolTableOffset + i * m_symbolEntrySize), sym);

    if (sym.st_name == 0 || sym.st_shndx == SHN_UNDEF) {
      continue;
    }

    const auto symbolName = readNullTerminatedString(m_file, m_symbolStringTableOffset, sym.st_name);
    const auto symbolType = ELF64_ST_TYPE(sym.st_info);

    if (codeSection != nullptr && symbolType == STT_FUNC) {
      functions[symbolName] = sym.st_value;
    }

    if (dataSection != nullptr && symbolType == STT_OBJECT) {
      globals.push_back(DataSection::GlobalVariable{
          symbolName,
          guessTypeBySize(sym.st_size),
          sym.st_size,
          sym.st_value == 0 ? 1ULL : (sym.st_value & (~sym.st_value + 1ULL))});
    }
  }

  if (codeSection != nullptr) {
    codeSection->setFunctions(std::move(functions));
  }
  if (dataSection != nullptr) {
    dataSection->setGlobals(std::move(globals));
  }
}

std::unique_ptr<ElfSection> ElfFile::getSection(std::string_view sectionName) {
  const auto it = m_sectionOffsets.find(std::string(sectionName));
  if (it == m_sectionOffsets.end()) {
    throw std::invalid_argument("Section not found: " + std::string(sectionName));
  }

  const auto &descriptor = it->second;
  std::unique_ptr<ElfSection> section;

  if (sectionName == ".text") {
    auto typedSection = std::make_unique<CodeSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(typedSection.get(), nullptr);
    section = std::move(typedSection);
  } else if (sectionName == ".data") {
    auto typedSection = std::make_unique<InitializedDataSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(nullptr, typedSection.get());
    section = std::move(typedSection);
  } else if (sectionName == ".bss") {
    auto typedSection = std::make_unique<UninitializedDataSection>(descriptor.size);
    fillSectionPayload(*typedSection, descriptor);
    enrichSymbols(nullptr, typedSection.get());
    section = std::move(typedSection);
  } else {
    section = std::make_unique<GenericSection>(std::string(sectionName), descriptor.size);
    fillSectionPayload(*section, descriptor);
  }

  return section;
}

} // namespace analyzer
