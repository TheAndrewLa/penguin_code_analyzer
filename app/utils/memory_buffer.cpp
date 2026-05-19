#include <ranges>
#include <utils/memory_buffer.hpp>

#include <vector>

using namespace analyzer;

analyzer::utils::MemoryBuffer::MemoryBuffer(std::size_t size)
    : m_data(std::make_unique<std::byte[]>(size)), m_size(size) {}

analyzer::utils::MemoryBuffer::MemoryBuffer(std::istream &stream) {
  std::vector<char> bytes{std::istreambuf_iterator<char>{stream},
                          std::istreambuf_iterator<char>{}};
  auto bytes_view = bytes | std::views::transform([](char c) {
                      return static_cast<std::byte>(c);
                    });
  m_size = std::ranges::size(bytes_view);
  m_data = std::make_unique<std::byte[]>(m_size);
  std::ranges::copy(bytes_view, m_data.get());
}

void analyzer::utils::MemoryBuffer::writeString(std::size_t offset,
                                                std::string_view string) {
  if (offset + string.size() > m_size) {
    throw std::runtime_error{"Failed to write string to memory buffer"};
  }
  auto *dest = m_data.get() + offset;
  auto view = string | std::views::transform(
                           [](char c) { return static_cast<std::byte>(c); });
  std::ranges::copy(view, dest);
}

std::string
analyzer::utils::MemoryBuffer::readString(std::size_t offset) const {
  std::string result;
  const auto *begin = m_data.get() + offset;
  const auto *end = m_data.get() + m_size;
  const auto range = std::ranges::subrange{begin, end};
  std::ranges::copy(range | std::views::take_while([](std::byte b) {
                      return b != std::byte{0};
                    }) | std::views::transform([](std::byte b) {
                      return static_cast<char>(b);
                    }),
                    std::back_inserter(result));
  return result;
}

std::string
analyzer::utils::MemoryBuffer::readString(std::size_t offset,
                                          std::size_t length) const {
  if (offset + length > m_size) {
    throw std::runtime_error{"Failed to read string from memory buffer"};
  }
  std::string result;
  const auto *data = m_data.get() + offset;
  const auto range = std::ranges::subrange{data, data + length};
  std::ranges::copy(range | std::views::transform([](std::byte byte) {
                      return static_cast<char>(byte);
                    }),
                    std::back_inserter(result));
  return result;
}

utils::MemoryView
analyzer::utils::MemoryBuffer::toView(std::size_t offset) const {
  if (offset >= m_size) {
    throw std::runtime_error{"Failed to create view from memory buffer"};
  }
  auto *begin = m_data.get() + offset;
  auto *end = m_data.get() + m_size;
  return {begin, end};
}

utils::MemoryView
analyzer::utils::MemoryBuffer::toView(std::size_t offset,
                                      std::size_t length) const {
  if (offset + length > m_size) {
    throw std::runtime_error{"Failed to create view from memory buffer"};
  }
  auto *begin = m_data.get() + offset;
  return {begin, begin + length};
}

analyzer::utils::MemoryView::MemoryView(std::byte *begin, std::byte *end)
    : m_begin(begin), m_end(end) {}
