#!/usr/bin/env bash
# Locates the WinLibs g++ and tectonic; source this file, do not execute it.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LAD="$(cygpath -u "$LOCALAPPDATA")"
WL_BIN="$(ls -d "$LAD"/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT*/mingw64/bin 2>/dev/null | head -1)"
if [ -z "$WL_BIN" ]; then
  echo "WinLibs not found; run: winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e" >&2
  return 1
fi
export CXX="$WL_BIN/g++.exe"
export TECTONIC="$ROOT/tools/tectonic.exe"
