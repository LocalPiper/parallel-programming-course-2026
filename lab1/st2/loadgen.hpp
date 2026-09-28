#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

constexpr uint64_t kArraySize = 1 << 20;


inline std::vector<uint64_t> generate(uint64_t seed) {
    constexpr uint64_t kMaxRank = 1023;
    constexpr double kS = 1.15;
    std::mt19937_64 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<double> cdf(kMaxRank);
    double total = 0.0;
    for (uint64_t i = 0; i < kMaxRank; i++) {
        cdf[i] = (total += 1.0 / std::pow((double)(i + 1), kS));
    }
    for (auto& v : cdf) v /= total;

    std::vector<uint64_t> out(kArraySize);
    for (auto& x : out) {
        x = std::lower_bound(cdf.begin(), cdf.end(), dist(gen)) - cdf.begin() + 1;
    }
    return out;
}