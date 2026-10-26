#pragma once
#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "strategy.hpp"

namespace sched {

struct FirstAlive : Strategy {
  int pick(const std::vector<int> &enabled, int) override { return enabled[0]; }
};

struct Replay : Strategy {
  explicit Replay(std::string trace) : trace_(std::move(trace)) {}
  void begin_run() override { pos_ = 0; }
  int pick(const std::vector<int> &enabled, int) override {
    if (pos_ >= trace_.size())
      throw std::runtime_error("replay: trace ended before run finished");
    char c = trace_[pos_++];
    int id = c - 'A';
    if (std::find(enabled.begin(), enabled.end(), id) == enabled.end())
      throw std::runtime_error(std::string("replay: thread ") + c +
                               " not enabled");
    return id;
  }

private:
  std::string trace_;
  size_t pos_ = 0;
};

struct RandomPick : Strategy {
  explicit RandomPick(uint32_t seed) : seed_(seed), rnd_(seed) {}
  void begin_run() override { rnd_.seed(seed_); }
  int pick(const std::vector<int> &enabled, int) override {
    return enabled[rnd_() % enabled.size()];
  }

private:
  uint32_t seed_;
  std::mt19937 rnd_;
};

struct Dfs : Strategy {
  explicit Dfs(int) {}
  void begin_run() override {}
  int pick(const std::vector<int> &, int) override {
    throw std::logic_error("TODO этап 4: Dfs::pick");
  }
  bool end_run() override {
    throw std::logic_error("TODO этап 4: Dfs::end_run");
  }
};

} // namespace sched