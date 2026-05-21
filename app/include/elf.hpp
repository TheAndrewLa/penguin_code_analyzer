#ifndef ELF_HPP
#define ELF_HPP

#include <llvm/Support/MemoryBuffer.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace elf {
struct Symbol {
  std::string name;
  std::uint64_t address{0};
  std::uint64_t size{0};
};

struct FunctionSymbol : Symbol {
  std::unique_ptr<llvm::MemoryBuffer> bytes;
};

struct VariableSymbol : Symbol {
  std::uint32_t alignment{0};
  bool isInitialized{false};
};

struct FileContent {
  std::vector<FunctionSymbol> functions;
  std::vector<VariableSymbol> variables;
};

FileContent readFile(const std::filesystem::path &path);
} // namespace elf

#endif
