#include "collector.hpp"

#include <algorithm>
#include <mutex>

inline void relaxed_add(std::atomic<uint64_t>& c, uint64_t delta) {
    c.store(c.load(std::memory_order_relaxed) + delta, std::memory_order_relaxed);
}

void ThreadLocalCollector::record(uint64_t value) {
    ThreadState* s = get_my_state();
    uint64_t b = std::min(value / 4, (uint64_t)255);

    relaxed_add(s->buckets[b], 1);
    relaxed_add(s->count, 1);
    relaxed_add(s->sum, value);

    if (value < s->min.load(std::memory_order_relaxed))
        s->min.store(value, std::memory_order_relaxed);
    if (value > s->max.load(std::memory_order_relaxed))
        s->max.store(value, std::memory_order_relaxed);
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

Snapshot ThreadLocalCollector::snapshot() {
    std::array<uint64_t, 256> buckets{0};
    uint64_t count = 0, sum = 0, min = UINT64_MAX, max = 0;

    std::lock_guard<std::mutex> g(list_lock_);
    for (const auto& up : all_states_) {
        ThreadState* s = up.get();
        for (int i = 0; i < 256; i++) {
            buckets[i] += s->buckets[i].load(std::memory_order_relaxed);
        }
        count += s->count.load(std::memory_order_relaxed);
        sum   += s->sum.load(std::memory_order_relaxed);
        min    = std::min(min, s->min.load(std::memory_order_relaxed));
        max    = std::max(max, s->max.load(std::memory_order_relaxed));
    }

    uint64_t p50 = computePercentile(count, buckets, 50);
    uint64_t p99 = computePercentile(count, buckets, 99);
    return Snapshot{buckets, count, sum, min, max, p50, p99};
}