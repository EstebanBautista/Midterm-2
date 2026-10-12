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
