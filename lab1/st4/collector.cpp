#include "collector.hpp"

#include <algorithm>
#include <mutex>
#include <thread>

void DoubleBufferedCollector::record(uint64_t value) {
    ThreadBuffers* my = get_my_buffers();
    /*
    int b;
    for (;;) {
        b = active_.load();
        my->inside.store(b);
        if (active_.load() == b) break;
        my->inside.store(-1, std::memory_order_release);
    }
    */
    int b = active_.load();
    my->inside.store(b);

    Buf& dst = my->buf[b];
    ++dst.buckets[std::min(value / 4, (uint64_t)255)];
    ++dst.count;
    dst.sum += value;
    if (value < dst.min) dst.min = value;
    if (value > dst.max) dst.max = value;

    my->inside.store(-1, std::memory_order_release);
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


Snapshot DoubleBufferedCollector::snapshot() {
    std::lock_guard<std::mutex> g(snap_lock_);

    int old = active_.load();
    active_.store(1 - old);

    for (const auto& up : all_buffers_) {
        ThreadBuffers* s = up.get();
        while (s->inside.load() == old) {
            std::this_thread::yield();
        }
    }

    for (const auto& up : all_buffers_) {
        const Buf& src = up->buf[old];
        for (uint64_t i = 0; i < 256; ++i) {
            global_buckets_[i] += src.buckets[i];
        }
        global_count_ += src.count;
        global_sum_ += src.sum;
        if (src.min < global_min_) global_min_ = src.min;
        if (src.max > global_max_) global_max_ = src.max;
    }

    for (const auto& up : all_buffers_) {
        up->buf[old].clear();
    }

    uint64_t p50 = computePercentile(global_count_, global_buckets_, 50);
    uint64_t p99 = computePercentile(global_count_, global_buckets_, 99);
    return Snapshot{global_buckets_, global_count_, global_sum_,
                    global_min_, global_max_, p50, p99};
}