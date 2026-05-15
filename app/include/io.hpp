#ifndef IO_HPP
#define IO_HPP

#include <types.hpp>

#include <ios>
#include <istream>
#include <stdexcept>
#include <type_traits>

namespace analyzer::io {
template <typename T>
concept readable_structure =
    std::is_trivially_copyable_v<T> && requires(T &value) {
      { &value } noexcept -> std::same_as<T *>;
    };

template <readable_structure T>
inline void ReadStruct(std::istream &file, std::streamoff offset, T &out) {
  file.seekg(offset, std::ios::beg);
  if (file) {
    file.read(reinterpret_cast<char *>(&out),
              static_cast<std::streamsize>(sizeof(T)));
  } else {
    throw std::runtime_error{"Failed to read structure from input stream"};
  }
}

inline std::string ReadString(std::istream &file, std::streamoff offset) {
  std::string result;
  file.seekg(offset, std::ios::beg);

  if (file) {
    for (char character; file.get(character) && character != '\0';) {
      result.push_back(character);
    }
  } else {
    throw std::runtime_error{"Failed to read string from input stream"};
  }

  return result;
}
} // namespace analyzer::io

#endif
