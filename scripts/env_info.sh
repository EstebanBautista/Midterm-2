#!/usr/bin/env bash
# Prints the hardware/software environment used for the benchmark.
set -euo pipefail
source "$(dirname "$0")/toolchain.sh"
echo "date: $(date -Iseconds)"
powershell -NoProfile -Command "Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed,L2CacheSize,L3CacheSize | Format-List"
powershell -NoProfile -Command "'RAM_GB: ' + [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory/1GB,1)"
powershell -NoProfile -Command "(Get-CimInstance Win32_OperatingSystem) | Select-Object Caption,Version,BuildNumber | Format-List"
echo "compiler: $("$CXX" --version | head -1)"
echo "flags: -O2 -fopenmp -std=c++17 -static"
echo "python: $(python --version 2>&1)"
python -c "import numpy, matplotlib; print('numpy', numpy.__version__, 'matplotlib', matplotlib.__version__)"
echo "power_plan: $(MSYS_NO_PATHCONV=1 powercfg /getactivescheme)"
echo "OMP_NUM_THREADS=${OMP_NUM_THREADS:-unset} OMP_PROC_BIND=${OMP_PROC_BIND:-unset} OMP_WAIT_POLICY=${OMP_WAIT_POLICY:-unset}"
