#include <utils/memory_buffer.hpp>

#include <vector>

analyzer::utils::MemoryBuffer::MemoryBuffer(std::size_t size)
    : m_data(std::make_unique<std::byte[]>(size)), m_size(size) {}

analyzer::utils::MemoryBuffer::MemoryBuffer(std::istream &stream) {
  std::vector<std::byte> bytes;
  while (!stream.eof()) {
    char c;
    stream.get(c);
    bytes.push_back(static_cast<std::byte>(c));
  }
  m_size = bytes.size();
  m_data = std::unique_ptr<std::byte[]>(std::move(bytes).data());
}

void analyzer::utils::MemoryBuffer::readString(std::size_t offset,
                                               std::string &result) const {
  for (std::size_t i = offset; m_data[i] != std::byte{0}; ++i) {
    if (i >= m_size) {
      throw std::runtime_error{"Failed to read string from memory buffer"};
    }
    result.push_back(static_cast<char>(m_data[i]));
  }
}

void analyzer::utils::MemoryBuffer::readString(std::size_t offset,
                                               std::string &result,
                                               std::size_t length) const {
  for (std::size_t i = 0; i < length; ++i) {
    if (i >= m_size) {
      throw std::runtime_error{"Failed to read string from memory buffer"};
    }
    result.push_back(static_cast<char>(m_data[offset + i]));
  }
}

std::string
analyzer::utils::MemoryBuffer::readString(std::size_t offset) const {
  std::string result;
  readString(offset, result);
  return result;
}

std::string
analyzer::utils::MemoryBuffer::readString(std::size_t offset,
                                          std::size_t length) const {
  std::string result;
  readString(offset, result, length);
  return result;
}
