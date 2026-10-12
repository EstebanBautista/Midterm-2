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
