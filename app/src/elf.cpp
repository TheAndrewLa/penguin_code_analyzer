#include <elf.hpp>

#include <llvm/Object/Binary.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

using namespace elf;

using namespace llvm;
using namespace llvm::object;

namespace {
std::unique_ptr<MemoryBuffer> readSymbolBytes(const SymbolRef &sym,
                                              const ObjectFile &obj) {
  uint64_t size = sym.getCommonSize();
  auto sectionIter = obj.section_end();

  if (auto sectionOrError = sym.getSection()) {
    sectionIter = *sectionOrError;
  } else {
    consumeError(sectionOrError.takeError());
    return nullptr;
  }

  if (sectionIter == obj.section_end()) {
    return nullptr;
  }

  const auto &section = *sectionIter;
  auto contentsOrError = section.getContents();

  if (!contentsOrError) {
    consumeError(contentsOrError.takeError());
    return nullptr;
  }

  const auto contents = *contentsOrError;

  auto secData = ArrayRef<std::byte>{
      reinterpret_cast<const std::byte *>(contents.bytes_begin()),
      reinterpret_cast<const std::byte *>(contents.bytes_end())};

  uint64_t symbolValue;

  if (auto valOrErr = sym.getValue()) {
    symbolValue = *valOrErr;
  } else {
    consumeError(valOrErr.takeError());
    return nullptr;
  }

  const auto offset = symbolValue - section.getAddress();

  if (offset + size > secData.size()) {
    return nullptr;
  }

  StringRef symBytes{reinterpret_cast<const char *>(secData.data() + offset),
                     size};

  return MemoryBuffer::getMemBufferCopy(symBytes, sym.getName() ? *sym.getName()
                                                                : "<unnamed>");
}
} // namespace

FileContent elf::readFile(const std::filesystem::path &path) {
  auto bufOrErr = MemoryBuffer::getFile(path.string());

  if (!bufOrErr) {
    throw std::runtime_error{"Cannot open file: " +
                             bufOrErr.getError().message()};
  }

  auto buf = std::move(*bufOrErr);

  auto binaryOrError = createBinary(buf->getMemBufferRef());
  if (!binaryOrError) {
    throw std::runtime_error{toString(binaryOrError.takeError())};
  }

  const auto *obj = dyn_cast<ObjectFile>(binaryOrError->get());

  if (obj == nullptr) {
    throw std::runtime_error("File is not a recognised object file.");
  }

  FileContent result;

  for (const auto &sym : obj->symbols()) {
    // 1. Name
    Expected<StringRef> nameOrErr = sym.getName();
    if (!nameOrErr) {
      consumeError(nameOrErr.takeError());
      continue;
    }
    std::string name = nameOrErr->str();

    // 2. Only global / weak symbols
    auto flagsOrError = sym.getFlags();

    if (!flagsOrError) {
      consumeError(flagsOrError.takeError());
      continue;
    }

    const auto flags = *flagsOrError;
    if ((flags & (SymbolRef::SF_Global | SymbolRef::SF_Weak)) == 0) {
      continue;
    }

    auto typeOrError = sym.getType();

    if (!typeOrError) {
      consumeError(typeOrError.takeError());
      continue;
    }

    const auto type = *typeOrError;

    if (type == SymbolRef::ST_Unknown) {
      continue;
    }

    std::uint64_t addr = 0;

    if (auto a = sym.getAddress()) {
      addr = *a;
    } else {
      consumeError(a.takeError());
    }

    const auto size = sym.getCommonSize();
    const auto alignment = sym.getAlignment();

    if (type == SymbolRef::ST_Function) {
      FunctionSymbol func;
      func.name = std::move(name);
      func.address = addr;
      func.size = size;
      func.bytes = readSymbolBytes(sym, *obj);
      result.functions.push_back(std::move(func));
      continue;
    }

    if (type == SymbolRef::ST_Data) {
      if ((flags & SymbolRef::SF_Common) != 0) {
        VariableSymbol var;
        var.name = std::move(name);
        var.address = addr;
        var.size = size;
        var.alignment = alignment;
        var.isInitialized = false;
        result.variables.push_back(std::move(var));
        continue;
      }

      section_iterator secIt = obj->section_end();

      if (auto secOrErr = sym.getSection()) {
        secIt = *secOrErr;
      } else {
        consumeError(secOrErr.takeError());
        continue;
      }
      if (secIt == obj->section_end())
        continue;

      SectionRef section = *secIt;

      if (section.isBSS()) {
        VariableSymbol var;
        var.name = std::move(name);
        var.address = addr;
        var.size = size;
        var.alignment = alignment;
        var.isInitialized = false;
        result.variables.push_back(std::move(var));
        continue;
      }

      if (section.isData()) {
        VariableSymbol var;
        var.name = std::move(name);
        var.address = addr;
        var.size = size;
        var.alignment = alignment;
        var.isInitialized = true;
        result.variables.push_back(std::move(var));
        continue;
      }
    }
  }

  return result;
}
