# Código fuente

Todas las variantes implementan la misma función:

```cpp
void convolve(const uint8_t* in, uint8_t* out, int w, int h,
              const int32_t* k, int ksize, int nthreads);
```

con acumulación en `int32_t`, zero-padding y normalización `clamp((acc + S/2) / S, 0, 255)`.
Al usar aritmética entera, todas deben dar un resultado idéntico bit a bit.

| Archivo | Qué es |
|---|---|
| `convolve.hpp` | Contrato común: firma de las variantes, generadores de kernel e imagen, normalización |
| `common.cpp` | Kernel binomial (triángulo de Pascal) e imagen pseudoaleatoria con semilla fija (xorshift32) |
| `seq.cpp` | Variante secuencial de referencia |
| `ai.cpp` | Variante "IA directa": código generado por Claude Opus 5.5, **sin modificaciones** (ver `ai_log/`) |
| `ai_opt.cpp` | Variante "IA + optimización dirigida por el equipo": `ai.cpp` con las optimizaciones O1 y O4 |
| `manual.cpp` | Variante manual adaptada de `rdb987/2DConvolution` (ver `third_party/manual-origin/`) |
| `variants.hpp`, `register.cpp` | Registro de las variantes disponibles para el benchmark y las pruebas |
| `bench.cpp` | Programa de benchmark: valida contra la secuencial, hace un calentamiento, mide 30 veces y escribe un CSV |

Opciones de `bench.exe`: `--sizes`, `--ksizes`, `--threads`, `--runs`, `--variants`, `--out`
(por ejemplo `./build/bench.exe --sizes 512 --ksizes 3,11 --threads 1,8 --runs 5 --out build/prueba.csv`).
