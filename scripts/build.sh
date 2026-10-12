#!/usr/bin/env bash
# Builds the benchmark and the test executable with identical flags.
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/toolchain.sh
major="$("$CXX" -dumpversion | cut -d. -f1)"
if [ "$major" -lt 14 ]; then echo "GCC >= 14 required, found $major" >&2; exit 1; fi
FLAGS="-O2 -fopenmp -std=c++17 -static -Wall -Wextra"
mkdir -p build
LIB_SRC=$(ls src/*.cpp | grep -v 'src/bench.cpp' || true)
"$CXX" $FLAGS $LIB_SRC tests/test_convolve.cpp -o build/test_convolve.exe
if [ -f src/bench.cpp ]; then
  "$CXX" $FLAGS $LIB_SRC src/bench.cpp -o build/bench.exe
fi
echo "build OK with $("$CXX" --version | head -1)"
