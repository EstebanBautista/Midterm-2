# Propuestas de optimización para la variante "IA + optimización dirigida por el equipo"

Punto de partida: `src/ai.cpp` (IA directa). Medición exploratoria (2048×2048, 5 repeticiones, `build/explore.csv`):

| Kernel | Variante | p=1 (ms) | p=8 (ms) | p=16 (ms) | Sp a p=16 vs. secuencial |
|---|---|---|---|---|---|
| 3×3 | Secuencial | 40.4 | – | – | 1.00 |
| 3×3 | IA directa | 8.6 | 2.2 | 1.7 | 23.5 |
| 3×3 | Manual | 63.9 | 11.9 | 10.7 | 3.8 |
| 11×11 | Secuencial | 354.7 | – | – | 1.00 |
| 11×11 | IA directa | 178.4 | 38.0 | 35.2 | 10.1 |
| 11×11 | Manual | 422.9 | 77.4 | 65.6 | 5.4 |

Hallazgo principal: la IA vectorizó con SSE2 solo cuando todos los pesos caben en int16 (≤ 32 767). El kernel binomial 11×11 tiene pesos de hasta 63 504, así que en 11×11 el código cae a un camino escalar y pierde la mayor parte de su ventaja.

## O1 — Mantener SIMD con pesos de hasta 16 bits sin signo (recomendada)
- Qué cambia: cada peso w (0 ≤ w ≤ 65 535) se descompone en w = 256·hi + lo con hi, lo ≤ 255 (caben en int16). Se calculan dos acumulaciones con `_mm_madd_epi16` y se combina acc = 256·acc_hi + acc_lo.
- Por qué mejora: el 11×11 deja de usar el camino escalar; se espera acercarlo al rendimiento del 7×7 por operación.
- Riesgo de equivalencia: nulo si se respeta la aritmética entera (acc máximo 255·2^20 ≈ 2.7·10^8 < 2^31). Las pruebas bit a bit lo verifican.
- Alcance: cambia solo la condición `fast` y el bucle interno.

## O2 — Convolución separable cuando el kernel es de rango 1
- Qué cambia: en tiempo de ejecución se detecta si k[i][j] = a[i]·b[j] con enteros; si es así, se hace una pasada horizontal y otra vertical (2k en lugar de k² productos por píxel).
- Por qué mejora: para 11×11 reduce de 121 a 22 productos por píxel (≈5×).
- Riesgo de equivalencia: nulo en aritmética entera (distributividad exacta), pero **cambia el algoritmo**: la comparación con las otras variantes deja de ser "misma cantidad de trabajo". Si se aplica, el paper debe declararlo y discutirlo como amenaza a la comparabilidad.

## O3 — AVX2 con despacho en tiempo de ejecución
- Qué cambia: versión del bucle interno con `__attribute__((target("avx2")))` (16 píxeles por instrucción) y selección con `__builtin_cpu_supports("avx2")`.
- Por qué mejora: duplica el ancho vectorial; el i7-10700F soporta AVX2.
- Riesgo: rompe el espíritu de "mismos flags para todos" (las demás variantes no pueden usar AVX2 porque no hay `-march=native`). Habría que declararlo.

## O4 — Limpieza menor
- Eliminar la advertencia `-Wignored-attributes` (`std::vector<__m128i>` → buffer alineado de `int32_t`). Sin efecto en rendimiento.

## Recomendación técnica de Claude
Aplicar O1 y O4 (mejoran sin cambiar el algoritmo ni las reglas de compilación). O2 y O3 se documentan como propuestas rechazadas por comparabilidad, lo cual también aporta a la discusión del paper. La decisión es del equipo.
