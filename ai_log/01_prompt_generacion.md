# Prompt de generación — variante "IA directa"
Herramienta: Claude Opus 5.5 (claude-opus-5-5), subagente en contexto limpio, sin acceso al repositorio.
Fecha: 2026-10-12

---

Escribe en C++17 una implementación paralela con OpenMP, optimizada para CPU x86-64 multinúcleo, de la siguiente función de convolución 2D de imágenes en escala de grises. Se compilará con g++ (GCC ≥ 14) con los flags `-O2 -fopenmp -std=c++17` (sin -march=native ni -ffast-math) y se medirá su tiempo de ejecución con imágenes de 512×512 a 4096×4096 y kernels de 3×3 a 11×11, usando 1, 2, 4, 8 y 16 hilos.

Encabezado disponible (`convolve.hpp`), no lo modifiques:

```cpp
#pragma once
#include <cstdint>
#include <vector>
using ConvolveFn = void (*)(const uint8_t* in, uint8_t* out, int w, int h,
                            const int32_t* k, int ksize, int nthreads);
namespace ai { void convolve(const uint8_t*, uint8_t*, int, int, const int32_t*, int, int); }
inline uint8_t normalize_pixel(int32_t acc, int32_t sum) {
    int32_t v = (acc + sum / 2) / sum;
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}
```

Semántica obligatoria (el resultado debe ser idéntico bit a bit a una implementación secuencial de referencia):
- `in` y `out` son imágenes de `w` columnas por `h` filas, almacenadas por filas (`in[y*w + x]`), sin solapamiento.
- `k` es un kernel cuadrado de `ksize`×`ksize` pesos enteros no negativos (ksize impar), almacenado por filas; r = ksize/2.
- Para cada píxel (x, y): acc = Σ_{ky=0..ksize-1} Σ_{kx=0..ksize-1} in[y+ky-r][x+kx-r] · k[ky*ksize+kx], acumulando en `int32_t`; los píxeles fuera de la imagen valen 0 (zero-padding).
- out[y*w+x] = normalize_pixel(acc, S), donde S es la suma de todos los pesos de `k`.
- `nthreads` es el número exacto de hilos que debe usar la función.
- Debe funcionar para cualquier w, h ≥ 1, incluso si la imagen es más pequeña que el kernel o hay más hilos que filas.

Responde con un único bloque de código que contenga el archivo completo `ai.cpp` (debe incluir `#include "convolve.hpp"` y definir `ai::convolve`). No incluyas `main`.
