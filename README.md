# Benchmarking de código C++ paralelo generado por IA

Convolución 2D de imágenes en C++/OpenMP: comparación de rendimiento entre una versión secuencial,
código paralelo generado por IA, código de IA optimizado bajo dirección del equipo y una implementación
paralela escrita por personas.

**Autores:** Esteban David Muñoz Bautista e Isabela Gutierrez Reyes — Pontificia Universidad Javeriana Cali,
Facultad de Ingeniería, Ingeniería de Sistemas y Computación.
**Enunciado:** [`CL11_04102026.pdf`](CL11_04102026.pdf)

## Guía rápida

| Si quieres… | Ve a |
|---|---|
| Leer el artículo final | [`entrega/paper_final.pdf`](entrega/paper_final.pdf) |
| Ver todos los entregables (versiones del paper, prompt y evaluaciones de la IA) | [`entrega/`](entrega/) |
| Ver cómo se generó el código con IA (prompt y respuesta literal) | [`ai_log/`](ai_log/) |
| Revisar el código de las cuatro variantes | [`src/`](src/) |
| Ver los datos y las figuras | [`results/`](results/) |
| Reproducir el experimento | [Reproducir](#reproducir) |

## Resultados principales (imagen 4096×4096)

- **IA directa:** speedup de 36,9 a 61,8 frente a la versión secuencial con kernels de 3×3 a 7×7; solo 12,7 con 11×11,
  porque su vectorización SSE2 se desactiva cuando los pesos no caben en 16 bits con signo.
- **IA + optimización dirigida por el equipo:** corrige ese caso y llega a 48,0 con 11×11.
- **Manual (GitHub, 2016):** paralelización directa sin optimizaciones de bajo nivel; speedup de 5,8 a 6,7.
- **Escalabilidad por hilos** (respecto a cada variante con 1 hilo): similar en todas, de 6 a 10 veces con 16 hilos.
  La ventaja de la IA viene de la eficiencia por núcleo (SIMD), no de un mejor uso de los hilos.
- Todas las variantes producen resultados **idénticos bit a bit** a la versión secuencial.
- Evaluación del artículo con Gemini Pro: **80/100** (versión inicial) → **88/100** (versión corregida).

## Las cuatro variantes

| Variante | Archivo | Origen |
|---|---|---|
| Secuencial | [`src/seq.cpp`](src/seq.cpp) | Referencia de tiempo y de corrección |
| IA directa | [`src/ai.cpp`](src/ai.cpp) | Generada por Claude Opus 5.5 y usada sin modificaciones |
| IA + optimización dirigida por el equipo | [`src/ai_opt.cpp`](src/ai_opt.cpp) | Copia de `ai.cpp` con las optimizaciones O1 y O4 aprobadas por el equipo |
| Manual (GitHub) | [`src/manual.cpp`](src/manual.cpp) | Adaptada de [`rdb987/2DConvolution`](https://github.com/rdb987/2DConvolution) (2016) |

## Estructura del repositorio

```
.
├── CL11_04102026.pdf        Enunciado del trabajo
├── entrega/                 Entregables finales (PDFs del paper y de las evaluaciones con IA)
├── paper/                   Fuente LaTeX del artículo (main.tex, referencias, figuras y tablas)
├── src/                     Código C++: las 4 variantes, el contrato común y el programa de benchmark
├── tests/                   Pruebas de equivalencia bit a bit de todas las variantes
├── scripts/                 Compilación, ejecución del benchmark, análisis estadístico y generación del paper
├── results/                 Datos crudos, resúmenes estadísticos, figuras y descripción del entorno
├── ai_log/                  Registro de la interacción con la IA y decisiones del equipo
├── third_party/
│   └── manual-origin/       Código original de la variante manual, su origen y el diff de adaptación
└── docs/diseno/             Especificación de diseño y plan de implementación del proyecto
```

Cada carpeta principal tiene su propio `README.md` con el detalle de sus archivos.

## Reproducir

Requisitos: Windows con Git Bash, Python 3.12 con numpy y matplotlib.

1. Instalar GCC (WinLibs): `winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e`
2. Benchmark completo (compila, prueba, registra el entorno, mide y analiza; unos 30–60 min): `bash scripts/run_bench.sh`
3. Experimento de sensibilidad con `-O3 -march=native` (4096²): `bash scripts/run_sensitivity.sh`
4. Prueba de anclaje de hilos (12 configuraciones con 8 hilos): `bash scripts/run_binding.sh`
5. Solo el análisis complementario sobre datos existentes: `python scripts/analyze_extra.py`
6. Paper: descargar [tectonic 0.15.0](https://github.com/tectonic-typesetting/tectonic/releases/tag/tectonic%400.15.0) en `tools/` y ejecutar `bash scripts/build_paper.sh`
7. Pruebas: `bash scripts/build.sh && ./build/test_convolve.exe` y `cd scripts && python -m unittest test_analyze`

## Uso de inteligencia artificial

La variante "IA directa" fue generada por Claude Opus 5.5. El mismo asistente ayudó a implementar las
optimizaciones aprobadas por el equipo, a preparar los scripts de medición y análisis y a redactar el artículo.
La evaluación crítica del artículo se hizo con Gemini Pro, un modelo de otra empresa, para reducir el sesgo de
autoevaluación. Los detalles están en el artículo (secciones "Declaración de uso de inteligencia artificial" y
"Reflexión sobre la evaluación mediante IA").
