#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <atomic>

struct Snapshot {
    std::array<uint64_t, 256> buckets;
    uint64_t count;
    uint64_t sum;
    uint64_t min;
    uint64_t max;
    uint64_t p50;
    uint64_t p99;
};

class MetricsCollector {
public:
    virtual ~MetricsCollector() = default;
    virtual void record(uint64_t value) = 0;
    virtual Snapshot snapshot() = 0;
};


class LockStripedCollector : public MetricsCollector {
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;

private:
    std::array<uint64_t, 256> buckets{};
    std::array<std::mutex, 16> locks{};
    std::atomic<uint64_t> count{0};
    std::atomic<uint64_t> sum{0};
    std::atomic<uint64_t> min{UINT64_MAX};
    std::atomic<uint64_t> max{0};
};