#!/bin/bash

set -e

mkdir -p build && cd build

conan install .. --output-folder=. --build=missing -s build_type=Release
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
