#include "x86_graph.hpp"

#include <iterator>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/TargetRegistry.h>

#include <llvm/Object/Binary.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>

#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
void InitializeLLVM() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86Disassembler();
  });
}

llvm::Error MakeError(const llvm::Twine &message) { return llvm::createStringError(message); }

class GraphBuildError : public std::exception {
public:
  explicit GraphBuildError(std::string what) : what_(std::move(what)) {}
  explicit GraphBuildError(const char *what) : what_(what) {}
  const char *what() const noexcept override { return what_.c_str(); }

private:
  std::string what_;
};

template <typename T> void Assert(T &&value, const std::string &message) {
  if (!(value)) {
    throw GraphBuildError{message};
  }
}

enum EdgeFlag : std::uint8_t {
  Conditional = 1U << 0U,
  Fallthrough = 1U << 1U,
  Taken = 1U << 2U,
};

std::string formatAddress(std::uint64_t address) { return (std::ostringstream{} << "0x" << std::hex << address).str(); }

std::string trimmed(const std::string &text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(" \t\r\n");
  return text.substr(begin, end - begin + 1);
}

std::uint64_t symbolSize(const llvm::object::SymbolRef &sym, const llvm::object::ObjectFile &object) {
  if (llvm::isa<llvm::object::ELFObjectFileBase>(&object)) {
    return llvm::object::ELFSymbolRef(sym).getSize();
  }
  return 0;
}

std::vector<std::uint8_t> readSymbolBytes(const llvm::object::SymbolRef &sym, const llvm::object::ObjectFile &object,
                                          std::uint64_t address, std::uint64_t size) {
  auto sectionOrError = sym.getSection();

  if (!sectionOrError) {
    llvm::consumeError(sectionOrError.takeError());
    return {};
  }

  auto sectionIter = *sectionOrError;

  if (sectionIter == object.section_end()) {
    return {};
  }

  const auto &section = *sectionIter;
  auto contentsOrError = section.getContents();

  if (!contentsOrError) {
    llvm::consumeError(contentsOrError.takeError());
    return {};
  }

  const auto contents = *contentsOrError;

  if (address < section.getAddress()) {
    return {};
  }

  const auto offset = address - section.getAddress();

  if (offset >= contents.size()) {
    return {};
  }

  const auto available = std::min<std::uint64_t>(size, contents.size() - offset);
  return {contents.bytes_begin() + offset, contents.bytes_begin() + offset + available};
}
} // namespace

X86GraphBuilder::X86GraphBuilder() {
  InitializeLLVM();

  llvm::StringRef tripleName(DefaultTargetTriple);
  std::string error;

  target_ = llvm::TargetRegistry::lookupTarget(tripleName, error);

  if (target_ == nullptr) {
    llvm::report_fatal_error("Can not find x86-64 target!");
  }

  regInfo_.reset(target_->createMCRegInfo(tripleName));
  Assert(regInfo_, "Can not create `MCRegisterInfo`!");

  llvm::MCTargetOptions options;

  asmInfo_.reset(target_->createMCAsmInfo(*regInfo_, tripleName, options));
  Assert(asmInfo_, "Can not create `MCAsmInfo`!");

  instrInfo_.reset(target_->createMCInstrInfo());
  Assert(instrInfo_, "Can not create `MCInstrInfo`!");
}

X86GraphBuilder::~X86GraphBuilder() = default;

std::string X86GraphBuilder::newSession() {
  const auto time = std::chrono::steady_clock::now().time_since_epoch();
  return std::to_string(time.count());
}

X86GraphBuilder::BaseResult X86GraphBuilder::start(const std::byte *bytes, std::size_t lenght) {
  try {
    Assert(bytes != nullptr, "Provided buffer is null!");
    Assert(lenght > 0, "Provided buffer is empty!");

    auto session = std::make_unique<Session>();
    session->buffer =
        llvm::MemoryBuffer::getMemBufferCopy(llvm::StringRef(reinterpret_cast<const char *>(bytes), lenght), "object");

    auto binaryOrError = llvm::object::createBinary(session->buffer->getMemBufferRef());

    if (!binaryOrError) {
      return MakeError("Can not parse object file: " + llvm::toString(binaryOrError.takeError()));
    }

    auto *object = llvm::dyn_cast<llvm::object::ObjectFile>(binaryOrError->get());

    if (object == nullptr) {
      return MakeError("Provided buffer is not an object file!");
    }

    binaryOrError->release();
    session->object = std::unique_ptr<llvm::object::ObjectFile>(object);

    parseFunctions(*session, *object);

    const auto sessionId = newSession();

    std::vector<std::string> names;
    names.reserve(session->functions.size());
    for (const auto &[name, unused] : session->functions) {
      (void)unused;
      names.push_back(name);
    }
    std::sort(names.begin(), names.end());

    sessions_.emplace(sessionId, std::move(session));

    return BaseInfo{std::move(names), sessionId};
  } catch (const GraphBuildError &error) {
    return MakeError(error.what());
  } catch (...) {
    return MakeError("An error occured!");
  }
}

void X86GraphBuilder::end(const std::string &session) { sessions_.erase(session); }

X86GraphBuilder::JsonResult X86GraphBuilder::getFunctionGraph(const std::string &session, const std::string &name) {
  auto iter = sessions_.find(session);

  if (iter == sessions_.end()) {
    return MakeError("Can not find session!");
  }

  auto functionIter = iter->second->functions.find(name);

  if (functionIter == iter->second->functions.end()) {
    return MakeError("Can not find function '" + name + "'!");
  }

  try {
    return buildGraph(functionIter->second);
  } catch (const GraphBuildError &error) {
    return MakeError(error.what());
  } catch (...) {
    return MakeError("An error occured!");
  }
}

void X86GraphBuilder::parseFunctions(Session &session, const llvm::object::ObjectFile &object) const {
  for (const auto &sym : object.symbols()) {
    if (auto function = extractFunction(sym, object)) {
      session.functions.emplace(function->name, std::move(*function));
    }
  }
}

std::optional<X86GraphBuilder::Function>
X86GraphBuilder::extractFunction(const llvm::object::SymbolRef &sym, const llvm::object::ObjectFile &object) const {
  auto nameOrError = sym.getName();

  if (!nameOrError) {
    llvm::consumeError(nameOrError.takeError());
    return std::nullopt;
  }

  auto typeOrError = sym.getType();

  if (!typeOrError) {
    llvm::consumeError(typeOrError.takeError());
    return std::nullopt;
  }

  if (*typeOrError != llvm::object::SymbolRef::ST_Function) {
    return std::nullopt;
  }

  auto addressOrError = sym.getAddress();

  if (!addressOrError) {
    llvm::consumeError(addressOrError.takeError());
    return std::nullopt;
  }

  Function function;
  function.name = nameOrError->str();
  function.address = *addressOrError;
  function.size = symbolSize(sym, object);
  function.bytes = readSymbolBytes(sym, object, function.address, function.size);

  disassemble(function);

  return function;
}

void X86GraphBuilder::disassemble(Function &function) const {
  if (function.bytes.empty()) {
    return;
  }

  auto subtargetInfo =
      std::unique_ptr<llvm::MCSubtargetInfo>(target_->createMCSubtargetInfo(DefaultTargetTriple, "", ""));
  Assert(subtargetInfo, "Can not create `MCSubtargetInfo`!");

  auto instrAnalysis = std::unique_ptr<llvm::MCInstrAnalysis>(target_->createMCInstrAnalysis(instrInfo_.get()));
  Assert(instrAnalysis, "Can not create `MCInstrAnalysis`!");

  auto ctx = llvm::MCContext(llvm::Triple(DefaultTargetTriple), asmInfo_.get(), regInfo_.get(), subtargetInfo.get());

  auto disassembler = std::unique_ptr<llvm::MCDisassembler>(target_->createMCDisassembler(*subtargetInfo, ctx));
  Assert(disassembler, "Can not create `MCDisassembler`!");

  auto printer = std::unique_ptr<llvm::MCInstPrinter>(
      target_->createMCInstPrinter(llvm::Triple(DefaultTargetTriple), 0, *asmInfo_, *instrInfo_, *regInfo_));
  Assert(printer, "Can not create `MCInstPrinter`!");

  auto progamCounter = function.address;
  auto offset = std::size_t(0);

  while (offset < function.bytes.size()) {
    llvm::MCInst inst;
    std::uint64_t size = 0;

    const auto status = disassembler->getInstruction(
        inst, size, llvm::ArrayRef<std::uint8_t>(function.bytes.data() + offset, function.bytes.size() - offset),
        progamCounter, llvm::nulls());

    if (status != llvm::MCDisassembler::Success || size == 0) {
      break;
    }

    Instruction instruction;
    instruction.address = progamCounter;
    instruction.isBranch = instrAnalysis->isBranch(inst);
    instruction.isCall = instrAnalysis->isCall(inst);
    instruction.isReturn = instrAnalysis->isReturn(inst);

    if (instruction.isBranch) {
      instruction.isConditional = instrAnalysis->isConditionalBranch(inst);
      instruction.isUnconditional = instrAnalysis->isUnconditionalBranch(inst);
      instruction.isIndirect = instrAnalysis->isIndirectBranch(inst);

      if (!instruction.isIndirect && instrAnalysis->evaluateBranch(inst, progamCounter, size, instruction.target)) {
        instruction.hasTarget = true;
      }
    }

    std::string text;
    llvm::raw_string_ostream ostream(text);
    printer->printInst(&inst, progamCounter, "", *subtargetInfo, ostream);
    ostream.flush();
    instruction.text = trimmed(text);

    function.instructions.push_back(std::move(instruction));

    progamCounter += size;
    offset += static_cast<std::size_t>(size);
  }
}

Json::Value X86GraphBuilder::buildGraph(const Function &function) {
  const auto index = buildAddressIndex(function);
  const auto leaders = findLeaders(function, index);
  auto blocks = makeBlocks(function, index, leaders);
  connectBlocks(function, blocks);
  return serializeBlocks(function, blocks);
}

X86GraphBuilder::AddressIndex X86GraphBuilder::buildAddressIndex(const Function &function) {
  const auto &instructions = function.instructions;

  AddressIndex index;
  index.reserve(instructions.size());

  for (std::size_t i = 0; i < instructions.size(); ++i) {
    index.emplace(instructions[i].address, i);
  }

  return index;
}

std::set<std::uint64_t> X86GraphBuilder::findLeaders(const Function &function, const AddressIndex &index) {
  const auto &instructions = function.instructions;

  std::set<std::uint64_t> leaders;

  if (instructions.empty()) {
    return leaders;
  }

  leaders.insert(instructions.front().address);

  for (const auto &instruction : instructions) {
    if (instruction.hasTarget && (instruction.isBranch || instruction.isReturn) &&
        index.find(instruction.target) != index.end()) {
      leaders.insert(instruction.target);
    }
  }

  for (std::size_t i = 0; i + 1 < instructions.size(); ++i) {
    if (instructions[i].isBranch || instructions[i].isReturn) {
      leaders.insert(instructions[i + 1].address);
    }
  }

  return leaders;
}

std::vector<X86GraphBuilder::Block> X86GraphBuilder::makeBlocks(const Function &function, const AddressIndex &index,
                                                                const std::set<std::uint64_t> &leaders) {
  const auto &instructions = function.instructions;

  std::vector<Block> blocks;
  blocks.reserve(leaders.size());

  std::size_t label = 0;

  for (auto it = leaders.begin(); it != leaders.end(); ++it) {
    Block block;
    block.start = *it;
    block.begin = index.at(*it);

    auto next = std::next(it);
    block.end = (next == leaders.end()) ? instructions.size() : index.at(*next);

    block.id = formatAddress(block.start);
    block.label = "L" + std::to_string(++label);

    blocks.push_back(std::move(block));
  }

  return blocks;
}

void X86GraphBuilder::connectBlocks(const Function &function, std::vector<Block> &blocks) {
  std::unordered_map<std::uint64_t, std::string> blockIds;

  blockIds.reserve(blocks.size());

  std::for_each(blocks.cbegin(), blocks.cend(),
                [&blockIds](const auto &block) { blockIds.emplace(block.start, block.id); });

  std::for_each(blocks.begin(), blocks.end(),
                [&blockIds, &function](auto &block) { connectBlock(function.instructions, block, blockIds); });
}

void X86GraphBuilder::connectBlock(const std::vector<Instruction> &instructions, Block &block,
                                   const std::unordered_map<std::uint64_t, std::string> &blockIds) {
  if (block.begin == block.end) {
    return;
  }

  const auto &last = instructions[block.end - 1];

  if (last.isReturn || (last.isBranch && !last.hasTarget)) {
    return;
  }

  if (last.isBranch) {
    connectBranch(block, last, blockIds, instructions);
  } else {
    connectFallthrough(block, blockIds, instructions, Fallthrough);
  }
}

void X86GraphBuilder::connectBranch(Block &block, const Instruction &branch,
                                    const std::unordered_map<std::uint64_t, std::string> &blockIds,
                                    const std::vector<Instruction> &instructions) {
  addEdge(block, findBlockId(blockIds, branch.target), branch.isConditional ? (Conditional | Taken) : 0);

  if (!branch.isUnconditional) {
    connectFallthrough(block, blockIds, instructions, branch.isConditional ? (Conditional | Fallthrough) : Fallthrough);
  }
}

void X86GraphBuilder::connectFallthrough(Block &block, const std::unordered_map<std::uint64_t, std::string> &blockIds,
                                         const std::vector<Instruction> &instructions, std::uint8_t flags) {
  if (block.end >= instructions.size()) {
    return;
  }

  addEdge(block, findBlockId(blockIds, instructions[block.end].address), flags);
}

void X86GraphBuilder::addEdge(Block &block, std::string targetId, std::uint8_t flags) {
  if (!targetId.empty()) {
    block.edges.emplace_back(std::move(targetId), flags);
  }
}

std::string X86GraphBuilder::findBlockId(const std::unordered_map<std::uint64_t, std::string> &blockIds,
                                         std::uint64_t address) {
  const auto iter = blockIds.find(address);
  return (iter == blockIds.end()) ? std::string{} : iter->second;
}

Json::Value X86GraphBuilder::serializeBlocks(const Function &function, const std::vector<Block> &blocks) {
  auto nodes = Json::Value(Json::arrayValue);

  for (const auto &block : blocks) {
    nodes.append(serializeBlock(function, block));
  }

  auto result = Json::Value(Json::objectValue);
  result["nodes"] = std::move(nodes);

  return result;
}

Json::Value X86GraphBuilder::serializeBlock(const Function &function, const Block &block) {
  const auto &instructions = function.instructions;

  auto node = Json::Value(Json::objectValue);
  node["id"] = block.id;
  node["label"] = block.label;

  auto instructionList = Json::Value(Json::arrayValue);

  for (std::size_t i = block.begin; i < block.end; ++i) {
    instructionList.append(instructions[i].text);
  }

  node["instructions"] = std::move(instructionList);

  auto edges = Json::Value(Json::arrayValue);

  for (const auto &[to, flags] : block.edges) {
    edges.append(serializeEdge(to, flags));
  }

  node["edgesOut"] = std::move(edges);

  return node;
}

Json::Value X86GraphBuilder::serializeEdge(const std::string &to, std::uint8_t flags) {
  auto edge = Json::Value(Json::objectValue);
  edge["to"] = to;

  if ((flags & Conditional) != 0) {
    edge["conditional"] = true;
  }
  if ((flags & Fallthrough) != 0) {
    edge["fallthrough"] = true;
  }
  if ((flags & Taken) != 0) {
    edge["taken"] = true;
  }

  return edge;
}
