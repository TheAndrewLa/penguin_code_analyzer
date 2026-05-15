#ifndef APP_ELF_UTILS_HPP
#define APP_ELF_UTILS_HPP

#include <cstddef>
#include <types.hpp>

namespace analyzer::elf {
using address = std::uint64_t;
using offset = std::uint64_t;
using version = std::uint16_t;

static constexpr auto IDENT_SIZE = std::size_t(16);

struct Header {
  static constexpr std::size_t INDEX_MAG0 = 0;
  static constexpr std::size_t INDEX_MAG1 = 1;
  static constexpr std::size_t INDEX_MAG2 = 2;
  static constexpr std::size_t INDEX_MAG3 = 3;
  static constexpr std::size_t INDEX_CLASS = 4;
  static constexpr std::size_t INDEX_DATA = 5;
  static constexpr std::size_t INDEX_VERSION = 6;
  static constexpr std::size_t INDEX_OSABI = 7;
  static constexpr std::size_t INDEX_ABIVERSION = 8;
  static constexpr std::size_t INDEX_PAD = 9;

  static constexpr auto MAGIC = "\177ELF";
  static constexpr std::size_t MAGIC_SIZE = 4;

  static constexpr auto MAGIC_0 = char(0x7F);
  static constexpr auto MAGIC_1 = 'E';
  static constexpr auto MAGIC_2 = 'L';
  static constexpr auto MAGIC_3 = 'F';

  static constexpr char CLASS_NONE = 0;
  static constexpr char CLASS_32 = 1;
  static constexpr char CLASS_64 = 2;

  static constexpr char DATA_NONE = 0;
  static constexpr char DATA_LSB = 1;
  static constexpr char DATA_MSB = 2;

  static constexpr u32 VERSION_NONE = 0;
  static constexpr u32 VERSION_CURRENT = 1;

  unsigned char ident[IDENT_SIZE];
  u16 type;
  u16 machine;
  u32 version;
  address entry;
  offset programHeaderOffset;
  offset sectionHeaderOffset;
  u32 flags;
  u16 execHeaderSize;
  u16 programHeaderEntrySize;
  u16 programHeaderNum;
  u16 sectionHeaderEntrySize;
  u16 sectionHeaderNum;
  u16 sectionHeaderStringTable;
};

struct ProgramHeader {
  static constexpr u32 FLAG_READ = 1;
  static constexpr u32 FLAG_WRITE = 2;
  static constexpr u32 FLAG_EXEC = 4;

  u32 type;
  u32 flags;
  offset offset;
  address virtualAddress;
  address physicalAddress;
  u64 sizeInFile;
  u64 sizeInMemory;
  u64 align;
};

struct SectionHeader {
  static constexpr u32 TYPE_NULL = 0;
  static constexpr u32 TYPE_PROGBITS = 1;
  static constexpr u32 TYPE_SYMTAB = 2;
  static constexpr u32 TYPE_STRTAB = 3;
  static constexpr u32 TYPE_RELA = 4;
  static constexpr u32 TYPE_HASH = 5;
  static constexpr u32 TYPE_DYNAMIC = 6;
  static constexpr u32 TYPE_NOTE = 7;
  static constexpr u32 TYPE_NOBITS = 8;
  static constexpr u32 TYPE_REL = 9;
  static constexpr u32 TYPE_SHLIB = 10;
  static constexpr u32 TYPE_DYNSYM = 11;

  static constexpr u64 FLAG_WRITE = 1;
  static constexpr u64 FLAG_ALLOC = 2;
  static constexpr u64 FLAG_EXEC = 4;
  static constexpr u64 FLAG_MERGE = 16;
  static constexpr u64 FLAG_STRINGS = 32;
  static constexpr u64 FLAG_INFO_LINK = 64;
  static constexpr u64 FLAG_LINK_ORDER = 128;
  static constexpr u64 FLAG_TLS = 1024;
  static constexpr u64 FLAG_MASKOS = 0x0ff00000;
  static constexpr u64 FLAG_MASKPROC = 0xf0000000;

  u32 name;
  u32 type;
  u64 flags;
  offset offset;
  address virtualAddress;
  address physicalAddress;
  u64 size;
  u32 link;
  u32 info;
  u64 alignment;
  u64 entrySize;
};

struct Symbol {
  static constexpr unsigned char TYPE_NOTYPE = 0;
  static constexpr unsigned char TYPE_OBJECT = 1;
  static constexpr unsigned char TYPE_FUNCTION = 2;
  static constexpr unsigned char TYPE_SECTION = 3;
  static constexpr unsigned char TYPE_FILE = 4;
  static constexpr unsigned char TYPE_COMMON = 5;
  static constexpr unsigned char TYPE_TLS = 6;

  static constexpr auto BIND_LOCAL = 0;
  static constexpr auto BIND_GLOBAL = 1;
  static constexpr auto BIND_WEAK = 2;

  static constexpr unsigned char Type(Symbol symbol) {
    return symbol.info >> 0x4;
  }

  static constexpr unsigned char Bind(Symbol symbol) {
    return symbol.info & 0xF;
  }

  static constexpr unsigned char VISIBILITY_DEFAULT = 0;
  static constexpr unsigned char VISIBILITY_INTERNAL = 1;
  static constexpr unsigned char VISIBILITY_HIDDEN = 2;
  static constexpr unsigned char VISIBILITY_PROTECTED = 3;

  static constexpr u16 SECTION_INDEX_UNDEF = 0;

  u32 name;
  unsigned char info;
  unsigned char other;
  u16 sectionHeaderIndex;
  address value;
  u64 size;
};
} // namespace analyzer::elf

#endif
