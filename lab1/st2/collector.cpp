#include "collector.hpp"

#include <algorithm>
#include <mutex>

void LockStripedCollector::record(uint64_t value) {
    {
        int bucket = std::min(value / 4, static_cast<uint64_t>(255));
        std::lock_guard<std::mutex> lock(locks[bucket % 16]);
        buckets[bucket]++;
    }
    count.fetch_add(1);
    sum.fetch_add(value);
    uint64_t cur = min.load();
    while (value < cur && !min.compare_exchange_weak(cur, value));
    cur = max.load();
    while (value > cur && !max.compare_exchange_weak(cur, value));
}

namespace {


uint64_t computePercentile(uint64_t total,
                           const std::array<uint64_t, 256>& buckets,
                           uint64_t percentile) {
    const uint64_t threshold = total * percentile / 100;
    uint64_t accumulated = 0;
    for (uint64_t i = 0; i < 256; i++) {
        accumulated += buckets[i];
        if (accumulated >= threshold) {
            return i * 4;
        }
    }
    return 255 * 4;
}

}  // namespace

Snapshot LockStripedCollector::snapshot() {
    Snapshot out;
    std::array<uint64_t, 256> local_buckets;
    for (uint64_t i = 0; i < 16; i++) {
        std::lock_guard<std::mutex> lock(locks[i]);
        for (uint64_t j = 0; j < 16; j++) {
            local_buckets[i * 16 + j] = buckets[i * 16 + j];
        }
    }
    out.buckets = local_buckets;
    out.count = count.load();
    out.sum = sum.load();
    out.min = min.load();
    out.max = max.load();
    out.p50 = computePercentile(count, local_buckets, 50);
    out.p99 = computePercentile(count, local_buckets, 99);
    return out;
}