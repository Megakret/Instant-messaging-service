#include <thread.hpp>

#include <functional>
#include <pthread.h>

namespace os {
void *Thread::ThreadWrapper(void *arg) {
  auto *func_ptr = static_cast<std::function<void()> *>(arg);
  (*func_ptr)();
  return nullptr;
}
struct Thread::OSSpecificData {
  pthread_t pthread;
};
Thread::Thread() : func_(ThreadWrapper), data_(new OSSpecificData()) {}
Thread::Thread(ThreadFunc func) : func_(func), data_(new OSSpecificData()) {}
int Thread::Run(void *thread_arg) {
  int err = pthread_create(&(data_->pthread), NULL, func_, thread_arg);
  return err;
}
int Thread::Join() {
  int err = pthread_join(data_->pthread, NULL);
  return err;
}
int Thread::Detach() {
  int err = pthread_detach(data_->pthread);
  return err;
}
int Thread::RunLambda(std::function<void()> *func) {
  return Run(static_cast<void *>(func));
}
Thread::~Thread() { delete data_; }
} // namespace os
