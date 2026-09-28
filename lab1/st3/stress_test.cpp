#include "collector.hpp"
#include "loadgen.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace {

constexpr unsigned kNumWriters = 4;
constexpr uint64_t kNumSnapshots = 10000;

template <typename Collector>
void runStressTest(const char* name, Collector& collector,
                   const std::vector<uint64_t>& values) {
    std::atomic<bool> stop{false};
    std::vector<uint64_t> perThreadCalls(kNumWriters, 0);

    std::vector<std::thread> writers;
    writers.reserve(kNumWriters);
    for (unsigned k = 0; k < kNumWriters; ++k) {
        writers.emplace_back([&, k] {
            uint64_t local_count = 0;
            uint64_t i = (k * 1000) % values.size();
            while (!stop.load()) {
                collector.record(values[i]);
                ++local_count;
                if (++i == values.size()) i = 0;
            }
            perThreadCalls[k] = local_count;
        });
    }

    uint64_t broken = 0;
    uint64_t less = 0;
    uint64_t greater = 0;

    for (uint64_t s = 0; s < kNumSnapshots; ++s) {
        Snapshot snap = collector.snapshot();
        uint64_t bucket_sum = std::accumulate(
            snap.buckets.begin(), snap.buckets.end(), uint64_t{0});
        if (bucket_sum != snap.count) {
            ++broken;
            if (bucket_sum < snap.count) {
                ++less;
            } else {
                ++greater;
            }
        }
    }

    stop.store(true, std::memory_order_relaxed);
    for (auto& t : writers) t.join();

    uint64_t totalCalls = std::accumulate(
        perThreadCalls.begin(), perThreadCalls.end(), uint64_t{0});
    uint64_t finalCount = collector.snapshot().count;
    int64_t diff = static_cast<int64_t>(finalCount) - static_cast<int64_t>(totalCalls);

    double pct = 100.0 * static_cast<double>(broken) / kNumSnapshots;

    std::cout << "=== " << name << " ===\n";
    std::cout << "snapshot() проверок:      " << kNumSnapshots << "\n";
    std::cout << "битых снимков (sum!=cnt): " << broken << "  (" << pct << "%)\n";
    std::cout << "  sum < count:            " << less << "\n";
    std::cout << "  sum > count:            " << greater << "\n";
    std::cout << "вызовов record() всего:   " << totalCalls << "\n";
    std::cout << "итоговый count:           " << finalCount << "\n";
    std::cout << "разница (count - calls):  " << diff << "\n";
    std::cout << "\n";
}

}  // namespace

int main() {
    auto values = generate(42);
    ThreadLocalCollector collector;
    runStressTest("Этап 3: ThreadLocalCollector", collector, values);
    return 0;
}