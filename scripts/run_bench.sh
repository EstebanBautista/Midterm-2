#!/usr/bin/env bash
# Full benchmark: build, test, record environment, run, analyze.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh
./build/test_convolve.exe
mkdir -p results
bash scripts/env_info.sh > results/environment.txt
./build/bench.exe --out results/raw.csv
python scripts/analyze.py results/raw.csv
python scripts/analyze_extra.py
