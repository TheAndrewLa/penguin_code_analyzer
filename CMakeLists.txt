cmake_minimum_required(VERSION 3.10)
project(PenguinCodeAnalyzer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vix CONFIG REQUIRED)
find_package(LLVM REQUIRED CONFIG)

set(APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/app")

file(GLOB UTILS_SRC "${APP_SOURCE_DIR}/utils/*.cpp")
file(GLOB UTILS_INC "${APP_SOURCE_DIR}/include/utils/*.hpp")

file(GLOB ELF_MODULE_SRC "${APP_SOURCE_DIR}/elf/*.cpp")
file(GLOB ELF_MODULE_INC "${APP_SOURCE_DIR}/include/elf/*.hpp")

set(ELF_MODULE ${ELF_MODULE_SRC} ${ELF_MODULE_INC})
set(UTILS_MODULE ${UTILS_SRC} ${UTILS_INC})

add_executable(PenguinCodeAnalyzer "${APP_SOURCE_DIR}/main.cpp")
target_sources(PenguinCodeAnalyzer PRIVATE ${UTILS_MODULE})
target_sources(PenguinCodeAnalyzer PRIVATE ${ELF_MODULE})

target_include_directories(PenguinCodeAnalyzer PRIVATE ${APP_SOURCE_DIR}/include)
target_link_libraries(MyApp PRIVATE vix::vix)
