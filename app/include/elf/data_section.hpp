#ifndef APP_ELF_DATA_SECTION_HPP
#define APP_ELF_DATA_SECTION_HPP

#include <elf/section.hpp>
#include <types.hpp>

#include <ranges>
#include <string>
#include <vector>

namespace analyzer::elf {
class ElfParser;

class DataSection : public ElfSection {
public:
  struct GlobalVariable {
    std::string name;
    std::string type;
    u64 size{};
    u64 alignment{};
  };

  using Globals = std::vector<GlobalVariable>;
  using GlobalsRange = std::ranges::subrange<Globals::const_iterator>;

  DataSection(std::string_view name, usize size);
  ~DataSection() override = default;

  GlobalsRange globals() const;

private:
  friend class ElfParser;

  void setGlobals(Globals &&globals);

  Globals m_globals;
};

class InitializedDataSection final : public DataSection {
public:
  explicit InitializedDataSection(usize size);
};

class UninitializedDataSection final : public DataSection {
public:
  explicit UninitializedDataSection(usize size);
};
} // namespace analyzer::elf

#endif
