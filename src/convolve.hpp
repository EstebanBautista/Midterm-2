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
