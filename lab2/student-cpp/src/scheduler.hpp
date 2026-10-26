#pragma once
#include <functional>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#include "strategy.hpp"

namespace sched {

class Scheduler;

struct ThreadCtx {
  Scheduler *sched = nullptr;
  int id = -1;
};

extern thread_local ThreadCtx g_ctx;

enum class ThreadState { READY, RUNNING, BLOCKED, FINISHED };

class Scheduler {
  std::vector<std::function<void()>> bodies_;
  std::vector<std::thread> threads_;
  std::vector<ThreadState> state_;
  std::vector<std::unique_ptr<std::binary_semaphore>> sem_;
  std::binary_semaphore sem_sched_{0};
  std::string trace_;
  int last_ = -1;
  std::vector<bool> pending_;
  bool aborted_ = false;

  std::vector<class sched_mutex *> mutexes_;

  friend class sched_mutex;

public:
  Scheduler();
  ~Scheduler();
  void spawn(std::function<void()> body);
  void run(Strategy &strategy);
  std::string trace_string() const;

  bool logging = false;
  std::vector<std::string> log;
  std::string deadlock;

  void yield_to_sched(ThreadState new_state);
  void mark_pending() { pending_[g_ctx.id] = true; }
  bool has_pending() const { return pending_[g_ctx.id]; }
  void clear_pending() { pending_[g_ctx.id] = false; }
  int current_id() const { return g_ctx.id; }
  char letter(int id) const { return 'A' + id; }
  void log_line(const std::string &op);

  void register_mutex(class sched_mutex *m);
  std::string held_string() const;
};

void point();
void await(std::function<bool()> cond);

} // namespace sched