#include "scheduler.hpp"
#include <stdexcept>

namespace sched {

thread_local ThreadCtx g_ctx;

Scheduler::Scheduler() {}

Scheduler::~Scheduler() {
  for (auto &t : threads_)
    if (t.joinable())
      t.join();
}

void Scheduler::spawn(std::function<void()> body) {
  bodies_.push_back(std::move(body));
}

void Scheduler::log_line(const std::string &op) {
  if (!logging)
    return;
  int id = g_ctx.id;
  log.push_back(std::string(1, letter(id)) + "  " + op);
}

void Scheduler::yield_to_sched(ThreadState new_state) {
  int id = g_ctx.id;
  state_[id] = new_state;
  sem_sched_.release();
  sem_[id]->acquire();
}

void Scheduler::run(Strategy &strategy) {
  int n = static_cast<int>(bodies_.size());
  pending_.assign(n, false);
  state_.assign(n, ThreadState::READY);
  sem_.clear();
  for (int i = 0; i < n; ++i)
    sem_.push_back(std::make_unique<std::binary_semaphore>(0));
  trace_.clear();
  last_ = -1;
  aborted_ = false;

  for (int i = 0; i < n; ++i) {
    threads_.emplace_back([this, i] {
      g_ctx.sched = this;
      g_ctx.id = i;
      sem_[i]->acquire();
      try {
        bodies_[i]();
      } catch (...) {
      }
      state_[i] = ThreadState::FINISHED;
      sem_sched_.release();
    });
  }

  // Старт: довести каждый поток до первой паузы.
  // pending_[i] = true ДО release — иначе поток проскочит первый get без паузы.
  for (int i = 0; i < n; ++i) {
    pending_[i] = true;
    state_[i] = ThreadState::RUNNING;
    sem_[i]->release();
    sem_sched_.acquire();
  }

  while (true) {
    std::vector<int> enabled;
    bool any_alive = false;
    for (int i = 0; i < n; ++i) {
      if (state_[i] == ThreadState::READY)
        enabled.push_back(i);
      if (state_[i] != ThreadState::FINISHED)
        any_alive = true;
    }
    if (enabled.empty()) {
      if (!any_alive)
        break;
      break; // deadlock — этап 2
    }
    int choice = strategy.pick(enabled, last_);
    last_ = choice;
    trace_.push_back(letter(choice));
    state_[choice] = ThreadState::RUNNING;
    sem_[choice]->release();
    sem_sched_.acquire();
  }

  for (auto &t : threads_)
    if (t.joinable())
      t.join();
  threads_.clear();
}

std::string Scheduler::trace_string() const { return trace_; }

void Scheduler::register_mutex(sched_mutex *m) { mutexes_.push_back(m); }
std::string Scheduler::held_string() const { return ""; }

void point() {
  Scheduler *s = g_ctx.sched;
  if (!s)
    return;
  s->log_line("window");
  s->mark_pending();
}

void await(std::function<bool()> cond) {
  Scheduler *s = g_ctx.sched;
  if (!s)
    return;
  while (!cond()) {
    s->clear_pending();
    s->yield_to_sched(ThreadState::BLOCKED);
  }
  if (s->has_pending()) {
    s->clear_pending();
    s->log_line("await done");
    s->yield_to_sched(ThreadState::READY);
  }
  s->mark_pending();
}

} // namespace sched