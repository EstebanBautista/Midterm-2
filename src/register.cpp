#include "variants.hpp"

const std::vector<Variant>& variants() {
    static const std::vector<Variant> v = {
        {"seq", seq::convolve},
        {"ai", ai::convolve},
    };
    return v;
}
