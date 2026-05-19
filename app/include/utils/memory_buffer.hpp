#ifndef APP_UTILS_MEMORY_BUFFER_HPP
#define APP_UTILS_MEMORY_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <istream>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>

namespace analyzer::utils {
class MemoryView;

template <typename T>
concept MemoryBufferWriteType = std::is_trivially_copy_assignable_v<T> &&
                                std::is_trivially_destructible_v<T>;

template <typename T>
concept MemoryBufferReadType = std::is_trivially_copy_assignable_v<T>;

class MemoryBuffer {
public:
  MemoryBuffer() = delete;

  explicit MemoryBuffer(std::size_t size);
  explicit MemoryBuffer(std::istream &stream);

  template <std::ranges::sized_range R>
    requires MemoryBufferWriteType<std::ranges::range_value_t<R>>
  explicit MemoryBuffer(R &&range) {
    using T = std::ranges::range_value_t<R>;
    m_size = std::ranges::size(std::forward<R>(range)) * sizeof(T);
    m_data = std::make_unique<std::byte[]>(m_size);
    std::ranges::copy(std::forward<R>(range),
                      reinterpret_cast<T *>(m_data.get()));
  }

  template <MemoryBufferWriteType T>
  MemoryBuffer(const T *source, std::size_t count)
      : MemoryBuffer(std::views::counted(source, count)) {}

  template <MemoryBufferWriteType T>
  void writeStruct(std::size_t offset, const T &value) {
    if (offset + sizeof(T) > m_size) {
      throw std::out_of_range{"Failed to write struct to memory buffer"};
    }
    auto *ptr = std::addressof(m_data[offset]);
    *reinterpret_cast<T *>(ptr) = value;
  }

  template <MemoryBufferReadType T>
  void readStruct(std::size_t offset, T &result) const {
    if (offset + sizeof(T) > m_size) {
      throw std::out_of_range{"Failed to read struct from memory buffer"};
    }
    const auto *ptr = std::addressof(m_data[offset]);
    result = *reinterpret_cast<const T *>(ptr);
  }

  void writeString(std::size_t offset, std::string_view string);

  std::string readString(std::size_t offset) const;
  std::string readString(std::size_t offset, std::size_t length) const;

  MemoryView toView(std::size_t offset) const;
  MemoryView toView(std::size_t offset, std::size_t length) const;

private:
  std::unique_ptr<std::byte[]> m_data;
  std::size_t m_size;
};

class MemoryView : public std::ranges::view_interface<MemoryView> {
public:
  MemoryView() = default;

  const std::byte *begin() const noexcept { return m_begin; }
  const std::byte *end() const noexcept { return m_end; }

private:
  friend class MemoryBuffer;

  MemoryView(std::byte *begin, std::byte *end);

  const std::byte *m_begin{nullptr};
  const std::byte *m_end{nullptr};
};
} // namespace analyzer::utils

#endif
