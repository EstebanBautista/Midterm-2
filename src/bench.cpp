#include "convolve.hpp"
#include "variants.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string t;
    while (std::getline(ss, t, ',')) if (!t.empty()) out.push_back(t);
    return out;
}

static std::vector<int> parse_ints(const std::string& s) {
    std::vector<int> v;
    for (auto& t : split(s)) v.push_back(std::stoi(t));
    return v;
}

int main(int argc, char** argv) {
    std::vector<int> sizes{512, 1024, 2048, 4096}, ksizes{3, 5, 7, 11}, threads{1, 2, 4, 8, 16};
    int runs = 30;
    std::string out_path = "results/raw.csv";
    std::vector<std::string> names;
    for (const Variant& v : variants()) names.push_back(v.name);

    for (int i = 1; i < argc; i += 2) {
        std::string a = argv[i];
        if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", a.c_str()); return 1; }
        std::string b = argv[i + 1];
        if (a == "--sizes") sizes = parse_ints(b);
        else if (a == "--ksizes") ksizes = parse_ints(b);
        else if (a == "--threads") threads = parse_ints(b);
        else if (a == "--runs") runs = std::stoi(b);
        else if (a == "--variants") names = split(b);
        else if (a == "--out") out_path = b;
        else { std::fprintf(stderr, "unknown argument %s\n", a.c_str()); return 1; }
    }

    std::vector<const Variant*> selected;
    for (auto& n : names) {
        auto it = std::find_if(variants().begin(), variants().end(),
                               [&](const Variant& v) { return n == v.name; });
        if (it == variants().end()) { std::fprintf(stderr, "unknown variant %s\n", n.c_str()); return 1; }
        selected.push_back(&*it);
    }

    FILE* f = std::fopen(out_path.c_str(), "w");
    if (!f) { std::perror(out_path.c_str()); return 1; }
    std::fprintf(f, "variant,width,height,ksize,threads,run,time_ms\n");

    using clock = std::chrono::steady_clock;
    int config = 0;
    for (int n : sizes) {
        auto img = make_image(n, n, 42u + static_cast<uint32_t>(n));
        std::vector<uint8_t> ref(img.size()), buf(img.size());
        for (int ks : ksizes) {
            auto k = make_binomial_kernel(ks);
            seq::convolve(img.data(), ref.data(), n, n, k.data(), ks, 1);
            for (int p : threads) {
                std::vector<const Variant*> order;
                for (auto* v : selected)
                    if (p == 1 || std::strcmp(v->name, "seq") != 0) order.push_back(v);
                if (order.empty()) continue;
                // Rotate the execution order per configuration to spread warm-up/thermal bias.
                std::rotate(order.begin(), order.begin() + (config++ % order.size()), order.end());
                for (auto* v : order) {
                    std::memset(buf.data(), 0xAB, buf.size());
                    v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);
                    if (buf != ref) {
                        std::fprintf(stderr, "VALIDATION FAILED: %s n=%d k=%d p=%d\n", v->name, n, ks, p);
                        std::fclose(f);
                        return 2;
                    }
                    v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);  // warm-up, discarded
                    for (int r = 0; r < runs; ++r) {
                        auto t0 = clock::now();
                        v->fn(img.data(), buf.data(), n, n, k.data(), ks, p);
                        auto t1 = clock::now();
                        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                        std::fprintf(f, "%s,%d,%d,%d,%d,%d,%.6f\n", v->name, n, n, ks, p, r, ms);
                    }
                    std::fflush(f);
                    std::fprintf(stderr, "done %-7s n=%-5d k=%-2d p=%d\n", v->name, n, ks, p);
                }
            }
        }
    }
    std::fclose(f);
    return 0;
}
