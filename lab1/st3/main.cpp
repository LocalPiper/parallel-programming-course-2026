#include "collector.hpp"
#include "loadgen.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <latch>
#include <numeric>
#include <thread>
#include <vector>


double run(MetricsCollector& collector,
           const std::vector<uint64_t>& values,
           unsigned T, double seconds) {
    std::latch start(1);
    std::atomic<bool> stop{false};
    std::vector<uint64_t> ops(T, 0);

    std::vector<std::thread> threads;
    threads.reserve(T);
    for (unsigned k = 0; k < T; ++k) {
        threads.emplace_back([&, k] {
            uint64_t local_count = 0;
            uint64_t i = (k * 1000) % values.size();
            start.wait();
            while (!stop.load()) {
                collector.record(values[i]);
                ++local_count;
                if (++i == values.size()) i = 0;
            }
            ops[k] = local_count;
        });
    }

    auto t0 = std::chrono::steady_clock::now();
    start.count_down();
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    stop.store(true);
    auto t1 = std::chrono::steady_clock::now();

    for (auto& t : threads) t.join();

    uint64_t total = std::accumulate(ops.begin(), ops.end(), uint64_t{0});
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    return double(total) / elapsed;
}


double measurePoint(MetricsCollector& collector,
                    const std::vector<uint64_t>& values,
                    unsigned T) {
    run(collector, values, T, 5.0);

    std::vector<double> results;
    results.reserve(5);
    for (int r = 0; r < 5; ++r)
        results.push_back(run(collector, values, T, 5.0));

    std::cout << "snapshot count: " << collector.snapshot().count << "\n";

    std::sort(results.begin(), results.end());
    return results[results.size() / 2];
}

int main() {
    auto values = generate(42);
    ThreadLocalCollector collector;
    unsigned T = 16;

    double ops_per_sec = measurePoint(collector, values, T);
    std::cout << "T= " << T << ": " << ops_per_sec << " ops/sec\n";
}