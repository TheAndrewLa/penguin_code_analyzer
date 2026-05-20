#ifndef APP_ELF_DATA_SECTION_HPP
#define APP_ELF_DATA_SECTION_HPP

#include <cstddef>
#include <elf/section_data.hpp>
#include <ranges>
#include <string_view>

namespace analyzer::elf {
/// Represents a symbol in an ELF file.
/// A symbol is a named entity that can be located in different sections and can
/// be accessed (read/write) by functions.
struct DataSymbol {
  std::string_view name;

  bool writable;
  bool initialized;

  std::size_t size;
  std::size_t alignment;
};

namespace details {
template <typename T>
class DataSectionBase : public std::ranges::view_interface<T> {
public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = DataSymbol;
    using difference_type = std::ptrdiff_t;

    Iterator() = delete;

    Iterator(const SectionData *first, const SectionData *second,
             std::size_t index)
        : m_first(first), m_second(second), m_index(index) {}

    value_type operator*() const {
      const SectionData *section = m_index == 0 ? m_first : m_second;
      return {section->name(),
              (section->header().flags & SectionHeader::FLAG_WRITE) != 0,
              (section->header().type != SectionHeader::TYPE_NOBITS),
              section->header().size, section->header().alignment};
    }

    Iterator &operator++() {
      ++m_index;
      return *this;
    }

    Iterator operator++(int) {
      auto copy = *this;
      ++copy.m_index;
      return copy;
    }

    bool operator==(const Iterator &other) const = default;

  private:
    const SectionData *m_first{nullptr};
    const SectionData *m_second{nullptr};
    std::size_t m_index{0};
  };

  DataSectionBase(const SectionData *first, const SectionData *second = nullptr)
      : m_first(first), m_second(second),
        m_count((m_first != nullptr ? 1 : 0) + (m_second != nullptr ? 1 : 0)) {}

  Iterator begin() const noexcept { return Iterator{m_first, m_second, 0}; }
  Iterator end() const noexcept { return Iterator{m_first, m_second, m_count}; }

protected:
  DataSectionBase() = default;

  const SectionData *m_first{nullptr};
  const SectionData *m_second{nullptr};
  std::size_t m_count{0};
};
} // namespace details

/// Represents a data section in an ELF file.
/// Contains both initialized (.data) and uninitialized (.bss) data.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class DataSection final : public details::DataSectionBase<DataSection> {
public:
  friend class File;
  using DataSectionBase::DataSectionBase;
};

/// Represents a read-only data section in an ELF file.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class RodataSection final : public details::DataSectionBase<RodataSection> {
public:
  friend class File;
  using DataSectionBase::DataSectionBase;
};

/// Represents a thread-local data section in an ELF file.
/// Contains both initialized (.tdata) and uninitialized (.tbss) data.
/// Implemented as a view of `Symbol`s located in corresponding sections.
class DataSectionTLS final : public details::DataSectionBase<DataSectionTLS> {
public:
  friend class File;
  using DataSectionBase::DataSectionBase;
};
} // namespace analyzer::elf

#endif
