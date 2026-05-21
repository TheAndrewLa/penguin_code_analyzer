cmake_minimum_required(VERSION 3.10)
project(PenguinCodeAnalyzer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vix CONFIG REQUIRED)
find_package(LLVM REQUIRED CONFIG)

set(APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/app/src")
set(APP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/app/include")

file(GLOB APP_SOURCES "${APP_SOURCE_DIR}/*.cpp")

llvm_map_components_to_libnames(LLVM_LIBRARIES Object Support)

add_executable(PenguinCodeAnalyzer ${APP_SOURCES})
target_include_directories(PenguinCodeAnalyzer PRIVATE ${APP_INCLUDE_DIR})
target_include_directories(PenguinCodeAnalyzer PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(PenguinCodeAnalyzer PRIVATE vix::vix)
target_link_libraries(PenguinCodeAnalyzer PRIVATE ${LLVM_LIBRARIES})
