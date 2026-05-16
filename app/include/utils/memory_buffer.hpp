#ifndef APP_UTILS_MEMORY_BUFFER_HPP
#define APP_UTILS_MEMORY_BUFFER_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <istream>
#include <memory>
#include <stdexcept>

namespace analyzer::utils {
class MemoryBuffer {
public:
  explicit MemoryBuffer(std::size_t size);
  explicit MemoryBuffer(std::istream &stream);

  template <std::copyable T>
  MemoryBuffer(const T *source, std::size_t count)
      : m_data(std::make_unique<std::byte[]>(sizeof(T) * count)),
        m_size(sizeof(T) * count) {
    auto *dest = reinterpret_cast<T *>(m_data.get());
    std::copy_n(source, count, dest);
  }

  template <std::copyable T>
  void readStruct(std::size_t offset, T &result) const {
    if (offset + sizeof(T) >= m_size) {
      throw std::runtime_error{"Failed to read struct from memory buffer"};
    }
    result = *reinterpret_cast<const T *>(&m_data[offset]);
  }

  void readString(std::size_t offset, std::string &result) const;
  void readString(std::size_t offset, std::string &result,
                  std::size_t length) const;

  std::string readString(std::size_t offset) const;
  std::string readString(std::size_t offset, std::size_t length) const;

private:
  std::unique_ptr<std::byte[]> m_data;
  std::size_t m_size;
};
} // namespace analyzer::utils

#endif
