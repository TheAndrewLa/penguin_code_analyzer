#ifndef APP_ELF_UTILS_HPP
#define APP_ELF_UTILS_HPP

#include <cstddef>
#include <cstdint>

namespace analyzer::elf {
using address_t = std::uint64_t;
using offset_t = std::uint64_t;
using version_t = std::uint16_t;

inline constexpr auto HEADER_IDENT_SIZE = std::size_t(16);

struct Header {
  static constexpr bool ValidMagic(Header header) noexcept {
    constexpr auto MAGIC_0 = '\x7F';
    constexpr auto MAGIC_1 = 'E';
    constexpr auto MAGIC_2 = 'L';
    constexpr auto MAGIC_3 = 'F';

    return header.ident[0] == MAGIC_0 && header.ident[1] == MAGIC_1 &&
           header.ident[2] == MAGIC_2 && header.ident[3] == MAGIC_3;
  }

  static constexpr std::size_t INDEX_CLASS = 4;
  static constexpr std::size_t INDEX_DATA = 5;
  static constexpr std::size_t INDEX_VERSION = 6;
  static constexpr std::size_t INDEX_OSABI = 7;
  static constexpr std::size_t INDEX_ABIVERSION = 8;
  static constexpr std::size_t INDEX_PAD = 9;

  static constexpr char CLASS_NONE = 0;
  static constexpr char CLASS_32 = 1;
  static constexpr char CLASS_64 = 2;

  static constexpr char DATA_NONE = 0;
  static constexpr char DATA_LSB = 1;
  static constexpr char DATA_MSB = 2;

  static constexpr char ABI_NONE = 0;
  static constexpr char ABI_HPUX = 1;
  static constexpr char ABI_NETBSD = 2;
  static constexpr char ABI_LINUX = 3;
  static constexpr char ABI_SOLARIS = 6;
  static constexpr char ABI_AIX = 7;
  static constexpr char ABI_IRIX = 8;
  static constexpr char ABI_FREEBSD = 9;
  static constexpr char ABI_TRU64 = 10;
  static constexpr char ABI_MODESTO = 11;
  static constexpr char ABI_OPENBSD = 12;
  static constexpr char ABI_OPENVMS = 13;
  static constexpr char ABI_NSK = 14;

  static constexpr std::uint16_t MACHINE_X86 = 3;
  static constexpr std::uint16_t MACHINE_X86_64 = 62;
  static constexpr std::uint16_t MACHINE_ARM = 40;
  static constexpr std::uint16_t MACHINE_ARM64 = 183;
  static constexpr std::uint16_t MACHINE_POWERPC = 20;

  static constexpr std::uint32_t VERSION_NONE = 0;
  static constexpr std::uint32_t VERSION_CURRENT = 1;

  unsigned char ident[HEADER_IDENT_SIZE];
  std::uint16_t type;
  std::uint16_t machine;
  std::uint32_t version;
  address_t entry;
  offset_t programHeaderOffset;
  offset_t sectionHeaderOffset;
  std::uint32_t flags;
  std::uint16_t execHeaderSize;
  std::uint16_t programHeaderEntrySize;
  std::uint16_t programHeaderNum;
  std::uint16_t sectionHeaderEntrySize;
  std::uint16_t sectionHeaderNum;
  std::uint16_t sectionHeaderStringTable;
};

struct ProgramHeader {
  static constexpr std::uint32_t TYPE_NULL = 0;
  static constexpr std::uint32_t TYPE_LOAD = 1;
  static constexpr std::uint32_t TYPE_DYNAMIC = 2;
  static constexpr std::uint32_t TYPE_INTERP = 3;
  static constexpr std::uint32_t TYPE_NOTE = 4;
  static constexpr std::uint32_t TYPE_SHLIB = 5;
  static constexpr std::uint32_t TYPE_PHDR = 6;
  static constexpr std::uint32_t TYPE_TLS = 7;

  static constexpr std::uint32_t FLAG_READ = 1;
  static constexpr std::uint32_t FLAG_WRITE = 2;
  static constexpr std::uint32_t FLAG_EXEC = 4;

  std::uint32_t type;
  std::uint32_t flags;
  offset_t offset;
  address_t virtualAddress;
  address_t physicalAddress;
  std::uint64_t sizeInFile;
  std::uint64_t sizeInMemory;
  std::uint64_t align;
};

struct SectionHeader {
  static constexpr std::uint32_t TYPE_NULL = 0;
  static constexpr std::uint32_t TYPE_PROGBITS = 1;
  static constexpr std::uint32_t TYPE_SYMTAB = 2;
  static constexpr std::uint32_t TYPE_STRTAB = 3;
  static constexpr std::uint32_t TYPE_RELA = 4;
  static constexpr std::uint32_t TYPE_HASH = 5;
  static constexpr std::uint32_t TYPE_DYNAMIC = 6;
  static constexpr std::uint32_t TYPE_NOTE = 7;
  static constexpr std::uint32_t TYPE_NOBITS = 8;
  static constexpr std::uint32_t TYPE_REL = 9;
  static constexpr std::uint32_t TYPE_SHLIB = 10;
  static constexpr std::uint32_t TYPE_DYNSYM = 11;

  static constexpr std::uint64_t FLAG_WRITE = 1;
  static constexpr std::uint64_t FLAG_ALLOC = 2;
  static constexpr std::uint64_t FLAG_EXEC = 4;
  static constexpr std::uint64_t FLAG_MERGE = 16;
  static constexpr std::uint64_t FLAG_STRINGS = 32;
  static constexpr std::uint64_t FLAG_INFO_LINK = 64;
  static constexpr std::uint64_t FLAG_LINK_ORDER = 128;
  static constexpr std::uint64_t FLAG_TLS = 1024;

  std::uint32_t name;
  std::uint32_t type;
  std::uint64_t flags;
  offset_t offset;
  address_t virtualAddress;
  address_t physicalAddress;
  std::uint64_t size;
  std::uint32_t link;
  std::uint32_t info;
  std::uint64_t alignment;
  std::uint64_t entrySize;
};

struct Symbol {
  static constexpr unsigned char TYPE_NOTYPE = 0;
  static constexpr unsigned char TYPE_OBJECT = 1;
  static constexpr unsigned char TYPE_FUNCTION = 2;
  static constexpr unsigned char TYPE_SECTION = 3;
  static constexpr unsigned char TYPE_FILE = 4;
  static constexpr unsigned char TYPE_COMMON = 5;
  static constexpr unsigned char TYPE_TLS = 6;

  static constexpr unsigned char BIND_LOCAL = 0;
  static constexpr unsigned char BIND_GLOBAL = 1;
  static constexpr unsigned char BIND_WEAK = 2;

  static constexpr unsigned char Type(Symbol symbol) {
    return symbol.info >> static_cast<unsigned char>(0x4);
  }

  static constexpr unsigned char Bind(Symbol symbol) {
    return symbol.info & static_cast<unsigned char>(0xF);
  }

  static constexpr unsigned char VISIBILITY_DEFAULT = 0;
  static constexpr unsigned char VISIBILITY_INTERNAL = 1;
  static constexpr unsigned char VISIBILITY_HIDDEN = 2;
  static constexpr unsigned char VISIBILITY_PROTECTED = 3;

  static constexpr std::uint16_t SECTION_INDEX_UNDEF = 0;

  std::uint32_t name;
  unsigned char info;
  unsigned char other;
  std::uint16_t sectionHeaderIndex;
  address_t value;
  std::uint64_t size;
};
} // namespace analyzer::elf

#endif
