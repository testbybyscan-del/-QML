#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-qt5 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-qt5
exec ./build-qt5/loderunner
