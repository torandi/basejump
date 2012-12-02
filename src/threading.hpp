#ifndef THREADING_HPP
#define THREADING_HPP

#include <functional>

namespace Threading {
	struct thread_t; //Opaque thread structure

	thread_t * create(std::function<unsigned int(void*)> start_routine, void * args = nullptr);
	unsigned int join(thread_t * thread, unsigned long timeout=0);
	void exit(unsigned int retval);
	void free(thread_t * thread);

	struct mutex_t;
	mutex_t * mutex_create();
	bool mutex_lock(mutex_t * mutex, unsigned long timeout=0);
	void mutex_unlock(mutex_t * mutex);
	void mutex_free(mutex_t * mutex);

	unsigned int num_cores();
};

#endif