#pragma once
#include <stdexcept>
#include <string>

#include "scheduler.hpp"

namespace sched {

template <class T> class shared {
public:
  shared(std::string name, T value) : name_(std::move(name)), v_(value) {}
  shared(const shared &) = delete;
  shared &operator=(const shared &) = delete;

  T get() {
    Scheduler *s = g_ctx.sched;
    if (s && s->has_pending()) {
      s->clear_pending();
      s->yield_to_sched(ThreadState::READY);
    }
    T val = v_;
    if (s) {
      s->log_line(name_ + " -> " + std::to_string(val));
      s->mark_pending();
    }
    return val;
  }

  void set(T x) {
    Scheduler *s = g_ctx.sched;
    if (s && s->has_pending()) {
      s->clear_pending();
      s->yield_to_sched(ThreadState::READY);
    }
    v_ = x;
    if (s) {
      s->log_line(name_ + " = " + std::to_string(x));
      s->mark_pending();
    }
  }

  T get_acquire() { return get(); }
  void set_release(T x) { set(x); }
  T peek() const { return v_; }

  operator T() { return get(); }
  shared &operator=(T x) {
    set(x);
    return *this;
  }
  shared &operator+=(T x) {
    T t = get();
    set(t + x);
    return *this;
  }
  shared &operator-=(T x) {
    T t = get();
    set(t - x);
    return *this;
  }

private:
  std::string name_;
  T v_;
};

} // namespace sched