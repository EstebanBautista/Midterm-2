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
3. Normalización final con `normalize_pixel`, acumulando en una variable local `int32_t acc` por píxel (el original suma directamente con `output[...] +=` y deja la suma cruda en un `int`).
4. Número de hilos con `num_threads(nthreads)`; el original usa el valor por defecto del runtime.
5. Se elimina la medición interna (`stopwatch`) y la escritura a archivo.
Se conservan: el bucle aplanado `z = 0..rows*columns-1`, el cálculo `x = z/columns, y = z%columns`,
el orden de los bucles del kernel y el `#pragma omp for` con `schedule` por defecto.
