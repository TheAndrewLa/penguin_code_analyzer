#ifndef APP_ELF_DATA_SECTION_HPP
#define APP_ELF_DATA_SECTION_HPP

#include <elf/section.hpp>

#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

namespace analyzer::elf {
class DataSection : public ElfSection {
public:
  struct GlobalVariable {
    std::string name;
    std::string type;
    std::uint64_t size{};
    std::uint64_t alignment{};
  };

  using Globals = std::vector<GlobalVariable>;

  DataSection(std::string name, std::size_t size);
  ~DataSection() override = default;

  void setGlobals(Globals globals);
  auto globals() const -> std::ranges::subrange<Globals::const_iterator>;

private:
  Globals m_globals;
};

class InitializedDataSection final : public DataSection {
public:
  explicit InitializedDataSection(std::size_t size);
};

class UninitializedDataSection final : public DataSection {
public:
  explicit UninitializedDataSection(std::size_t size);
};
} // namespace analyzer::elf

#endif
