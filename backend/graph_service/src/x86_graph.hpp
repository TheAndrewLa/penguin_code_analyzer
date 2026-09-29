#ifndef X86_GRAPH_HPP
#define X86_GRAPH_HPP

#include <json/json.h>

#include <llvm/Support/Error.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {
class MCAsmInfo;
class MCInstrInfo;
class MCRegisterInfo;
class MemoryBuffer;
class Target;
namespace object {
class ObjectFile;
class SymbolRef;
} // namespace object
} // namespace llvm

class X86GraphBuilder {
public:
  struct BaseInfo {
    std::vector<std::string> functions;
    std::string session;
  };

  using JsonResult = llvm::Expected<Json::Value>;
  using BaseResult = llvm::Expected<BaseInfo>;

  X86GraphBuilder();
  X86GraphBuilder(const X86GraphBuilder &) = delete;
  X86GraphBuilder(X86GraphBuilder &&) = delete;

  ~X86GraphBuilder();

  BaseResult start(const std::byte *bytes, std::size_t lenght);
  void end(const std::string &session);

  JsonResult getFunctionGraph(const std::string &session, const std::string &name);

private:
  static constexpr auto DefaultTargetTriple = "x86_64-unknown-linux-gnu";

  struct Instruction {
    std::uint64_t address{0};
    std::string text;
    bool isBranch{false};
    bool isConditional{false};
    bool isUnconditional{false};
    bool isIndirect{false};
    bool isCall{false};
    bool isReturn{false};
    bool hasTarget{false};
    std::uint64_t target{0};
  };

  struct Function {
    std::string name;
    std::uint64_t address{0};
    std::uint64_t size{0};
    std::vector<std::uint8_t> bytes;
    std::vector<Instruction> instructions;
  };

  struct Session {
    std::unique_ptr<llvm::MemoryBuffer> buffer;
    std::unique_ptr<llvm::object::ObjectFile> object;
    std::unordered_map<std::string, Function> functions;
  };

  struct Block {
    std::uint64_t start{0};
    std::size_t begin{0};
    std::size_t end{0};
    std::string id;
    std::string label;
    std::vector<std::pair<std::string, std::uint8_t>> edges;
  };

  using AddressIndex = std::unordered_map<std::uint64_t, std::size_t>;

  static std::string newSession();

  void parseFunctions(Session &session, const llvm::object::ObjectFile &object) const;
  std::optional<Function> extractFunction(const llvm::object::SymbolRef &sym,
                                          const llvm::object::ObjectFile &object) const;

  void disassemble(Function &function) const;

  static Json::Value buildGraph(const Function &function);

  static void addEdge(Block &block, std::string targetId, std::uint8_t flags);
  static std::string findBlockId(const std::unordered_map<std::uint64_t, std::string> &blockIds, std::uint64_t address);

  static AddressIndex buildAddressIndex(const Function &function);
  static std::set<std::uint64_t> findLeaders(const Function &function, const AddressIndex &index);
  static std::vector<Block> makeBlocks(const Function &function, const AddressIndex &index,
                                       const std::set<std::uint64_t> &leaders);

  static void connectBlocks(const Function &function, std::vector<Block> &blocks);
  static void connectBlock(const std::vector<Instruction> &instructions, Block &block,
                           const std::unordered_map<std::uint64_t, std::string> &blockIds);
  static void connectBranch(Block &block, const Instruction &branch,
                            const std::unordered_map<std::uint64_t, std::string> &blockIds,
                            const std::vector<Instruction> &instructions);
  static void connectFallthrough(Block &block, const std::unordered_map<std::uint64_t, std::string> &blockIds,
                                 const std::vector<Instruction> &instructions, std::uint8_t flags);

  static Json::Value serializeBlocks(const Function &function, const std::vector<Block> &blocks);
  static Json::Value serializeBlock(const Function &function, const Block &block);
  static Json::Value serializeEdge(const std::string &to, std::uint8_t flags);

  const llvm::Target *target_;

  std::unique_ptr<llvm::MCRegisterInfo> regInfo_;
  std::unique_ptr<llvm::MCAsmInfo> asmInfo_;
  std::unique_ptr<llvm::MCInstrInfo> instrInfo_;

  std::unordered_map<std::string, std::unique_ptr<Session>> sessions_;
};

#endif // X86_GRAPH_HPP
