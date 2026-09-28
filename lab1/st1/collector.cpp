#include "collector.hpp"

#include <algorithm>
#include <mutex>

std::mutex record_mutex;
std::mutex snapshot_mutex;

void MutexCollector::record(uint64_t value) {
    std::lock_guard<std::mutex> lock(record_mutex);
    buckets[std::min(value / 4, static_cast<uint64_t>(255))]++;
    count++;
    sum += value;
    if (value < min) min = value;
    if (value > max) max = value;
}


void EmptyCollector::record(uint64_t value) {
    std::lock_guard<std::mutex> lock(record_mutex);
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

Snapshot MutexCollector::snapshot() {
    std::lock_guard<std::mutex> lock(snapshot_mutex);
    Snapshot out;
    out.buckets = buckets;
    out.count = count;
    out.sum = sum;
    out.min = min;
    out.max = max;
    out.p50 = computePercentile(count, buckets, 50);
    out.p99 = computePercentile(count, buckets, 99);
    return out;
}

Snapshot EmptyCollector::snapshot() {
    std::lock_guard<std::mutex> lock(snapshot_mutex);
    Snapshot out;
    out.buckets = buckets;
    out.count = count;
    out.sum = sum;
    out.min = min;
    out.max = max;
    out.p50 = computePercentile(count, buckets, 50);
    out.p99 = computePercentile(count, buckets, 99);
    return out;
}