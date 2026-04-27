#!/bin/bash

set -e

conan="./venv/bin/conan"
pip="./venv/bin/pip3"

python3 -m virtualenv venv

$pip install conan
$conan profile detect || true
$conan install . --output-folder=build --build=missing -s build_type=Release
