cmake_minimum_required(VERSION 3.10)
project(PenguinCodeAnalyzer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
  include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

find_package(oatpp REQUIRED)
find_package(oatpp-sqlite REQUIRED)
find_package(LLVM REQUIRED CONFIG)
