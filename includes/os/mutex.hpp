#pragma once

#include<pthread.h>

namespace os{
class Mutex{
public:
	Mutex();
	Mutex(const Mutex&) = delete;
	Mutex(Mutex&&) = delete;
	void lock();
	void unlock();
private:
	pthread_mutex_t mu_;
};
};
