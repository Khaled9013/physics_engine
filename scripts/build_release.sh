#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBALLISTICS_BUILD_TESTS=OFF
cmake --build build-release --parallel
