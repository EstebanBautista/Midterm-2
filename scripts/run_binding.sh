#!/usr/bin/env bash
# Thread-binding check: 12 configurations with 8 threads under OMP_PROC_BIND=close, OMP_PLACES=cores.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh
mkdir -p results/binding_test
OMP_PROC_BIND=close OMP_PLACES=cores ./build/bench.exe --sizes 1024,2048 --ksizes 5,11 --threads 8 --runs 30 \
  --variants ai,ai_opt,manual --out results/binding_test/raw_omp_proc_bind_close.csv
