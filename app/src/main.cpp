#include <elf.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <ELF file>\n";
    return 1;
  }

  try {
    auto elf = elf::readFile(argv[1]);

    std::cout << "Functions (" << elf.functions.size() << "):\n";

    for (const auto &func : elf.functions) {
      std::cout << "  " << func.name << " @0x" << std::hex << func.address
                << " size=0x" << func.size << std::dec;
      if (func.bytes)
        std::cout << " (" << func.bytes->getBufferSize()
                  << " bytes in MemoryBuffer)";
      std::cout << '\n';
    }

    std::cout << "\nVariables (" << elf.variables.size() << "):\n";

    for (const auto &var : elf.variables) {
      std::cout << "  " << var.name << " @0x" << std::hex << var.address
                << " size=0x" << var.size << " align=" << std::dec
                << var.alignment << " init=" << var.isInitialized << '\n';
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
