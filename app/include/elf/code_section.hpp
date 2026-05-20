#ifndef APP_ELF_CODE_SECTION_HPP
#define APP_ELF_CODE_SECTION_HPP

#include <cstddef>
#include <elf/section_data.hpp>
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
  utils::MemoryView code;
};

/// Represents a code section in an ELF file.
/// Contains functions that can be called.
class CodeSection : public std::ranges::view_interface<CodeSection> {
public:
  class Iterator {
  public:
    using value_type = Function;
    using difference_type = std::ptrdiff_t;

    Iterator() = default;
    Iterator(const SectionData *section, std::size_t index) noexcept;

    value_type operator*() const;
    Iterator &operator++();
    Iterator operator++(int) const;
    bool operator==(const Iterator &other) const = default;

  private:
    const SectionData *m_section{nullptr};
    std::size_t m_index{0};
  };

  explicit CodeSection(const SectionData *textSection);

  Iterator begin() const noexcept { return Iterator{m_textSection, 0}; }
  Iterator end() const noexcept { return Iterator{m_textSection, 1}; }

private:
  friend class File;

  CodeSection() = default;

  const SectionData *m_textSection{nullptr};
};
} // namespace analyzer::elf

#endif
