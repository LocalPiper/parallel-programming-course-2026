#pragma once

#include <array>
#include <cstdint>

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


class EmptyCollector : public MetricsCollector {
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;

private:
    std::array<uint64_t, 256> buckets{};
    uint64_t count = 0;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;
};

class MutexCollector : public MetricsCollector {
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;

private:
    std::array<uint64_t, 256> buckets{};
    uint64_t count = 0;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX;
    uint64_t max = 0;
};