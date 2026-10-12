#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/toolchain.sh
mkdir -p paper/figures
cp results/figures/*.pdf paper/figures/
"$TECTONIC" paper/main.tex
echo "paper/main.pdf built"
