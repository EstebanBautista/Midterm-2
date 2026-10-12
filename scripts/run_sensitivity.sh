#!/usr/bin/env bash
# Sensitivity experiment: same benchmark on 4096x4096 compiled with -O3 -march=native.
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/toolchain.sh
mkdir -p build results/sensitivity_o3native
LIB_SRC=$(ls src/*.cpp | grep -v 'src/bench.cpp')
"$CXX" -O3 -march=native -fopenmp -std=c++17 -static $LIB_SRC tests/test_convolve.cpp -o build/test_convolve_o3native.exe
./build/test_convolve_o3native.exe
"$CXX" -O3 -march=native -fopenmp -std=c++17 -static $LIB_SRC src/bench.cpp -o build/bench_o3native.exe
./build/bench_o3native.exe --sizes 4096 --out results/sensitivity_o3native/raw.csv
