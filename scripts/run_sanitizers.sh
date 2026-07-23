#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build-sanitizers \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBALLISTICS_BUILD_TESTS=ON \
  -DBALLISTICS_ENABLE_SANITIZERS=ON
cmake --build build-sanitizers --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --test-dir build-sanitizers --output-on-failure
