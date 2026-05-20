#ifndef APP_ELF_CODE_SECTION_HPP
#define APP_ELF_CODE_SECTION_HPP

#include <ranges>
#include <string_view>
#include <utils/memory_buffer.hpp>

namespace analyzer::elf {
class File;

/// Represents a function in an ELF file.
/// A function is a named entity that is located in the `.text` section and can
/// be called.
struct Function {
  std::string_view name;
  std::string_view signature;
  utils::MemoryView code;
};

/// Represents a code section in an ELF file.
/// Contains functions that can be called.
class CodeSection : public std::ranges::view_interface<CodeSection> {
public:
  // begin() -> an iterator s.t. *it is a `Function`
  // end() -> a sentinel iterator
};
} // namespace analyzer::elf

#endif
