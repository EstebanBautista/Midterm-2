# Scripts

| Archivo | Qué hace |
|---|---|
| `toolchain.sh` | Localiza el g++ de WinLibs y tectonic (se usa con `source`, no se ejecuta directamente) |
| `build.sh` | Compila `build/bench.exe` y `build/test_convolve.exe` con `-O2 -fopenmp -std=c++17 -static` |
| `env_info.sh` | Imprime el entorno de hardware y software (va a `results/environment.txt`) |
| `run_bench.sh` | Experimento completo: compila, prueba, registra el entorno, mide y analiza |
| `run_sensitivity.sh` | Experimento de sensibilidad con `-O3 -march=native` en 4096² |
| `run_binding.sh` | Prueba de anclaje de hilos con `OMP_PROC_BIND=close` |
| `analyze.py` | Estadísticas, figuras y tablas LaTeX a partir de `results/raw.csv` |
| `analyze_extra.py` | Prueba de Mann-Whitney y tabla de sensibilidad |
| `test_analyze.py` | Pruebas unitarias del análisis (`cd scripts && python -m unittest test_analyze`) |
| `build_paper.sh` | Compila `paper/main.tex` a PDF con tectonic |
