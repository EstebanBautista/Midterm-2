# Notas sobre la variante "IA directa"

- Primer intento: compiló y pasó todas las pruebas de equivalencia bit a bit (5 formas × 4 kernels × 6 conteos de hilos, identidad 1×1 y saturación). No hubo segundo intento.
- Advertencia del compilador (no bloqueante): `ignoring attributes on template argument '__m128i' [-Wignored-attributes]` en `std::vector<__m128i>`.
- Observación técnica: la ruta rápida SSE2 (`_mm_madd_epi16`) solo se activa si todos los pesos caben en int16 (≤ 32767). El kernel binomial 11×11 tiene peso máximo C(10,5)² = 252² = 63 504, por lo que para 11×11 el código usa su camino escalar genérico. Esto debe considerarse al interpretar los resultados de 11×11.
- Estrategia de la IA: reparto estático de filas por hilo, ring buffer de filas convertidas a int16 con zero-padding, vectorización SSE2 manual con intrínsecos, normalización por multiplicación con el inverso en doble precisión (con prueba de exactitud en comentario).
