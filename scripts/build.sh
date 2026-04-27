#!/bin/bash

set -e

output="build"

conan="./venv/bin/conan"
pip="./venv/bin/pip3"

mkdir -p build

$conan install . --output-folder=$output --build=missing -s build_type=Release
cmake -B $output -DCMAKE_TOOLCHAIN_FILE="$(output)/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build $output --parallel
