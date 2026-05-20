#ifndef APP_ELF_DATA_SECTION_HPP
#define APP_ELF_DATA_SECTION_HPP

#include <ranges>
#include <string_view>
#include <utils/memory_buffer.hpp>

namespace analyzer::elf {
class File;

/// Represents a symbol in an ELF file.
/// A symbol is a named entity that can be located in different sections and can
/// be accessed (read/write) by functions.
struct Symbol {
  std::string_view name;
  std::string_view type;

  bool writable;
  bool initialized;

  std::size_t size;
  std::size_t alignment;
};

namespace details {
template <typename T>
class DataSectionBase : public std::ranges::view_interface<T> {
  // begin() -> an iterator s.t. *it is a `Symbol`
  // end() -> a sentinel iterator
};
} // namespace details

/// Represents a data section in an ELF file.
/// Contains both initialized (.data) and uninitialized (.bss) data.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class DataSection final : public details::DataSectionBase<DataSection> {};

/// Represents a read-only data section in an ELF file.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class RodataSection final : public details::DataSectionBase<RodataSection> {};

/// Represents a thread-local data section in an ELF file.
/// Contains both initialized (.tdata) and uninitialized (.tbss) data.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class DataSectionTLS final : public details::DataSectionBase<DataSectionTLS> {};
} // namespace analyzer::elf

#endif
