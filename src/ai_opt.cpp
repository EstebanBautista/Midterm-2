// ai_opt.cpp — copia de ai.cpp (IA directa) con las optimizaciones aprobadas por el equipo
// (ver ai_log/05_decisiones_equipo.md):
//   O1: la ruta SIMD acepta pesos de 0 a 65535 descomponiendo w = 256*hi + lo
//       (hi, lo <= 255 caben en int16) y combinando acc = 256*acc_hi + acc_lo.
//   O4: los pesos empaquetados se guardan en un std::vector<int32_t> (sin
//       std::vector<__m128i>, que provocaba -Wignored-attributes).
#include "convolve.hpp"

#include <omp.h>
#include <emmintrin.h>   // SSE2 (baseline en x86-64, no requiere -march)
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// Copia la fila iy de la imagen a un buffer int16 con zero-padding:
// buf[i] = in[iy][i - r] si 0 <= i - r < w; 0 en otro caso.
// Una fila fuera de la imagen queda entera en ceros.
inline void load_row(const uint8_t* in, int w, int h, int r, int iy,
                     int16_t* buf, int blen) {
    if (iy < 0 || iy >= h) {
        std::memset(buf, 0, static_cast<size_t>(blen) * sizeof(int16_t));
        return;
    }
    const uint8_t* src = in + static_cast<size_t>(iy) * static_cast<size_t>(w);
    std::memset(buf, 0, static_cast<size_t>(r) * sizeof(int16_t));
    int16_t* d = buf + r;
    const __m128i z = _mm_setzero_si128();
    int x = 0;
    for (; x + 16 <= w; x += 16) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + x));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + x), _mm_unpacklo_epi8(v, z));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + x + 8), _mm_unpackhi_epi8(v, z));
    }
    for (; x < w; ++x) d[x] = static_cast<int16_t>(src[x]);
    std::memset(buf + r + w, 0, static_cast<size_t>(blen - r - w) * sizeof(int16_t));
}

// Normalización exacta (idéntica a normalize_pixel) con camino rápido:
// si 0 <= acc y acc + S/2 < min(256*S, 2^31), el cociente es <= 255 y
// trunc(n * (1/S) + 2^-32) es exactamente n / S (error relativo del producto
// <= 2^-44 y la distancia al siguiente entero es >= 1/S > 2^-31).
// En cualquier otro caso se usa normalize_pixel directamente.
struct Normalizer {
    int32_t S;
    int32_t half;
    int64_t lim;
    double inv;

    explicit Normalizer(int32_t s) : S(s), half(s / 2), lim(0), inv(0.0) {
        if (S > 0) {
            lim = std::min<int64_t>(256LL * S, 2147483648LL);
            inv = 1.0 / static_cast<double>(S);
        }
    }

    inline uint8_t operator()(int32_t acc) const {
        if (acc >= 0) {
            const int64_t n = static_cast<int64_t>(acc) + half;
            if (n < lim) {
                constexpr double EPS = 1.0 / 4294967296.0; // 2^-32
                return static_cast<uint8_t>(
                    static_cast<int32_t>(static_cast<double>(n) * inv + EPS));
            }
        }
        return normalize_pixel(acc, S);
    }
};

} // namespace

namespace ai_opt {

void convolve(const uint8_t* in, uint8_t* out, int w, int h,
              const int32_t* k, int ksize, int nthreads) {
    if (w <= 0 || h <= 0 || ksize <= 0) return;
    const int nt = nthreads < 1 ? 1 : nthreads;
    const int r = ksize / 2;
    const int kk = ksize * ksize;

    // Suma de pesos (int32 con aritmética modular, igual que una suma int32).
    uint32_t su = 0;
    bool fast = true;    // O1: pesos en [0, 65535] -> ruta pmaddwd
    bool split = false;  // O1: algún peso > 32767 -> w = 256*hi + lo
    for (int i = 0; i < kk; ++i) {
        su += static_cast<uint32_t>(k[i]);
        if (k[i] < 0 || k[i] > 65535) fast = false;
        if (k[i] > 32767) split = true;
    }
    const int32_t S = static_cast<int32_t>(su);
    const Normalizer norm(S);

    // Pesos empaquetados por pares (k[kx], k[kx+1]) para _mm_madd_epi16,
    // replicados en 4 lanes (O4: int32_t en lugar de std::vector<__m128i>).
    // Sin split, wlo guarda el peso completo; con split, wlo = w & 255, whi = w >> 8.
    const int npairs = (ksize + 1) / 2;
    std::vector<int32_t> wlo, whi;
    if (fast) {
        const size_t nw = static_cast<size_t>(ksize) * npairs * 4;
        wlo.resize(nw);
        if (split) whi.resize(nw);
        auto pack = [](int32_t a, int32_t b) {
            return static_cast<int32_t>((static_cast<uint32_t>(a) & 0xFFFFu) |
                                        (static_cast<uint32_t>(b) << 16));
        };
        for (int ky = 0; ky < ksize; ++ky) {
            for (int j = 0; j < npairs; ++j) {
                const int kx = 2 * j;
                const int32_t k0 = k[ky * ksize + kx];
                const int32_t k1 = (kx + 1 < ksize) ? k[ky * ksize + kx + 1] : 0;
                const size_t at = (static_cast<size_t>(ky) * npairs + j) * 4;
                for (int l = 0; l < 4; ++l) {
                    if (split) {
                        wlo[at + l] = pack(k0 & 255, k1 & 255);
                        whi[at + l] = pack(k0 >> 8, k1 >> 8);
                    } else {
                        wlo[at + l] = pack(k0, k1);
                    }
                }
            }
        }
    }

    // Longitud del buffer de fila: padding r a cada lado + holgura para
    // lecturas vectoriales de 8 elementos más allá del último píxel.
    const int blen = w + 2 * r + 32;

    omp_set_dynamic(0);
#pragma omp parallel num_threads(nt)
    {
        const int tid = omp_get_thread_num();
        const int T = omp_get_num_threads();
        const int y0 = static_cast<int>(static_cast<int64_t>(h) * tid / T);
        const int y1 = static_cast<int>(static_cast<int64_t>(h) * (tid + 1) / T);

        if (y0 < y1) {
            // Ring buffer de ksize filas convertidas a int16 con zero-padding.
            std::vector<int16_t> storage(static_cast<size_t>(blen) * ksize);
            std::vector<const int16_t*> rows(ksize);
            const int base = y0 - r;
            auto slot = [&](int iy) -> int16_t* {
                return storage.data() +
                       static_cast<size_t>((iy - base) % ksize) * blen;
            };

            for (int j = 0; j < ksize; ++j) {
                const int iy = base + j;
                load_row(in, w, h, r, iy, slot(iy), blen);
            }

            alignas(16) int32_t tmp[8];

            for (int y = y0; y < y1; ++y) {
                if (y > y0) {
                    const int iy = y + r;  // nueva fila que entra en la ventana
                    load_row(in, w, h, r, iy, slot(iy), blen);
                }
                for (int ky = 0; ky < ksize; ++ky) rows[ky] = slot(y - r + ky);

                uint8_t* o = out + static_cast<size_t>(y) * static_cast<size_t>(w);

                if (fast && !split) {
                    const int32_t* wbase = wlo.data();
                    for (int x = 0; x < w; x += 8) {
                        __m128i a0 = _mm_setzero_si128();
                        __m128i a1 = _mm_setzero_si128();
                        for (int ky = 0; ky < ksize; ++ky) {
                            const int16_t* p = rows[ky] + x;
                            const int32_t* wp = wbase + static_cast<size_t>(ky) * npairs * 4;
                            for (int j = 0; j < npairs; ++j) {
                                const __m128i u = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(p + 2 * j));
                                const __m128i v = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(p + 2 * j + 1));
                                const __m128i wj = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(wp + 4 * j));
                                a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_unpacklo_epi16(u, v), wj));
                                a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_unpackhi_epi16(u, v), wj));
                            }
                        }
                        _mm_store_si128(reinterpret_cast<__m128i*>(tmp), a0);
                        _mm_store_si128(reinterpret_cast<__m128i*>(tmp + 4), a1);
                        const int n = std::min(8, w - x);
                        for (int i = 0; i < n; ++i) o[x + i] = norm(tmp[i]);
                    }
                } else if (fast) {
                    // O1: acc = 256*acc_hi + acc_lo, exacto en int32.
                    const int32_t* lbase = wlo.data();
                    const int32_t* hbase = whi.data();
                    for (int x = 0; x < w; x += 8) {
                        __m128i l0 = _mm_setzero_si128();
                        __m128i l1 = _mm_setzero_si128();
                        __m128i h0 = _mm_setzero_si128();
                        __m128i h1 = _mm_setzero_si128();
                        for (int ky = 0; ky < ksize; ++ky) {
                            const int16_t* p = rows[ky] + x;
                            const size_t off = static_cast<size_t>(ky) * npairs * 4;
                            const int32_t* lp = lbase + off;
                            const int32_t* hp = hbase + off;
                            for (int j = 0; j < npairs; ++j) {
                                const __m128i u = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(p + 2 * j));
                                const __m128i v = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(p + 2 * j + 1));
                                const __m128i lo = _mm_unpacklo_epi16(u, v);
                                const __m128i hi = _mm_unpackhi_epi16(u, v);
                                const __m128i wl = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(lp + 4 * j));
                                const __m128i wh = _mm_loadu_si128(
                                    reinterpret_cast<const __m128i*>(hp + 4 * j));
                                l0 = _mm_add_epi32(l0, _mm_madd_epi16(lo, wl));
                                l1 = _mm_add_epi32(l1, _mm_madd_epi16(hi, wl));
                                h0 = _mm_add_epi32(h0, _mm_madd_epi16(lo, wh));
                                h1 = _mm_add_epi32(h1, _mm_madd_epi16(hi, wh));
                            }
                        }
                        _mm_store_si128(reinterpret_cast<__m128i*>(tmp),
                                        _mm_add_epi32(_mm_slli_epi32(h0, 8), l0));
                        _mm_store_si128(reinterpret_cast<__m128i*>(tmp + 4),
                                        _mm_add_epi32(_mm_slli_epi32(h1, 8), l1));
                        const int n = std::min(8, w - x);
                        for (int i = 0; i < n; ++i) o[x + i] = norm(tmp[i]);
                    }
                } else {
                    // Camino genérico (pesos fuera de [0, 65535]): escalar, aritmética
                    // modular de 32 bits (mismo resultado que la suma int32).
                    for (int x = 0; x < w; ++x) {
                        uint32_t acc = 0;
                        for (int ky = 0; ky < ksize; ++ky) {
                            const int16_t* p = rows[ky] + x;
                            const int32_t* kr = k + ky * ksize;
                            for (int kx = 0; kx < ksize; ++kx) {
                                acc += static_cast<uint32_t>(p[kx]) *
                                       static_cast<uint32_t>(kr[kx]);
                            }
                        }
                        o[x] = norm(static_cast<int32_t>(acc));
                    }
                }
            }
        }
    }
}

} // namespace ai_opt
