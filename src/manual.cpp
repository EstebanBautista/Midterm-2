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
