#!/bin/bash

set -e

if ! command -v conan &> /dev/null; then
    source ./venv/bin/activate
    echo "Installing conan via pip..."
    pip install conan
fi

conan profile detect || true
conan install . --output-folder=build --build=missing -s build_type=Release
