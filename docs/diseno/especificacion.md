# Diseño: Benchmarking de código C++ paralelo generado por IA (convolución 2D)

Fecha: 2026-10-12 · Entrega: 2026-10-14 · Idioma del paper: español

## 1. Objetivo

Cuantificar el impacto de la paralelización en una convolución 2D de imágenes en C++ y
comparar la competitividad del código generado por IA frente a código paralelo escrito por
personas, según el enunciado `CL11_04102026.pdf`.

Criterios de éxito:
- Las cuatro variantes producen salidas idénticas bit a bit a la versión secuencial en todas
  las configuraciones.
- Se miden todas las configuraciones (4 tamaños × 4 kernels × 5 conteos de hilos) con
  30 repeticiones cada una.
- Se reportan tiempo (ms), desviación estándar, speedup y eficiencia, con tablas y gráficas.
- El paper en LaTeX (formato Ingeniería y Universidad) compila a PDF y cubre todos los puntos
  que el prompt de evaluación analiza.
- Todo es reproducible desde el repositorio con scripts documentados.

## 2. Decisiones acordadas

| Tema | Decisión |
|---|---|
| Compilador | GCC ≥ 14 de 64 bits nativo en Windows (WinLibs POSIX/UCRT vía winget; hoy publica GCC 16.2), OpenMP. El MinGW 6.3 actual no se usa. La versión exacta se registra en el paper. |
| Variante manual | `rdb987/2DConvolution` (`src/hpc_conv_omp.cpp`, commit `d5b3922`, 2016: anterior a los LLM de código, autoría humana verificable). Aritmética entera y kernel de tamaño arbitrario. Se adapta solo lo necesario (bordes con zero-padding en lugar de periódicos, normalización final, firma común); diff documentado y cita en el paper. El repositorio original no declara licencia: uso académico con cita, en repositorio privado. |
| Variante IA directa | Generada por un subagente de Claude (Opus 5.5) en contexto limpio a partir de un prompt fijo; código usado sin modificaciones. Prompt y respuesta guardados en `ai_log/`. |
| Variante IA + opt. | Nombre en el paper: "IA + optimización dirigida por el equipo". Claude propone optimizaciones justificadas, el equipo decide cuáles aplicar y Claude las implementa. Se describe así en la metodología; nunca se presenta como código manual. |
| Evaluación del paper | La realiza el equipo en ChatGPT o Gemini Pro con el prompt exacto del enunciado (evaluación inicial y final, mismo modelo, chat nuevo cada vez). Claude no evalúa el paper. |
| Reflexión | Sección "Reflexión sobre la evaluación mediante IA" redactada con Claude a partir de las decisiones del equipo sobre cada observación. |
| Paper | LaTeX que reproduce el formato de la plantilla de la revista; compilado localmente con `tectonic` y subido a Overleaf (git si la cuenta es Premium, si no .zip). |

## 3. Datos y problema

- Imágenes: escala de grises, `uint8_t`, tamaños 512², 1024², 2048², 4096², generadas en
  memoria con un PRNG de semilla fija (sin E/S dentro de la medición).
- Kernels: 3×3, 5×5, 7×7, 11×11 de enteros (binomiales, aproximación gaussiana), pesos `int32_t`.
- Cálculo: acumulador `int32_t` por píxel; salida = acumulado / suma del kernel, saturado a
  [0,255]. Con aritmética entera el resultado no depende del orden de suma, lo que garantiza
  equivalencia exacta entre variantes.
- Bordes: zero-padding; la salida tiene el mismo tamaño que la entrada.

## 4. Interfaz común

```cpp
void convolve(const uint8_t* in, uint8_t* out, int w, int h,
              const int32_t* k, int ksize, int nthreads);
```

Cada variante vive en su propio archivo y expone esta función en un namespace propio
(`seq`, `ai`, `ai_opt`, `manual`). La variante secuencial ignora `nthreads`.

## 5. Harness de benchmark (`bench.cpp`)

1. Para cada (tamaño, kernel): generar imagen y kernel; calcular referencia secuencial.
2. Para cada variante y cada p ∈ {1,2,4,8,16} (secuencial solo p=1):
   - Ejecutar una vez y validar `memcmp` contra la referencia; si falla, abortar con error.
   - 1 ejecución de calentamiento descartada.
   - 30 ejecuciones medidas con `std::chrono::steady_clock` solo alrededor de `convolve`.
3. Escribir una fila CSV por ejecución:
   `variant,width,height,ksize,threads,run,time_ms`.

El orden de ejecución de las variantes se intercala por configuración para reducir sesgo por
calentamiento térmico. Todas se compilan con los mismos flags: `-O2 -fopenmp -std=c++17`
(sin `-ffast-math`, sin `-march=native`).

## 6. Análisis (`scripts/analyze.py`, numpy + matplotlib)

- Por configuración: media, desviación estándar, mínimo, máximo, coeficiente de variación.
- Speedup Sp = T_seq / Tp, con T_seq = tiempo medio de la versión secuencial (definición
  principal). También se reporta el speedup relativo a la misma variante con 1 hilo.
- Eficiencia Ep = Sp / p.
- Figuras: tiempo vs. hilos, speedup vs. hilos (con línea ideal), eficiencia vs. hilos, por
  tamaño y kernel; barras de error con desviación estándar.
- Tablas LaTeX generadas automáticamente en `paper/tables/`.

## 7. Control experimental (declarado en el paper)

- CPU Intel Core i7-10700F (8 núcleos físicos, 16 hilos lógicos con HyperThreading);
  se discute que p=16 usa HyperThreading.
- SO Windows 10 Pro 19045, versión exacta de GCC, flags, plan de energía de alto rendimiento,
  aplicaciones cerradas, RAM y versión de Python.
- Script que registra automáticamente el entorno en `results/environment.txt`.

## 8. Estructura del repositorio

```
src/        seq.cpp, ai.cpp, ai_opt.cpp, manual.cpp, convolve.hpp, bench.cpp
ai_log/     prompt de generación, respuesta completa de la IA, propuestas de optimización
third_party/manual-origin/  código original de GitHub + LICENSE + diff de adaptación
scripts/    build.sh, run_bench.sh, analyze.py, env_info.sh
results/    raw.csv, summary.csv, figuras PNG/PDF, environment.txt
paper/      main.tex, figuras, tablas, referencias
entrega/    paper_v1.pdf, prompt_evaluacion.txt, evaluacion_inicial.pdf,
            evaluacion_final.pdf, paper_final.pdf
README.md   instrucciones de reproducción
```

## 9. Flujo de entrega y puntos de intervención del equipo

1. Claude: código, validación, benchmark, análisis, paper v1 en PDF.
2. **Equipo:** evaluación inicial en ChatGPT/Gemini con el prompt del enunciado; guardar PDF.
3. Equipo + Claude: decidir observación por observación (aceptar/rechazar con justificación);
   Claude aplica cambios y genera el paper v2.
4. **Equipo:** evaluación final con la misma IA en un chat nuevo; guardar PDF.
5. Equipo + Claude: sección de reflexión (puntajes inicial y final, recomendaciones
   rechazadas y por qué); paper final.

Claude avisa de forma explícita cuándo le toca al equipo en los pasos 2 y 4.

## 10. Riesgos

- Plazo corto (2 días): el paper v1 debe estar listo el 13 temprano para dejar margen a las
  evaluaciones.
- Ruido de medición en Windows: mitigado con calentamiento, 30 repeticiones, ejecución
  intercalada y reporte de desviación estándar y CV.
- Variante manual con tipos distintos (p. ej. `float`): se adapta a enteros y el cambio se
  documenta como amenaza a la validez.
- La plantilla oficial de la revista es Word: el LaTeX la imita; se revisa contra la guía de
  autores antes de la entrega.
