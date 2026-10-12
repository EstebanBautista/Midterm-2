# Decisiones del equipo sobre las propuestas de optimización

Fecha: 2026-10-12

Decisión del equipo (redacción corregida a petición del equipo; el contenido es el mismo del mensaje original, que se conserva al final):

> El equipo aprueba las sugerencias O1 y O4. Se considera acertado aplicar O1 porque mantiene la vectorización en el kernel 11×11 sin cambiar el algoritmo y conservando un resultado exacto. También se aplica O4, aunque sea una limpieza menor del código. O2 y O3 no se aplican.

| Propuesta | Decisión | Justificación del equipo |
|---|---|---|
| O1 — SIMD con pesos de 16 bits sin signo (w = 256·hi + lo) | Aceptada | Mantiene la vectorización en 11×11 sin cambiar el algoritmo y con resultado exacto. |
| O2 — Convolución separable | Rechazada | Cambia el algoritmo (2k en lugar de k² productos); rompería la comparabilidad con las demás variantes. |
| O3 — AVX2 con despacho en tiempo de ejecución | Rechazada | Rompe la regla de mismos flags/conjunto de instrucciones para todas las variantes. |
| O4 — Eliminar advertencia `-Wignored-attributes` | Aceptada | Limpieza menor del código. |

Implementación: Claude, siguiendo esta decisión.

Mensaje original del equipo (literal):

> "okey me parece correcto lo que sugieres, aplicalo es accertado tomar la sugerencia uno dado que asi se mantiene la vectorizacion 11x11, asi manteniendo el algoritmo un resultado exacto, ademas la 04 tambien es bueno hacerlo aunque sea una limpieza menor"
