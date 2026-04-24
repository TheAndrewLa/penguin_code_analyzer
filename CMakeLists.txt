cmake_minimum_required(VERSION 3.10)
project(PenguinCodeAnalyzer CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(userver COMPONENTS core REQUIRED)
