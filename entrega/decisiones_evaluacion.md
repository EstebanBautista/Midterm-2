# Decisiones del equipo sobre la evaluación inicial

- Evaluador: Gemini Pro (Google), chat nuevo, prompt literal de `prompt_evaluacion.txt`.
- Documento evaluado: `paper_v1.pdf`.
- Puntaje inicial: 80/100 (rigor experimental 15/20, corrección técnica 12/20, análisis de resultados 18/20, reproducibilidad 20/20, calidad científica 15/20).
- Las posturas técnicas fueron propuestas por Claude a partir de verificaciones sobre los datos y el código; el equipo las aprobó todas ("de acuerdo con las 6 posturas").

| # | Observación (clasificación del evaluador) | Decisión | Justificación y evidencia | Cambio en la v2 |
|---|---|---|---|---|
| 1 | Comparación asimétrica: `-O2` sin `-O3`/`-march=native` impide autovectorizar el secuencial y el manual (crítica) | Parcial | La premisa es incorrecta en los hechos: con `-O3 -march=native`, el informe de vectorización de GCC (`-fopt-info-vec-optimized`) solo vectoriza el bucle que suma los pesos del kernel, no la convolución, en `seq.cpp` y `manual.cpp`. Una prueba rápida en 4096² dio 163,9 ms (O3/native) frente a 162,6 ms (O2) para el secuencial 3×3. Se mantienen los flags del diseño, que son iguales para todas las variantes. Se acepta que conviene mostrarlo con datos. | Experimento de sensibilidad con `-O3 -march=native` (4096², todos los kernels e hilos, 30 repeticiones) y aclaración de que S_p combina SIMD e hilos, mientras S_rel aísla los hilos. |
| 2 | La Tabla 3 tiene dígitos truncados que invalidan S_p y E_p (crítica) | Rechazada | Error de lectura del evaluador. La extracción de texto de `paper_v1.pdf` contiene los valores correctos (597.69, 9.68, 120.34, 124.68), que coinciden con `results/summary.csv`. La tabla estaba en letra muy pequeña, lo que probablemente causó la mala lectura. | La tabla del anexo se divide en dos y se imprime en letra más grande. |
| 3 | Variabilidad: sin anclaje de hilos ni pruebas de significancia (importante) | Parcial | Se aceptan las pruebas estadísticas. Para el anclaje se hizo una prueba con `OMP_PROC_BIND=close`, `OMP_PLACES=cores` en 12 configuraciones con 8 hilos (30 repeticiones): el CV bajó en 8 de 12, pero el tiempo medio cambió en ambas direcciones. El efecto no es concluyente en Windows/libgomp, por lo que no se repite todo el experimento; se reporta la prueba. | Prueba de Mann-Whitney entre IA directa e IA + optimización por configuración; mediana en el resumen; párrafo con la prueba de anclaje. |
| 4 | La variante manual es ineficiente; se sobreestima a la IA (importante) | Aceptada | Ya estaba en la discusión y en las amenazas, pero el resumen y las conclusiones no lo decían. | Resumen y conclusiones precisan "implementación paralela directa (naive)". |
| 5 | La explicación de HyperThreading es especulativa (menor) | Parcial | Se acepta reforzar la redacción. No se instrumenta con VTune por alcance y tiempo. | Redacción explícita como inferencia no verificada. |
| 6 | Figura de tiempo poco legible (menor) | Aceptada | Las curvas de IA quedan comprimidas abajo. | Eje vertical en escala logarítmica. |

---

# Decisiones del equipo sobre la evaluación final

- Evaluador: Gemini Pro (Google), chat nuevo, mismo prompt.
- Documento evaluado: `paper_v2.pdf`.
- Puntaje final: 88/100 (rigor experimental 18/20, corrección técnica 16/20, análisis de resultados 18/20, reproducibilidad 20/20, calidad científica 16/20).
- El equipo aprobó todas las posturas y eligió aplicar los cambios aceptados en la versión final, declarando que se hicieron después de la evaluación final y no fueron re-evaluados.

| # | Observación (clasificación del evaluador) | Decisión | Justificación y evidencia | Cambio en la versión final |
|---|---|---|---|---|
| C1 | Se confunden vectorización y paralelización; deshabilitar SSE2 en el código de IA (crítica) | Parcial | Se acepta aclarar que se evalúa la capacidad combinada SIMD + hilos. Se rechaza quitar SSE2: modificaría el código de la IA, que dejaría de ser "IA directa", y el speedup relativo ya aísla la escalabilidad por hilos. | Objetivos y métricas aclaran qué mide cada indicador. |
| C2 | Explicar que la falta de vectorización se debe a la estructura del código (crítica) | Aceptada (con precisión) | La causa son las verificaciones de límites dentro del bucle interno y, en la variante manual, la división y el módulo por píxel; no "aritmética de punteros compleja" como sugiere el evaluador. | Párrafo en la discusión. |
| I1 | La Ecuación 2 usa "mix" en lugar de "max" (importante) | Rechazada | Error de lectura: con babel en español LaTeX escribe "máx" y "mín"; el texto extraído del PDF es `m´ax`, que el evaluador leyó como "mix". La ecuación siempre usó máximo. | Ninguno. |
| I2 | Reportar eficiencia relativa E_rel = S_rel/p (importante) | Aceptada | Los datos ya existían. | Ecuación, columna en las tablas del anexo y resultados. |
| I3 | Usar la mediana para el speedup o filtrar atípicos (importante) | Parcial | Se calculó el speedup con medianas: diferencia típica 1,3 %, 21 de 240 configuraciones con diferencia > 10 %, todas con tiempos medios menores que 30 ms; en 4096² no cambia el orden ni las conclusiones. Se mantiene la media porque el enunciado la exige. | Párrafo en resultados. |
| M1 | Marcar p = 8 en las gráficas (menor) | Aceptada | Facilita leer el paso de núcleos físicos a lógicos. | Línea vertical en p = 8. |
| M2 | Turbo Boost / SpeedStep no controlados (menor) | Aceptada como limitación | No se modificó la BIOS. | Nueva amenaza a la validez. |
