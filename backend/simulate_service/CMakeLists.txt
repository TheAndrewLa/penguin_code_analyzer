cmake_minimum_required(VERSION 3.20)
project(simulate_service C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Drogon REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR=${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

separate_arguments(LLVM_DEFINITIONS_LIST UNIX_COMMAND "${LLVM_DEFINITIONS}")

if(LLVM_DEFINITIONS_LIST)
  add_compile_options(${LLVM_DEFINITIONS_LIST})
endif()

llvm_map_components_to_libnames(
    LLVM_LIBS
    core
    support
    target
    mc
    mcparser
    mcdisassembler
    object
    binaryformat
    demangle
    mca
    x86info
    x86desc
    x86codegen
    x86asmparser
    x86disassembler
)

add_executable(service
    src/main.cpp
    src/x86_simulation.cpp
    src/x86_simulation.hpp
)

target_include_directories(service PRIVATE src)

target_link_libraries(
    service
    PRIVATE
    Drogon::Drogon
    ${LLVM_LIBS}
    pthread
)
