#ifndef THREADING_HPP
#define THREADING_HPP

#include <functional>

#ifdef WIN32
	#define THREADING_INF INFINITE
#else
	#error "FIX ME"
#endif

namespace Threading {
	struct thread_t; //Opaque thread structure

	thread_t * create(std::function<unsigned int(void*)> start_routine, void * args = nullptr);
	unsigned int join(thread_t * thread, unsigned long timeout=THREADING_INF);
	void exit(unsigned int retval);
	void free(thread_t * thread);

	struct mutex_t;
	mutex_t * mutex_create();
	bool mutex_lock(mutex_t * mutex, unsigned long timeout=THREADING_INF);
	void mutex_unlock(mutex_t * mutex);
	void mutex_free(mutex_t * mutex);

	unsigned int num_cores();
};

#endif