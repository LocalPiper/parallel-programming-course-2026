#pragma once
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
  explicit Replay(std::string) {}
  void begin_run() override {}
  int pick(const std::vector<int> &, int) override {
    throw std::logic_error("TODO этап 2: Replay::pick");
  }
};

struct RandomPick : Strategy {
  explicit RandomPick(uint32_t) {}
  int pick(const std::vector<int> &, int) override {
    throw std::logic_error("TODO этап 3: RandomPick::pick");
  }
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