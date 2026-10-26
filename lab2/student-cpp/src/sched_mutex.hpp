#pragma once
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "scheduler.hpp"

namespace sched {

class sched_mutex {
public:
  sched_mutex(Scheduler &s, std::string name)
      : sched_(s), name(std::move(name)) {
    s.register_mutex(this);
  }
  sched_mutex(const sched_mutex &) = delete;

  void lock() {
    Scheduler *s = g_ctx.sched;
    if (!s)
      return;
    while (owner_ != -1) {
      waiters_.push_back(g_ctx.id);
      s->clear_pending();
      s->yield_to_sched(ThreadState::BLOCKED);
      if (s->is_aborted())
        throw std::runtime_error("aborted");
    }
    if (s->has_pending()) {
      s->clear_pending();
      s->yield_to_sched(ThreadState::READY);
    }
    owner_ = g_ctx.id;
    s->log_line("lock " + name);
    s->mark_pending();
  }

  void unlock() {
    Scheduler *s = g_ctx.sched;
    if (!s)
      return;
    if (s->is_aborted()) {
      owner_ = -1;
      return;
    }
    if (s->has_pending()) {
      s->clear_pending();
      s->yield_to_sched(ThreadState::READY);
    }
    owner_ = -1;
    s->log_line("unlock " + name);
    for (int w : waiters_)
      s->wake(w);
    waiters_.clear();
    s->mark_pending();
  }

  int owner() const { return owner_; }
  const std::string name;

private:
  Scheduler &sched_;
  int owner_ = -1;
  std::vector<int> waiters_;
};

class sched_guard {
public:
  explicit sched_guard(sched_mutex &m) : m_(m) { m_.lock(); }
  ~sched_guard() noexcept(false) { m_.unlock(); }
  sched_guard(const sched_guard &) = delete;

private:
  sched_mutex &m_;
};

} // namespace sched