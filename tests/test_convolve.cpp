#include "../src/convolve.hpp"
#include "../src/variants.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            ++failures;                                               \
            std::printf("FAIL %s:%d: ", __FILE__, __LINE__);          \
            std::printf(__VA_ARGS__);                                 \
            std::printf("\n");                                        \
        }                                                             \
    } while (0)

static void test_kernel() {
    auto k3 = make_binomial_kernel(3);
    const int32_t e3[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
    CHECK(k3.size() == 9 && std::memcmp(k3.data(), e3, sizeof e3) == 0, "binomial 3x3");
    auto k1 = make_binomial_kernel(1);
    CHECK(k1.size() == 1 && k1[0] == 1, "binomial 1x1");
    long long s = 0;
    for (int32_t v : make_binomial_kernel(11)) s += v;
    CHECK(s == 1048576LL, "sum of 11x11 kernel = %lld, expected 1048576", s);
}

static void test_seq_known_values() {
    std::vector<uint8_t> in(9, 100), out(9, 0);
    auto k = make_binomial_kernel(3);
    seq::convolve(in.data(), out.data(), 3, 3, k.data(), 3, 1);
    CHECK(out[4] == 100, "center = %d, expected 100", out[4]);
    CHECK(out[0] == 56, "corner = %d, expected 56", out[0]);   // (900+8)/16
    CHECK(out[1] == 75, "edge = %d, expected 75", out[1]);     // (1200+8)/16
}

static void test_identity_kernel() {
    auto img = make_image(37, 29, 7u);
    auto k = make_binomial_kernel(1);
    for (const Variant& v : variants()) {
        std::vector<uint8_t> out(img.size(), 0xAB);
        v.fn(img.data(), out.data(), 37, 29, k.data(), 1, 4);
        CHECK(out == img, "%s: 1x1 kernel must be identity", v.name);
    }
}

static void test_equivalence() {
    const int shapes[][2] = {{1, 1}, {5, 3}, {37, 29}, {64, 64}, {129, 7}};
    const int ksizes[] = {3, 5, 7, 11};
    const int threads[] = {1, 2, 3, 4, 8, 16};
    for (auto& s : shapes) {
        int w = s[0], h = s[1];
        auto img = make_image(w, h, 1000u + w * 31u + h);
        for (int ks : ksizes) {
            auto k = make_binomial_kernel(ks);
            std::vector<uint8_t> ref(img.size());
            seq::convolve(img.data(), ref.data(), w, h, k.data(), ks, 1);
            for (const Variant& v : variants()) {
                for (int p : threads) {
                    std::vector<uint8_t> out(img.size(), 0xAB);  // detects unwritten pixels
                    v.fn(img.data(), out.data(), w, h, k.data(), ks, p);
                    CHECK(out == ref, "%s differs from seq: w=%d h=%d k=%d p=%d", v.name, w, h, ks, p);
                }
            }
        }
    }
}

static void test_saturation() {
    std::vector<uint8_t> img(64 * 64, 255);
    auto k = make_binomial_kernel(11);
    for (const Variant& v : variants()) {
        std::vector<uint8_t> out(img.size(), 0);
        v.fn(img.data(), out.data(), 64, 64, k.data(), 11, 8);
        CHECK(out[32 * 64 + 32] == 255, "%s: saturated interior = %d", v.name, out[32 * 64 + 32]);
    }
}

int main() {
    test_kernel();
    test_seq_known_values();
    test_identity_kernel();
    test_equivalence();
    test_saturation();
    std::printf("variants:");
    for (const Variant& v : variants()) std::printf(" %s", v.name);
    std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
