# Resultados

| Archivo | Qué es |
|---|---|
| `raw.csv` | Experimento principal: las 7 680 mediciones (4 tamaños × 4 kernels × 16 combinaciones de variante e hilos × 30 repeticiones). Columnas: `variant,width,height,ksize,threads,run,time_ms` |
| `summary.csv` | Estadísticas por configuración: media, desviación estándar, mínimo, máximo, CV, mediana, speedup, eficiencia, speedup relativo y eficiencia relativa |
| `environment.txt` | Hardware, sistema operativo, compilador, flags, plan de energía y versiones de Python usados |
| `bench_progress.txt` | Registro del avance del benchmark principal |
| `mann_whitney_ai_vs_ai_opt.csv` | Prueba U de Mann-Whitney entre "IA directa" e "IA + optimización" para cada configuración |
| `sensitivity_o3native/` | Experimento de sensibilidad: mismo protocolo en 4096² compilado con `-O3 -march=native` (`raw.csv`, `summary.csv`) |
| `binding_test/` | Prueba de anclaje de hilos (`OMP_PROC_BIND=close`, `OMP_PLACES=cores`), 12 configuraciones con 8 hilos |
| `figures/` | Figuras por tamaño de imagen: `tiempo_N`, `speedup_N`, `speedup_relativo_N` y `eficiencia_N` (PDF y PNG) |

Definiciones: speedup `Sp = T_seq / Tp`; eficiencia `Ep = Sp / p`; speedup relativo `T1 / Tp` usando la misma variante
con 1 hilo; eficiencia relativa = speedup relativo / p. La desviación estándar es muestral (n − 1).
