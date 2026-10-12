# AI-Generated C++ Parallel Code Benchmarking

Benchmark de convolución 2D en C++/OpenMP con cuatro variantes: secuencial, IA directa (Claude Opus 5.5),
IA + optimización dirigida por el equipo y una implementación paralela escrita por personas
(`rdb987/2DConvolution`, 2016). Imágenes de 512² a 4096², kernels de 3×3 a 11×11, 1–16 hilos,
30 repeticiones por configuración y validación bit a bit contra la versión secuencial.

Autores: Esteban David Muñoz Bautista, Isabela Gutierrez Reyes — Pontificia Universidad Javeriana Cali.

## Reproducir
1. Instalar GCC (WinLibs): `winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e`
2. Benchmark completo (compila, prueba, registra entorno, mide y analiza, incluida la prueba de Mann-Whitney): `bash scripts/run_bench.sh`
3. Experimento de sensibilidad `-O3 -march=native` (4096²) y su tabla: `bash scripts/run_sensitivity.sh`
4. Prueba de anclaje de hilos (12 configuraciones con 8 hilos): `bash scripts/run_binding.sh`
5. Solo análisis complementario sobre datos existentes: `python scripts/analyze_extra.py`
6. Paper: descargar tectonic 0.15.0 en `tools/` y ejecutar `bash scripts/build_paper.sh`
7. Pruebas: `bash scripts/build.sh && ./build/test_convolve.exe` y `cd scripts && python -m unittest test_analyze`

## Estructura
- `src/` variantes (`seq.cpp`, `ai.cpp`, `ai_opt.cpp`, `manual.cpp`), contrato común y harness (`bench.cpp`)
- `tests/` pruebas de equivalencia bit a bit
- `scripts/` build, ejecución, análisis estadístico y figuras
- `ai_log/` prompt y respuesta de la IA, propuestas de optimización y decisiones del equipo
- `third_party/manual-origin/` código original de la variante manual, origen y diff de adaptación
- `results/` datos crudos, resumen, figuras, entorno, sensibilidad, anclaje y prueba de Mann-Whitney
- `paper/` fuente LaTeX
- `entrega/` PDFs entregables: `paper_v1.pdf`, `evaluacion_inicial.pdf`, `paper_v2.pdf`,
  `evaluacion_final.pdf`, `paper_final.pdf`, `prompt_evaluacion.txt`, `decisiones_evaluacion.md`
- `docs/superpowers/` especificación de diseño y plan de implementación
