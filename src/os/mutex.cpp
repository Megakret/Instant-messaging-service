#include <os/mutex.hpp>

namespace os {
Mutex::Mutex() { pthread_mutex_init(&mu_, nullptr); }
void Mutex::lock() { pthread_mutex_lock(&mu_); }
void Mutex::unlock() { pthread_mutex_unlock(&mu_); }
} // namespace os
