#pragma once

#include <functional>

namespace os {
using ThreadFunc = typeof(void *(void *)) *;
// Accepts void*(void*)
class Thread {
public:
  // Use empty constructor to start lambdas
  Thread();
  Thread(ThreadFunc);
  Thread(const Thread &) = delete;
  ~Thread();

  int Join();
  int Detach();
  int Run(void *);
  int RunLambda(std::function<void()> *func);

private:
  static void *ThreadWrapper(void *arg);
  ThreadFunc func_;
  struct OSSpecificData;
  OSSpecificData *data_;
};
} // namespace os
