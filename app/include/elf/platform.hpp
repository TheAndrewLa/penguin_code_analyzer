#ifndef APP_ELF_PLATFORM_HPP
#define APP_ELF_PLATFORM_HPP

#include <tuple>

namespace analyzer::elf {
enum class Architecture { X86, X86_64, ARM, ARM64, POWERPC };
enum class Endianness { LITTLE, BIG };
enum class ABI { LINUX, FREEBSD, HPUX, SOLARIS, AIX };

/// Represents a platform's architecture, endianness, and ABI.
using Platform = std::tuple<Architecture, Endianness, ABI>;
} // namespace analyzer::elf

#endif
