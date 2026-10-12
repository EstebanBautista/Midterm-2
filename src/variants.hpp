#pragma once
#include "convolve.hpp"
#include <vector>

struct Variant {
    const char* name;
    ConvolveFn fn;
};

// Registered variants, "seq" always first. Defined in register.cpp.
const std::vector<Variant>& variants();
