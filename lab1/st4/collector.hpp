#pragma once

#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
#include <atomic>

struct Buf {
    std::array<uint64_t, 256> buckets{};
    uint64_t count = 0, sum = 0, min = UINT64_MAX, max = 0;
    void clear() {
        buckets.fill(0);
        count = 0; sum = 0; min = UINT64_MAX; max = 0;
    }
};

struct alignas(64) ThreadBuffers {
    std::atomic<int> inside{-1};
    Buf buf[2];
};

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

inline uint64_t next_collector_id() {
    static std::atomic<uint64_t> counter{1};
    return counter.fetch_add(1);
}

class DoubleBufferedCollector : public MetricsCollector {
public:
    void record(uint64_t value) override;
    Snapshot snapshot() override;
private:
    const uint64_t id_ = next_collector_id();
    std::mutex snap_lock_;
    std::vector<std::unique_ptr<ThreadBuffers>> all_buffers_;
    std::atomic<int> active_{0};
    std::array<uint64_t, 256> global_buckets_{0};
    uint64_t global_count_ = 0;
    uint64_t global_sum_ = 0;
    uint64_t global_min_ = UINT64_MAX;
    uint64_t global_max_ = 0;

    ThreadBuffers* get_my_buffers() {
        struct TLSSlot { uint64_t id = 0; ThreadBuffers* buffers = nullptr; };
        static thread_local TLSSlot slot;
        if (slot.id != id_) {
            auto s = std::make_unique<ThreadBuffers>();
            ThreadBuffers* raw = s.get();
            {
                std::lock_guard<std::mutex> g(snap_lock_);
                all_buffers_.push_back(std::move(s));
            }
            slot.id = id_;
            slot.buffers = raw;
        }
        return slot.buffers;
    }
};
