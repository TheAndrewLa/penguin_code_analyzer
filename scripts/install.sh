#!/bin/bash

set -e

conan="./venv/bin/conan"
pip="./venv/bin/pip3"
curl="curl"

python3 -m virtualenv venv

$curl -fsSL https://vixcpp.com/install.sh | bash

$pip install conan
$conan profile detect || true
$conan install . --output-folder=build --build=missing -s build_type=Release
