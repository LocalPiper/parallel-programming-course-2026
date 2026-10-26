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
  explicit Dfs(int bound) : bound_(bound) {}

  void begin_run() override {
    pos_ = 0;
    evictions_ = 0;
  }

  int pick(const std::vector<int> &enabled, int last) override {
    if (pos_ == (int)path_.size()) {
      std::vector<int> choices;
      for (int id : enabled) {
        bool evicts = (last != -1 && id != last);
        if (!evicts || evictions_ < bound_)
          choices.push_back(id);
      }
      if (choices.empty()) {
        choices.push_back(enabled[0]);
      }
      choices_.push_back(choices);
      path_.push_back(0);
    } else if ((int)choices_.size() <= pos_) {
      choices_.resize(pos_ + 1);
      path_.resize(pos_ + 1, 0);
    }
    int idx = path_[pos_];
    int chosen = choices_[pos_][idx];
    if (last != -1 && chosen != last)
      evictions_++;
    pos_++;
    return chosen;
  }

  bool end_run() override {
    while (!path_.empty()) {
      int i = (int)path_.size() - 1;
      if (path_[i] + 1 < (int)choices_[i].size()) {
        path_[i]++;
        path_.resize(i + 1);
        choices_.resize(i + 1);
        return true;
      }
      path_.pop_back();
      choices_.pop_back();
    }
    return false;
  }

private:
  int bound_;
  std::vector<std::vector<int>> choices_;
  std::vector<int> path_;
  int pos_ = 0;
  int evictions_ = 0;
};

} // namespace sched