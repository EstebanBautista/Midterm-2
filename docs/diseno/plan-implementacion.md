# Benchmark de convolución 2D paralela generada por IA — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Medir tiempo, speedup y eficiencia de cuatro variantes C++ de convolución 2D (secuencial, IA directa, IA + optimización dirigida por el equipo, manual de GitHub) y producir el paper en LaTeX con su ciclo de evaluación por IA.

**Architecture:** Cada variante implementa la misma función `convolve` en su propio archivo y namespace; un registro (`register.cpp`) las expone a un harness de benchmark y a un ejecutable de pruebas que exige igualdad bit a bit con la versión secuencial. El benchmark escribe un CSV crudo; un script Python calcula estadísticas, figuras y tablas LaTeX que el paper incluye directamente.

**Tech Stack:** C++17 + OpenMP, GCC ≥ 14 (WinLibs POSIX/UCRT, enlace estático), Git Bash, Python 3.12 (numpy, matplotlib, unittest), LaTeX compilado con tectonic, Overleaf para edición en equipo.

**Spec:** `docs/diseno/especificacion.md`

## Global Constraints

- Flags idénticos para todo el código medido: `-O2 -fopenmp -std=c++17 -static` (sin `-ffast-math`, sin `-march=native`).
- Compilador: GCC con versión mayor ≥ 14; `scripts/build.sh` aborta si no.
- Firma común: `void convolve(const uint8_t* in, uint8_t* out, int w, int h, const int32_t* k, int ksize, int nthreads);`
- Semántica: acumulador `int32_t`, zero-padding, salida `clamp((acc + sum/2) / sum, 0, 255)` con `sum` = suma de pesos del kernel.
- Configuraciones: tamaños 512, 1024, 2048, 4096 (cuadradas); kernels 3, 5, 7, 11; hilos 1, 2, 4, 8, 16; 30 repeticiones + 1 calentamiento descartado; la secuencial solo con 1 hilo.
- Speedup principal `Sp = T_seq / Tp`; eficiencia `Ep = Sp / p`; desviación estándar muestral (ddof = 1).
- Paper en español, LaTeX, Times 12 pt, una columna, justificado, ecuaciones numeradas, referencias IEEE, máximo 5 400 palabras incluyendo referencias, resumen estructurado ≤ 200 palabras.
- La variante IA + opt. se llama en el paper "IA + optimización dirigida por el equipo"; nunca se presenta como manual.
- La evaluación del paper la hace el equipo en ChatGPT o Gemini; Claude no evalúa el paper.
- Nada se sube a GitHub (`git push`) sin confirmación explícita del usuario en ese momento.

## Review Focus

- Imagen más pequeña que el kernel (p. ej. 5×3 con kernel 11×11): todas las variantes deben dar lo mismo que la secuencial, sin leer fuera de la memoria. Test en Task 2 (`test_equivalence`, forma 5×3).
- Más hilos que filas (16 hilos sobre 3 filas): sin crash, sin píxeles sin escribir. Test en Task 2 (buffer de salida pre-llenado con `0xAB`).
- Dimensiones no múltiplo del número de hilos ni cuadradas (37×29, 129×7): igualdad bit a bit. Test en Task 2.
- Saturación (imagen toda en 255 con kernel 11×11): el interior vale 255, sin overflow de `int32_t`. Test en Task 2 (`test_saturation`).
- Speedup referido a la secuencial cuando falta alguna configuración en el CSV: el análisis debe fallar con un mensaje claro, no producir NaN silenciosos. Test en Task 7 (`test_missing_seq_raises`).

---

### Task 1: Toolchain moderno y registro de entorno

**Files:**
- Create: `scripts/toolchain.sh`, `scripts/env_info.sh`, `.gitignore`

**Interfaces:**
- Produces: `source scripts/toolchain.sh` exporta `CXX` (ruta a g++ de WinLibs) y `TECTONIC` (ruta a tectonic.exe). `bash scripts/env_info.sh > results/environment.txt`.

- [ ] **Step 1: Instalar WinLibs (GCC POSIX/UCRT, 64 bits)**

Run (PowerShell):
```powershell
winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e --accept-package-agreements --accept-source-agreements
```
Expected: "Successfully installed". No modifica el MinGW 6.3 existente.

- [ ] **Step 2: Descargar tectonic a `tools/` (ignorado por git)**

```bash
mkdir -p tools && cd tools
curl -L -o tectonic.zip "https://github.com/tectonic-typesetting/tectonic/releases/download/tectonic%400.15.0/tectonic-0.15.0-x86_64-pc-windows-msvc.zip"
unzip -o tectonic.zip && rm tectonic.zip && ./tectonic.exe --version
```
Expected: `Tectonic 0.15.0`.

- [ ] **Step 3: Crear `scripts/toolchain.sh`**

```bash
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
```

- [ ] **Step 4: Crear `scripts/env_info.sh`**

```bash
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
echo "power_plan: $(powercfg /getactivescheme)"
echo "OMP_NUM_THREADS=${OMP_NUM_THREADS:-unset} OMP_PROC_BIND=${OMP_PROC_BIND:-unset} OMP_WAIT_POLICY=${OMP_WAIT_POLICY:-unset}"
```

- [ ] **Step 5: Crear `.gitignore`**

```
build/
tools/
__pycache__/
*.aux
*.log
*.out
```

- [ ] **Step 6: Verificar**

Run: `source scripts/toolchain.sh && "$CXX" -dumpversion && bash scripts/env_info.sh`
Expected: versión mayor ≥ 14; CPU `Intel(R) Core(TM) i7-10700F`, 8 núcleos, 16 lógicos.

- [ ] **Step 7: Commit**

```bash
git add scripts/toolchain.sh scripts/env_info.sh .gitignore
git commit -m "build: add WinLibs/tectonic toolchain locator and environment report"
```

---

### Task 2: Núcleo común, variante secuencial, registro, pruebas y build

**Files:**
- Create: `src/convolve.hpp`, `src/common.cpp`, `src/seq.cpp`, `src/variants.hpp`, `src/register.cpp`, `tests/test_convolve.cpp`, `scripts/build.sh`

**Interfaces:**
- Produces:
  - `std::vector<int32_t> make_binomial_kernel(int ksize)` (ksize impar ≥ 1, fila-mayor `k[i*ksize+j]`).
  - `std::vector<uint8_t> make_image(int w, int h, uint32_t seed)`.
  - `inline uint8_t normalize_pixel(int32_t acc, int32_t sum)`.
  - `namespace seq { void convolve(...); }` (firma común).
  - `struct Variant { const char* name; ConvolveFn fn; };` y `const std::vector<Variant>& variants();` en `variants.hpp`; `register.cpp` es el único lugar donde se listan.
  - `bash scripts/build.sh` → `build/bench.exe` (si existe `src/bench.cpp`) y `build/test_convolve.exe`.

- [ ] **Step 1: Escribir `src/convolve.hpp`**

```cpp
#pragma once
#include <cstdint>
#include <vector>

// Common contract for every variant:
//   out[y*w+x] = clamp((sum_{ky,kx} in[y+ky-r][x+kx-r] * k[ky*ksize+kx] + S/2) / S, 0, 255)
// with r = ksize/2, S = sum of kernel weights, zero padding outside the image,
// int32_t accumulation. Results are bit-exact regardless of summation order.
using ConvolveFn = void (*)(const uint8_t* in, uint8_t* out, int w, int h,
                            const int32_t* k, int ksize, int nthreads);

namespace seq    { void convolve(const uint8_t*, uint8_t*, int, int, const int32_t*, int, int); }
namespace ai     { void convolve(const uint8_t*, uint8_t*, int, int, const int32_t*, int, int); }
namespace ai_opt { void convolve(const uint8_t*, uint8_t*, int, int, const int32_t*, int, int); }
namespace manual { void convolve(const uint8_t*, uint8_t*, int, int, const int32_t*, int, int); }

std::vector<int32_t> make_binomial_kernel(int ksize);
std::vector<uint8_t> make_image(int w, int h, uint32_t seed);

inline uint8_t normalize_pixel(int32_t acc, int32_t sum) {
    int32_t v = (acc + sum / 2) / sum;
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}
```

- [ ] **Step 2: Escribir `src/variants.hpp`**

```cpp
#pragma once
#include "convolve.hpp"
#include <vector>

struct Variant {
    const char* name;
    ConvolveFn fn;
};

// Registered variants, "seq" always first. Defined in register.cpp.
const std::vector<Variant>& variants();
```

- [ ] **Step 3: Escribir el test que falla `tests/test_convolve.cpp`**

```cpp
#include "../src/convolve.hpp"
#include "../src/variants.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            ++failures;                                               \
            std::printf("FAIL %s:%d: ", __FILE__, __LINE__);          \
            std::printf(__VA_ARGS__);                                 \
            std::printf("\n");                                        \
        }                                                             \
    } while (0)

static void test_kernel() {
    auto k3 = make_binomial_kernel(3);
    const int32_t e3[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
    CHECK(k3.size() == 9 && std::memcmp(k3.data(), e3, sizeof e3) == 0, "binomial 3x3");
    auto k1 = make_binomial_kernel(1);
    CHECK(k1.size() == 1 && k1[0] == 1, "binomial 1x1");
    long long s = 0;
    for (int32_t v : make_binomial_kernel(11)) s += v;
    CHECK(s == 1048576LL, "sum of 11x11 kernel = %lld, expected 1048576", s);
}

static void test_seq_known_values() {
    std::vector<uint8_t> in(9, 100), out(9, 0);
    auto k = make_binomial_kernel(3);
    seq::convolve(in.data(), out.data(), 3, 3, k.data(), 3, 1);
    CHECK(out[4] == 100, "center = %d, expected 100", out[4]);
    CHECK(out[0] == 56, "corner = %d, expected 56", out[0]);   // (900+8)/16
    CHECK(out[1] == 75, "edge = %d, expected 75", out[1]);     // (1200+8)/16
}

static void test_identity_kernel() {
    auto img = make_image(37, 29, 7u);
    auto k = make_binomial_kernel(1);
    for (const Variant& v : variants()) {
        std::vector<uint8_t> out(img.size(), 0xAB);
        v.fn(img.data(), out.data(), 37, 29, k.data(), 1, 4);
        CHECK(out == img, "%s: 1x1 kernel must be identity", v.name);
    }
}

static void test_equivalence() {
    const int shapes[][2] = {{1, 1}, {5, 3}, {37, 29}, {64, 64}, {129, 7}};
    const int ksizes[] = {3, 5, 7, 11};
    const int threads[] = {1, 2, 3, 4, 8, 16};
    for (auto& s : shapes) {
        int w = s[0], h = s[1];
        auto img = make_image(w, h, 1000u + w * 31u + h);
        for (int ks : ksizes) {
            auto k = make_binomial_kernel(ks);
            std::vector<uint8_t> ref(img.size());
            seq::convolve(img.data(), ref.data(), w, h, k.data(), ks, 1);
            for (const Variant& v : variants()) {
                for (int p : threads) {
                    std::vector<uint8_t> out(img.size(), 0xAB);  // detects unwritten pixels
                    v.fn(img.data(), out.data(), w, h, k.data(), ks, p);
                    CHECK(out == ref, "%s differs from seq: w=%d h=%d k=%d p=%d", v.name, w, h, ks, p);
                }
            }
        }
    }
}

static void test_saturation() {
    std::vector<uint8_t> img(64 * 64, 255);
    auto k = make_binomial_kernel(11);
    for (const Variant& v : variants()) {
        std::vector<uint8_t> out(img.size(), 0);
        v.fn(img.data(), out.data(), 64, 64, k.data(), 11, 8);
        CHECK(out[32 * 64 + 32] == 255, "%s: saturated interior = %d", v.name, out[32 * 64 + 32]);
    }
}

int main() {
    test_kernel();
    test_seq_known_values();
    test_identity_kernel();
    test_equivalence();
    test_saturation();
    std::printf("variants:");
    for (const Variant& v : variants()) std::printf(" %s", v.name);
    std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
```

- [ ] **Step 4: Escribir `scripts/build.sh`**

```bash
#!/usr/bin/env bash
# Builds the benchmark and the test executable with identical flags.
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/toolchain.sh
major="$("$CXX" -dumpversion | cut -d. -f1)"
if [ "$major" -lt 14 ]; then echo "GCC >= 14 required, found $major" >&2; exit 1; fi
FLAGS="-O2 -fopenmp -std=c++17 -static -Wall -Wextra"
mkdir -p build
LIB_SRC=$(ls src/*.cpp | grep -v 'src/bench.cpp')
"$CXX" $FLAGS $LIB_SRC tests/test_convolve.cpp -o build/test_convolve.exe
if [ -f src/bench.cpp ]; then
  "$CXX" $FLAGS $LIB_SRC src/bench.cpp -o build/bench.exe
fi
echo "build OK with $("$CXX" --version | head -1)"
```

- [ ] **Step 5: Ejecutar build para ver el fallo**

Run: `bash scripts/build.sh`
Expected: FAIL en el enlace con `undefined reference to 'variants()'` / `make_binomial_kernel`.

- [ ] **Step 6: Escribir `src/common.cpp`**

```cpp
#include "convolve.hpp"

std::vector<int32_t> make_binomial_kernel(int ksize) {
    std::vector<int32_t> row(ksize, 0);
    row[0] = 1;
    for (int n = 1; n < ksize; ++n)
        for (int j = n; j > 0; --j) row[j] += row[j - 1];
    std::vector<int32_t> k(static_cast<size_t>(ksize) * ksize);
    for (int i = 0; i < ksize; ++i)
        for (int j = 0; j < ksize; ++j) k[i * ksize + j] = row[i] * row[j];
    return k;
}

std::vector<uint8_t> make_image(int w, int h, uint32_t seed) {
    std::vector<uint8_t> img(static_cast<size_t>(w) * h);
    uint32_t s = seed ? seed : 1u;
    for (auto& px : img) {  // xorshift32: deterministic across platforms
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        px = static_cast<uint8_t>(s >> 24);
    }
    return img;
}
```

- [ ] **Step 7: Escribir `src/seq.cpp`**

```cpp
#include "convolve.hpp"

namespace seq {

void convolve(const uint8_t* in, uint8_t* out, int w, int h,
              const int32_t* k, int ksize, int /*nthreads*/) {
    const int r = ksize / 2;
    int32_t sum = 0;
    for (int i = 0; i < ksize * ksize; ++i) sum += k[i];
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int32_t acc = 0;
            for (int ky = 0; ky < ksize; ++ky) {
                const int yy = y + ky - r;
                if (yy < 0 || yy >= h) continue;
                for (int kx = 0; kx < ksize; ++kx) {
                    const int xx = x + kx - r;
                    if (xx < 0 || xx >= w) continue;
                    acc += in[yy * w + xx] * k[ky * ksize + kx];
                }
            }
            out[y * w + x] = normalize_pixel(acc, sum);
        }
    }
}

}  // namespace seq
```

- [ ] **Step 8: Escribir `src/register.cpp`**

```cpp
#include "variants.hpp"

const std::vector<Variant>& variants() {
    static const std::vector<Variant> v = {
        {"seq", seq::convolve},
    };
    return v;
}
```

- [ ] **Step 9: Compilar y correr pruebas**

Run: `bash scripts/build.sh && ./build/test_convolve.exe`
Expected: `variants: seq` y `OK (0 failures)`.

- [ ] **Step 10: Commit**

```bash
git add src tests scripts/build.sh
git commit -m "feat: add common convolution contract, sequential baseline and equivalence tests"
```

---

### Task 3: Harness de benchmark

**Files:**
- Create: `src/bench.cpp`

**Interfaces:**
- Consumes: `variants()`, `make_image`, `make_binomial_kernel`, `seq::convolve`.
- Produces: `build/bench.exe [--sizes a,b] [--ksizes a,b] [--threads a,b] [--runs n] [--variants n1,n2] [--out path]`; CSV con cabecera `variant,width,height,ksize,threads,run,time_ms`. Código de salida 2 si una validación falla, 1 si hay argumentos inválidos.

- [ ] **Step 1: Escribir `src/bench.cpp`**

```cpp
#include "convolve.hpp"
#include "variants.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string t;
    while (std::getline(ss, t, ',')) if (!t.empty()) out.push_back(t);
    return out;
}

static std::vector<int> parse_ints(const std::string& s) {
    std::vector<int> v;
    for (auto& t : split(s)) v.push_back(std::stoi(t));
    return v;
}

int main(int argc, char** argv) {
    std::vector<int> sizes{512, 1024, 2048, 4096}, ksizes{3, 5, 7, 11}, threads{1, 2, 4, 8, 16};
    int runs = 30;
    std::string out_path = "results/raw.csv";
    std::vector<std::string> names;
    for (const Variant& v : variants()) names.push_back(v.name);

    for (int i = 1; i < argc; i += 2) {
        std::string a = argv[i];
        if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", a.c_str()); return 1; }
        std::string b = argv[i + 1];
        if (a == "--sizes") sizes = parse_ints(b);
        else if (a == "--ksizes") ksizes = parse_ints(b);
        else if (a == "--threads") threads = parse_ints(b);
        else if (a == "--runs") runs = std::stoi(b);
        else if (a == "--variants") names = split(b);
        else if (a == "--out") out_path = b;
        else { std::fprintf(stderr, "unknown argument %s\n", a.c_str()); return 1; }
    }

    std::vector<const Variant*> selected;
    for (auto& n : names) {
        auto it = std::find_if(variants().begin(), variants().end(),
                               [&](const Variant& v) { return n == v.name; });
        if (it == variants().end()) { std::fprintf(stderr, "unknown variant %s\n", n.c_str()); return 1; }
        selected.push_back(&*it);
    }

    FILE* f = std::fopen(out_path.c_str(), "w");
    if (!f) { std::perror(out_path.c_str()); return 1; }
    std::fprintf(f, "variant,width,height,ksize,threads,run,time_ms\n");

    using clock = std::chrono::steady_clock;
    int config = 0;
    for (int n : sizes) {
        auto img = make_image(n, n, 42u + static_cast<uint32_t>(n));
        std::vector<uint8_t> ref(img.size()), buf(img.size());
        for (int ks : ksizes) {
            auto k = make_binomial_kernel(ks);
            seq::convolve(img.data(), ref.data(), n, n, k.data(), ks, 1);
            for (int p : threads) {
                std::vector<const Variant*> order;
                for (auto* v : selected)
                    if (p == 1 || std::strcmp(v->name, "seq") != 0) order.push_back(v);
                if (order.empty()) continue;
                // Rotate the execution order per configuration to spread warm-up/thermal bias.
                std::rotate(order.begin(), order.begin() + (config++ % order.size()), order.end());
                for (auto* v : order) {
                    std::memset(buf.data(), 0xAB, buf.size());
                    v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);
                    if (buf != ref) {
                        std::fprintf(stderr, "VALIDATION FAILED: %s n=%d k=%d p=%d\n", v->name, n, ks, p);
                        std::fclose(f);
                        return 2;
                    }
                    v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);  // warm-up, discarded
                    for (int r = 0; r < runs; ++r) {
                        auto t0 = clock::now();
                        v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);
                        auto t1 = clock::now();
                        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                        std::fprintf(f, "%s,%d,%d,%d,%d,%d,%.6f\n", v->name, n, n, ks, p, r, ms);
                    }
                    std::fflush(f);
                    std::fprintf(stderr, "done %-7s n=%-5d k=%-2d p=%d\n", v->name, n, ks, p);
                }
            }
        }
    }
    std::fclose(f);
    return 0;
}
```

- [ ] **Step 2: Compilar y prueba corta**

Run:
```bash
bash scripts/build.sh && mkdir -p results && ./build/bench.exe --sizes 64,128 --ksizes 3,11 --threads 1,2 --runs 3 --out build/smoke.csv && head -5 build/smoke.csv && wc -l build/smoke.csv
```
Expected: cabecera correcta; 2 tamaños × 2 kernels × 1 variante (seq solo p=1) × 3 = 12 filas + cabecera = 13 líneas.

- [ ] **Step 3: Verificar errores de argumentos**

Run: `./build/bench.exe --variants nope; echo "exit=$?"`
Expected: `unknown variant nope` y `exit=1`.

- [ ] **Step 4: Commit**

```bash
git add src/bench.cpp
git commit -m "feat: add benchmark harness with validation, warm-up and CSV output"
```

---

### Task 4: Variante "IA directa" generada en contexto limpio

**Files:**
- Create: `ai_log/01_prompt_generacion.md`, `ai_log/02_respuesta_ia.md`, `src/ai.cpp`
- Modify: `src/register.cpp`

**Interfaces:**
- Produces: `namespace ai { void convolve(...); }` registrado como `"ai"`.

- [ ] **Step 1: Guardar el prompt exacto en `ai_log/01_prompt_generacion.md`**

Contenido (texto literal que recibirá la IA, más un encabezado con modelo y fecha):

````markdown
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
````

- [ ] **Step 2: Generar el código con un subagente en contexto limpio**

Lanzar un `Agent` (`subagent_type: general-purpose`, `model: opus`) cuyo `prompt` sea exactamente el texto entre las líneas `---` y el final del bloque anterior, precedido de: "No leas ni uses ningún archivo del disco; responde solo con texto." Guardar la respuesta completa, sin editar, en `ai_log/02_respuesta_ia.md` con encabezado de fecha/hora.

- [ ] **Step 3: Extraer el código tal cual a `src/ai.cpp`**

Copiar el bloque de código de la respuesta sin cambios. Si el subagente redefine `normalize_pixel` o el encabezado, se conserva; si eso rompe la compilación por redefinición, se anota en `ai_log/03_notas.md` y se considera fallo de la IA (ver Step 6).

- [ ] **Step 4: Registrar la variante en `src/register.cpp`**

```cpp
    static const std::vector<Variant> v = {
        {"seq", seq::convolve},
        {"ai", ai::convolve},
    };
```

- [ ] **Step 5: Compilar y probar**

Run: `bash scripts/build.sh && ./build/test_convolve.exe`
Expected: `variants: seq ai` y `OK (0 failures)`.

- [ ] **Step 6: Si falla (compilación o equivalencia)**

No se corrige `src/ai.cpp`. Registrar en `ai_log/03_notas.md` el error exacto (salida del compilador o líneas `FAIL`). Repetir Steps 2–5 una sola vez con un subagente nuevo y el mismo prompt, guardando la respuesta en `ai_log/02b_respuesta_ia_intento2.md`. Ambos intentos se reportan en el paper (tasa de éxito al primer intento es un resultado). Si el segundo intento también falla, detener la ejecución y consultar al usuario.

- [ ] **Step 7: Commit**

```bash
git add ai_log src/ai.cpp src/register.cpp
git commit -m "feat: add AI-generated parallel variant (clean-context generation, logged prompt and response)"
```

---

### Task 5: Variante manual (rdb987/2DConvolution)

**Files:**
- Create: `third_party/manual-origin/hpc_conv_omp.cpp`, `third_party/manual-origin/conv_openmp.h`, `third_party/manual-origin/SOURCE.md`, `src/manual.cpp`, `third_party/manual-origin/adaptacion.diff`
- Modify: `src/register.cpp`

**Interfaces:**
- Produces: `namespace manual { void convolve(...); }` registrado como `"manual"`.

- [ ] **Step 1: Descargar el original fijado a un commit**

```bash
mkdir -p third_party/manual-origin && cd third_party/manual-origin
SHA=d5b3922e47c11ce12fa1bd954023cdfee2fa46d9
curl -sSfLO "https://raw.githubusercontent.com/rdb987/2DConvolution/$SHA/src/hpc_conv_omp.cpp"
curl -sSfLO "https://raw.githubusercontent.com/rdb987/2DConvolution/$SHA/include/conv_openmp.h"
```

- [ ] **Step 2: Escribir `third_party/manual-origin/SOURCE.md`**

```markdown
# Origen de la variante manual

- Repositorio: https://github.com/rdb987/2DConvolution
- Autor: rdb987 (GitHub). Repositorio creado 2016-02-15; último commit 2016-03-01.
- Commit fijado: d5b3922e47c11ce12fa1bd954023cdfee2fa46d9
- Archivo: src/hpc_conv_omp.cpp (método `Convolutionmp::Compute`)
- Licencia: el repositorio no declara licencia. Uso académico, no comercial, con cita, en repositorio privado.
- Motivo de elección: escrito antes de la disponibilidad de asistentes de código basados en LLM, aritmética entera,
  kernel de tamaño arbitrario y paralelización solo con OpenMP.

## Adaptaciones (ver adaptacion.diff)
1. Firma común `manual::convolve` en lugar de la clase `Convolutionmp` con `std::vector<int>`.
2. Bordes: el original usa bordes periódicos (wrap-around); se cambia a zero-padding para cumplir el contrato común.
3. Normalización final con `normalize_pixel` (el original deja la suma cruda en un `int`).
4. Número de hilos con `num_threads(nthreads)`; el original usa el valor por defecto del runtime.
5. Se elimina la medición interna (`stopwatch`) y la escritura a archivo.
Se conservan: el bucle aplanado `z = 0..rows*columns-1`, el cálculo `x = z/columns, y = z%columns`,
el orden de los bucles del kernel y el `#pragma omp for` con `schedule` por defecto.
```

- [ ] **Step 3: Escribir `src/manual.cpp`**

```cpp
// Adapted from rdb987/2DConvolution, src/hpc_conv_omp.cpp, commit d5b3922 (2016).
// See third_party/manual-origin/SOURCE.md for the list of adaptations.
#include "convolve.hpp"
#include <omp.h>

namespace manual {

void convolve(const uint8_t* input, uint8_t* out, int columns, int rows,
              const int32_t* kernel, int krows, int nthreads) {
    const int kcolumns = krows;
    int32_t sum = 0;
    for (int i = 0; i < krows * kcolumns; ++i) sum += kernel[i];

    int a, b, x, y;
    #pragma omp parallel num_threads(nthreads)
    #pragma omp for private(x, y, a, b)
    for (int z = 0; z < rows * columns; ++z) {
        x = z / columns;
        y = z % columns;
        int32_t acc = 0;
        for (int i = 0; i < krows; ++i) {
            for (int j = 0; j < kcolumns; ++j) {
                a = x + i - krows / 2;
                b = y + j - kcolumns / 2;
                if (a < 0 || a > rows - 1 || b < 0 || b > columns - 1)
                    continue;  // adaptation: zero padding instead of wrap-around
                acc += input[a * columns + b] * kernel[i * kcolumns + j];
            }
        }
        out[x * columns + y] = normalize_pixel(acc, sum);
    }
}

}  // namespace manual
```

- [ ] **Step 4: Generar el diff de adaptación**

Run: `git diff --no-index third_party/manual-origin/hpc_conv_omp.cpp src/manual.cpp > third_party/manual-origin/adaptacion.diff || true`
Expected: archivo no vacío.

- [ ] **Step 5: Registrar**

```cpp
    static const std::vector<Variant> v = {
        {"seq", seq::convolve},
        {"ai", ai::convolve},
        {"manual", manual::convolve},
    };
```

- [ ] **Step 6: Compilar y probar**

Run: `bash scripts/build.sh && ./build/test_convolve.exe`
Expected: `variants: seq ai manual` y `OK (0 failures)`.

- [ ] **Step 7: Commit**

```bash
git add third_party src/manual.cpp src/register.cpp
git commit -m "feat: add human-written OpenMP variant adapted from rdb987/2DConvolution"
```

---

### Task 6: Variante "IA + optimización dirigida por el equipo"

**Files:**
- Create: `ai_log/04_propuestas_optimizacion.md`, `ai_log/05_decisiones_equipo.md`, `src/ai_opt.cpp`
- Modify: `src/register.cpp`

**Interfaces:**
- Produces: `namespace ai_opt { void convolve(...); }` registrado como `"ai_opt"`.

- [ ] **Step 1: Medición exploratoria de `ai` para orientar propuestas**

Run: `./build/bench.exe --sizes 2048 --ksizes 3,11 --threads 1,8,16 --runs 5 --variants seq,ai,manual --out build/explore.csv`
Leer `src/ai.cpp` y los tiempos.

- [ ] **Step 2: Escribir `ai_log/04_propuestas_optimizacion.md`**

Una sección por propuesta con: qué cambia, por qué debería mejorar (cache, ramas, vectorización, balance de carga), riesgo para la equivalencia. Catálogo a evaluar contra el código real de `ai.cpp` (solo se proponen las que apliquen):
- **O1 Separar interior y bordes:** bucle interior sin comprobaciones de límites para `r ≤ y < h-r`, `r ≤ x < w-r`; bordes con la ruta comprobada.
- **O2 Punteros de fila:** precalcular `const uint8_t* row = in + (y+ky-r)*w + (x-r)` y recorrer `kx` con índice contiguo para permitir autovectorización.
- **O3 Reparto por filas con `schedule(static)`** y `num_threads(nthreads)` explícito sobre el bucle `y`.
- **O4 Kernel separable:** el kernel binomial es separable (k = fila ⊗ fila), lo que reduce de k² a 2k operaciones por píxel. Solo es válido si se recibe la fila; con la firma común habría que detectar separabilidad en tiempo de ejecución. Se documenta como propuesta con riesgo de "cambiar el algoritmo" y su impacto en la comparabilidad.

- [ ] **Step 3: ⏸ CHECKPOINT DEL EQUIPO**

Presentar al usuario el resumen de `04_propuestas_optimizacion.md`. El equipo decide cuáles aplicar y por qué. Registrar la decisión literal en `ai_log/05_decisiones_equipo.md`. No continuar sin esta respuesta.

- [ ] **Step 4: Implementar `src/ai_opt.cpp`**

Partir de una copia de `src/ai.cpp`, cambiar el namespace a `ai_opt` y aplicar solo las propuestas aprobadas. Comentario de cabecera con la lista de cambios (O1…O4) aplicados.

- [ ] **Step 5: Registrar**

```cpp
    static const std::vector<Variant> v = {
        {"seq", seq::convolve},
        {"ai", ai::convolve},
        {"ai_opt", ai_opt::convolve},
        {"manual", manual::convolve},
    };
```

- [ ] **Step 6: Compilar y probar**

Run: `bash scripts/build.sh && ./build/test_convolve.exe`
Expected: `variants: seq ai ai_opt manual` y `OK (0 failures)`.

- [ ] **Step 7: Commit**

```bash
git add ai_log src/ai_opt.cpp src/register.cpp
git commit -m "feat: add AI + team-directed optimization variant"
```

---

### Task 7: Script de análisis (estadísticas, figuras, tablas)

**Files:**
- Create: `scripts/analyze.py`, `scripts/test_analyze.py`

**Interfaces:**
- Consumes: CSV `variant,width,height,ksize,threads,run,time_ms`.
- Produces:
  - `load(path) -> list[dict]`
  - `summarize(rows) -> dict[(variant, n, ksize, threads)] -> {"mean","std","min","max","cv","n"}`
  - `add_speedup(summary) -> None` (añade `"sp"`, `"ep"`, `"sp_self"`; lanza `ValueError` si falta la secuencial de un (n, ksize))
  - CLI: `python scripts/analyze.py results/raw.csv` → `results/summary.csv`, `results/figures/*.pdf|png`, `paper/tables/*.tex`

- [ ] **Step 1: Escribir el test `scripts/test_analyze.py`**

```python
import io
import math
import unittest

import analyze

CSV = """variant,width,height,ksize,threads,run,time_ms
seq,8,8,3,1,0,10.0
seq,8,8,3,1,1,12.0
ai,8,8,3,1,0,10.0
ai,8,8,3,1,1,10.0
ai,8,8,3,2,0,5.0
ai,8,8,3,2,1,5.0
"""


class AnalyzeTest(unittest.TestCase):
    def setUp(self):
        self.rows = analyze.load(io.StringIO(CSV))
        self.s = analyze.summarize(self.rows)
        analyze.add_speedup(self.s)

    def test_mean_and_sample_std(self):
        seq = self.s[("seq", 8, 3, 1)]
        self.assertAlmostEqual(seq["mean"], 11.0)
        self.assertAlmostEqual(seq["std"], math.sqrt(2.0))
        self.assertEqual(seq["n"], 2)

    def test_speedup_and_efficiency_vs_sequential(self):
        ai2 = self.s[("ai", 8, 3, 2)]
        self.assertAlmostEqual(ai2["sp"], 2.2)
        self.assertAlmostEqual(ai2["ep"], 1.1)
        self.assertAlmostEqual(ai2["sp_self"], 2.0)

    def test_missing_seq_raises(self):
        rows = [r for r in self.rows if r["variant"] != "seq"]
        s = analyze.summarize(rows)
        with self.assertRaisesRegex(ValueError, "seq"):
            analyze.add_speedup(s)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Correr para ver el fallo**

Run: `cd scripts && python -m unittest test_analyze -v`
Expected: FAIL `ModuleNotFoundError: No module named 'analyze'`.

- [ ] **Step 3: Escribir `scripts/analyze.py`**

```python
"""Statistics, figures and LaTeX tables for the convolution benchmark."""
import csv
import os
import sys
from collections import defaultdict

import numpy as np

VARIANT_ORDER = ["seq", "ai", "ai_opt", "manual"]
LABELS = {
    "seq": "Secuencial",
    "ai": "IA directa",
    "ai_opt": "IA + opt. equipo",
    "manual": "Manual (GitHub)",
}


def load(src):
    """Reads the raw CSV from a path or a file-like object."""
    f = open(src, newline="") if isinstance(src, str) else src
    try:
        return [
            {
                "variant": r["variant"],
                "n": int(r["width"]),
                "ksize": int(r["ksize"]),
                "threads": int(r["threads"]),
                "time_ms": float(r["time_ms"]),
            }
            for r in csv.DictReader(f)
        ]
    finally:
        if isinstance(src, str):
            f.close()


def summarize(rows):
    groups = defaultdict(list)
    for r in rows:
        groups[(r["variant"], r["n"], r["ksize"], r["threads"])].append(r["time_ms"])
    out = {}
    for key, times in groups.items():
        t = np.array(times)
        std = float(t.std(ddof=1)) if len(t) > 1 else 0.0
        mean = float(t.mean())
        out[key] = {"mean": mean, "std": std, "min": float(t.min()),
                    "max": float(t.max()), "cv": std / mean, "n": len(t)}
    return out


def add_speedup(summary):
    for (variant, n, ks, p), st in summary.items():
        seq = summary.get(("seq", n, ks, 1))
        if seq is None:
            raise ValueError(f"missing seq baseline for n={n} ksize={ks}")
        own1 = summary.get((variant, n, ks, 1))
        st["sp"] = seq["mean"] / st["mean"]
        st["ep"] = st["sp"] / p
        st["sp_self"] = own1["mean"] / st["mean"] if own1 else float("nan")


def write_summary_csv(summary, path):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["variant", "n", "ksize", "threads", "mean_ms", "std_ms", "min_ms",
                    "max_ms", "cv", "runs", "speedup", "efficiency", "speedup_self"])
        for key in sorted(summary, key=lambda k: (k[1], k[2], VARIANT_ORDER.index(k[0]), k[3])):
            s = summary[key]
            w.writerow([*key, f"{s['mean']:.4f}", f"{s['std']:.4f}", f"{s['min']:.4f}",
                        f"{s['max']:.4f}", f"{s['cv']:.4f}", s["n"], f"{s['sp']:.4f}",
                        f"{s['ep']:.4f}", f"{s['sp_self']:.4f}"])


def plot_metric(summary, n, metric, ylabel, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    ksizes = sorted({k[2] for k in summary if k[1] == n})
    threads = sorted({k[3] for k in summary if k[1] == n})
    fig, axes = plt.subplots(2, 2, figsize=(9, 7), sharex=True)
    for ax, ks in zip(axes.flat, ksizes):
        for v in VARIANT_ORDER:
            if v == "seq":
                continue
            pts = [(p, summary[(v, n, ks, p)]) for p in threads if (v, n, ks, p) in summary]
            if not pts:
                continue
            xs = [p for p, _ in pts]
            ys = [s[metric] for _, s in pts]
            err = [s["std"] for _, s in pts] if metric == "mean" else None
            ax.errorbar(xs, ys, yerr=err, marker="o", capsize=3, label=LABELS[v])
        if metric == "mean" and ("seq", n, ks, 1) in summary:
            ax.axhline(summary[("seq", n, ks, 1)]["mean"], color="gray", ls="--", label=LABELS["seq"])
        if metric == "sp":
            ax.plot(threads, threads, color="gray", ls=":", label="Ideal")
        if metric == "ep":
            ax.axhline(1.0, color="gray", ls=":", label="Ideal")
        ax.set_xscale("log", base=2)
        ax.set_xticks(threads)
        ax.set_xticklabels([str(t) for t in threads])
        ax.set_title(f"Kernel {ks}×{ks}")
        ax.set_xlabel("Hilos (p)")
        ax.set_ylabel(ylabel)
        ax.grid(True, alpha=0.3)
    axes.flat[0].legend(fontsize=8)
    fig.suptitle(f"Imagen {n}×{n}")
    fig.tight_layout()
    fig.savefig(path + ".pdf")
    fig.savefig(path + ".png", dpi=150)
    plt.close(fig)


def write_latex_table(summary, n, path):
    ksizes = sorted({k[2] for k in summary if k[1] == n})
    threads = sorted({k[3] for k in summary if k[1] == n})
    lines = [
        "\\begin{tabular}{llrrrr}",
        "\\toprule",
        "Kernel & Variante & $p$ & $\\bar{T}$ (ms) $\\pm\\sigma$ & $S_p$ & $E_p$ \\\\",
        "\\midrule",
    ]
    for ks in ksizes:
        for v in VARIANT_ORDER:
            for p in threads:
                s = summary.get((v, n, ks, p))
                if s is None:
                    continue
                lines.append(f"{ks}$\\times${ks} & {LABELS[v]} & {p} & "
                             f"{s['mean']:.2f} $\\pm$ {s['std']:.2f} & {s['sp']:.2f} & {s['ep']:.2f} \\\\")
        lines.append("\\midrule")
    lines[-1] = "\\bottomrule"
    lines.append("\\end{tabular}")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main(raw_path):
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    summary = summarize(load(raw_path))
    add_speedup(summary)
    fig_dir = os.path.join(root, "results", "figures")
    tab_dir = os.path.join(root, "paper", "tables")
    os.makedirs(fig_dir, exist_ok=True)
    os.makedirs(tab_dir, exist_ok=True)
    write_summary_csv(summary, os.path.join(root, "results", "summary.csv"))
    for n in sorted({k[1] for k in summary}):
        plot_metric(summary, n, "mean", "Tiempo (ms)", os.path.join(fig_dir, f"tiempo_{n}"))
        plot_metric(summary, n, "sp", "Speedup $S_p$", os.path.join(fig_dir, f"speedup_{n}"))
        plot_metric(summary, n, "ep", "Eficiencia $E_p$", os.path.join(fig_dir, f"eficiencia_{n}"))
        write_latex_table(summary, n, os.path.join(tab_dir, f"tabla_{n}.tex"))
    print(f"analyzed {len(summary)} configurations")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "results/raw.csv")
```

- [ ] **Step 4: Correr los tests**

Run: `cd scripts && python -m unittest test_analyze -v`
Expected: 3 tests OK.

- [ ] **Step 5: Probar extremo a extremo con datos de humo**

Run:
```bash
./build/bench.exe --sizes 128 --ksizes 3,5,7,11 --threads 1,2,4 --runs 3 --out build/smoke.csv && python scripts/analyze.py build/smoke.csv
```
Expected: `analyzed N configurations`, figuras `results/figures/{tiempo,speedup,eficiencia}_128.{pdf,png}` y `paper/tables/tabla_128.tex`. Borrar luego esos artefactos de humo: `rm -f results/figures/*_128.* paper/tables/tabla_128.tex results/summary.csv`.

- [ ] **Step 6: Commit**

```bash
git add scripts/analyze.py scripts/test_analyze.py
git commit -m "feat: add analysis script for statistics, figures and LaTeX tables"
```

---

### Task 8: Ejecución del benchmark completo

**Files:**
- Create: `scripts/run_bench.sh`, `results/raw.csv`, `results/environment.txt`, `results/summary.csv`, `results/figures/*`, `paper/tables/*`

- [ ] **Step 1: Escribir `scripts/run_bench.sh`**

```bash
#!/usr/bin/env bash
# Full benchmark: build, test, record environment, run, analyze.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh
./build/test_convolve.exe
mkdir -p results
bash scripts/env_info.sh > results/environment.txt
./build/bench.exe --out results/raw.csv
python scripts/analyze.py results/raw.csv
```

- [ ] **Step 2: ⏸ CHECKPOINT DEL USUARIO — preparar la máquina**

Pedir al usuario: conectar a corriente, cerrar aplicaciones pesadas (navegador, Docker Desktop, juegos), y autorizar el cambio del plan de energía a "Alto rendimiento" (`powercfg /setactive SCHEME_MIN`; se restaura al final con el GUID que muestre `powercfg /getactivescheme` antes del cambio). No ejecutar sin confirmación.

- [ ] **Step 3: Ejecutar en segundo plano**

Run (background): `bash scripts/run_bench.sh 2> results/bench_progress.log`
Duración estimada: 30–60 min. Vigilar `results/bench_progress.log`.
Expected: exit 0; `results/raw.csv` con 4 × 4 × (1 + 3 × 5) × 30 = 7 680 filas + cabecera.

- [ ] **Step 4: Verificar**

Run: `wc -l results/raw.csv && python -c "import csv;r=list(csv.DictReader(open('results/raw.csv')));print(len(r))" && ls results/figures | wc -l`
Expected: 7681 líneas, 7680 filas, 24 archivos de figura (3 métricas × 4 tamaños × 2 formatos). Revisar `results/summary.csv`: CV > 0.10 en alguna configuración se reporta como amenaza a la validez.

- [ ] **Step 5: Restaurar plan de energía y commit**

```bash
git add scripts/run_bench.sh results paper/tables
git commit -m "data: full benchmark run (30 runs per configuration) with summary, figures and tables"
```

---

### Task 9: Paper v1 en LaTeX

**Files:**
- Create: `paper/main.tex`, `paper/referencias.bib`, `paper/figures/` (copias de las PDF de `results/figures`), `scripts/build_paper.sh`, `entrega/paper_v1.pdf`, `entrega/prompt_evaluacion.txt`

- [ ] **Step 1: ⏸ Pedir al usuario los datos de autores**

Nombres completos, afiliación (programa/universidad), correos y ORCID si tienen. No inventar nombres.

- [ ] **Step 2: Escribir `scripts/build_paper.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/toolchain.sh
mkdir -p paper/figures
cp results/figures/*.pdf paper/figures/
"$TECTONIC" paper/main.tex
echo "paper/main.pdf built"
```

- [ ] **Step 3: Escribir el esqueleto `paper/main.tex`**

```latex
\documentclass[12pt]{article}
\usepackage[spanish,es-noshorthands]{babel}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage{newtxtext,newtxmath}   % Times New Roman, as required by the journal
\usepackage[a4paper,margin=2.5cm]{geometry}
\usepackage{graphicx,booktabs,amsmath,siunitx,hyperref,listings,xcolor}
\usepackage[nocompress]{cite}
\setlength{\parindent}{0pt}
\setlength{\parskip}{6pt}
\graphicspath{{figures/}}
\lstset{basicstyle=\ttfamily\footnotesize,breaklines=true,frame=single}

\title{Benchmarking de código C++ paralelo generado por inteligencia artificial: convolución 2D de imágenes con OpenMP}
\author{AUTORES_DEL_STEP_1}
\date{}

\begin{document}
\maketitle

\section*{Resumen}
\textbf{Objetivo:} \ldots \textbf{Materiales y métodos:} \ldots
\textbf{Resultados y discusión:} \ldots \textbf{Conclusiones:} \ldots

\textbf{Palabras clave:} programación paralela; OpenMP; convolución 2D; código generado por IA; benchmarking.

\section*{Abstract}
% English version of the structured abstract.

\section{Introducción}
\section{Materiales y métodos}
\subsection{Entorno de hardware y software}
\subsection{Problema y contrato funcional}
\subsection{Variantes evaluadas}
\subsection{Protocolo de medición}
\subsection{Métricas}
\section{Resultados}
\section{Discusión}
\subsection{Amenazas a la validez}
\section{Conclusiones}
\section*{Declaración de uso de inteligencia artificial}
\section*{Disponibilidad de código y datos}

\bibliographystyle{IEEEtran}
\bibliography{referencias}
\end{document}
```
`AUTORES_DEL_STEP_1` se sustituye por los datos reales del Step 1 antes de compilar.

- [ ] **Step 4: Redactar cada sección con los datos reales**

Contenido obligatorio por sección (cubre todos los puntos del prompt de evaluación):
- **Resumen** estructurado ≤ 200 palabras, con cifras reales tomadas de `results/summary.csv` (speedup máximo por variante en 4096² y 11×11; eficiencia a 8 y 16 hilos).
- **Introducción:** problema (calidad de rendimiento del código paralelo generado por IA), objetivo general y objetivos específicos, preguntas que responde el estudio.
- **Entorno:** tabla con todos los datos de `results/environment.txt` (CPU, núcleos físicos/lógicos, caché, RAM, SO, compilador exacto, flags, plan de energía, versiones de Python y bibliotecas).
- **Problema y contrato:** ecuación numerada de la convolución, normalización, zero-padding, kernels binomiales; por qué aritmética entera garantiza equivalencia exacta.
- **Variantes:** secuencial; IA directa (modelo, fecha, prompt resumido y referencia a `ai_log/`, número de intentos); IA + optimización dirigida por el equipo (propuestas aplicadas y rechazadas, quién decidió); manual (rdb987, commit, año, adaptaciones de `SOURCE.md`).
- **Protocolo:** validación bit a bit previa, 1 calentamiento, 30 repeticiones, solo tiempo de `convolve` con `steady_clock`, orden rotado de variantes, generación de imágenes con semilla, 4 × 4 × 5 configuraciones.
- **Métricas:** ecuaciones numeradas de $\bar{T}$, $\sigma$ (muestral), CV, $S_p = T_{seq}/T_p$, $E_p = S_p/p$; justificar el uso de $T_{seq}$ como $T_1$ y reportar también el speedup relativo.
- **Resultados:** `\input{tables/tabla_4096}` en el cuerpo y las figuras `tiempo_4096`, `speedup_4096`, `eficiencia_4096`, más un resumen de los demás tamaños (las tablas completas quedan en el repositorio). Solo afirmaciones que se lean de los datos.
- **Discusión:** comparación entre las cuatro variantes; efecto de HyperThreading (p = 16 sobre 8 núcleos físicos); efecto del tamaño del kernel y de la imagen (intensidad aritmética, memoria); variabilidad (CV).
- **Amenazas a la validez:** una sola máquina, Windows con procesos en segundo plano, una sola generación de IA (no determinista), adaptación de la variante manual, imágenes sintéticas, un solo compilador y flags.
- **Conclusiones:** respuestas directas a los objetivos, solo con evidencia de las tablas.
- **Declaración de uso de IA:** variante IA directa generada con Claude Opus 5.5; optimización dirigida por el equipo implementada con asistencia de IA; asistencia de IA en la redacción y el análisis; evaluación del manuscrito con la herramienta que use el equipo.
- **Disponibilidad:** URL del repositorio y cómo reproducir (`scripts/run_bench.sh`).
- **Referencias** (`referencias.bib`, IEEE): especificación OpenMP 5.2, documentación de GCC, repositorio rdb987/2DConvolution, Amdahl (1967), Gustafson (1988) y bibliografía estándar de convolución/procesamiento de imágenes. Cada entrada se verifica antes de citarla; no se inventan referencias.
- Límite de 5 400 palabras incluyendo referencias.

- [ ] **Step 5: Compilar y verificar**

Run: `bash scripts/build_paper.sh`
Expected: `paper/main.pdf` sin errores; revisar que todas las figuras y tablas aparecen, que las ecuaciones están numeradas y que no quedan marcadores `\ldots` del esqueleto.

- [ ] **Step 6: Preparar la entrega v1**

```bash
mkdir -p entrega && cp paper/main.pdf entrega/paper_v1.pdf
```
Escribir en `entrega/prompt_evaluacion.txt` el prompt del enunciado copiado literalmente de `CL11_04102026.pdf` (página 2).

- [ ] **Step 7: Commit**

```bash
git add paper scripts/build_paper.sh entrega
git commit -m "docs: paper v1 with full results"
```

- [ ] **Step 8: ⏸ CHECKPOINT DEL EQUIPO — Evaluación inicial**

Avisar al usuario: subir `entrega/paper_v1.pdf` a ChatGPT o Gemini Pro (anotar cuál y la versión) en un chat nuevo, pegar `entrega/prompt_evaluacion.txt` y guardar la respuesta completa como `entrega/evaluacion_inicial.pdf`. También pegar el texto de la respuesta en el chat con Claude para el Task 10. Para el zip de Overleaf: `cd paper && zip -r ../entrega/paper_overleaf.zip . -x "*.pdf"` (o `git push` al remoto de Overleaf si la cuenta es Premium).

---

### Task 10: Paper v2 según las observaciones aceptadas

**Files:**
- Create: `entrega/decisiones_evaluacion.md`
- Modify: `paper/main.tex`

- [ ] **Step 1: Tabla de decisiones**

En `entrega/decisiones_evaluacion.md`, una fila por observación de la evaluación inicial: clasificación (crítica/importante/menor), resumen, decisión del equipo (aceptar/rechazar/parcial), justificación. Claude propone una postura técnica por fila; el equipo decide. Las observaciones que pidan datos que no existen se marcan como "rechazada: requeriría inventar datos" o "aceptada: nuevo experimento" (solo si da tiempo y se re-ejecuta el benchmark de verdad).

- [ ] **Step 2: Aplicar los cambios aceptados a `paper/main.tex` y compilar**

Run: `bash scripts/build_paper.sh && cp paper/main.pdf entrega/paper_v2.pdf`

- [ ] **Step 3: Commit**

```bash
git add paper entrega
git commit -m "docs: paper v2 addressing accepted review observations"
```

- [ ] **Step 4: ⏸ CHECKPOINT DEL EQUIPO — Evaluación final**

Avisar al usuario: subir `entrega/paper_v2.pdf` a la misma IA, en un chat nuevo y con el mismo prompt; guardar como `entrega/evaluacion_final.pdf` y pegar el texto en el chat con Claude.

---

### Task 11: Reflexión, versión final, README y publicación

**Files:**
- Modify: `paper/main.tex`, `README.md`
- Create: `entrega/paper_final.pdf`

- [ ] **Step 1: Sección "Reflexión sobre la evaluación mediante IA"**

Añadir antes de Conclusiones: herramienta y versión usadas; puntaje inicial y final con sus justificaciones; observaciones aceptadas con efecto; observaciones rechazadas con argumento técnico (al menos las de `decisiones_evaluacion.md` marcadas como rechazadas); límites del evaluador (no ejecuta el código, puede alucinar); qué aprendió el equipo. Redactada a partir de las decisiones del equipo.

- [ ] **Step 2: Compilar la versión final**

Run: `bash scripts/build_paper.sh && cp paper/main.pdf entrega/paper_final.pdf`

- [ ] **Step 3: Escribir `README.md`**

```markdown
# AI-Generated C++ Parallel Code Benchmarking

Benchmark de convolución 2D en C++/OpenMP: secuencial, IA directa, IA + optimización dirigida por el equipo
y una implementación manual de GitHub. Imágenes de 512² a 4096², kernels 3×3 a 11×11, 1–16 hilos, 30 repeticiones.

## Reproducir
1. `winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e`
2. `bash scripts/run_bench.sh` (compila, prueba, registra entorno, mide y analiza)
3. `bash scripts/build_paper.sh` (requiere `tools/tectonic.exe`, ver Task 1 del plan)

## Estructura
- `src/` variantes y harness · `tests/` equivalencia bit a bit · `scripts/` build, ejecución y análisis
- `ai_log/` prompt y respuestas de la IA, propuestas y decisiones de optimización
- `third_party/manual-origin/` código original de la variante manual, origen y diff de adaptación
- `results/` CSV crudo, resumen, figuras y entorno · `paper/` LaTeX · `entrega/` PDFs entregables
```

- [ ] **Step 4: Commit final**

```bash
git add paper entrega README.md
git commit -m "docs: final paper with AI-review reflection and reproduction README"
```

- [ ] **Step 5: ⏸ Confirmar con el usuario y publicar**

Preguntar antes de `git push origin main`. Tras confirmar, hacer push y mostrar la lista de entregables en `entrega/`.
