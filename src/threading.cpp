#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "threading.hpp"

#include <functional>

#ifdef WIN32

//TODO: Error handling

#include <process.h>

	struct Threading::thread_t {
		HANDLE hndl;
		std::function<unsigned int(void *)> start_routine;
		void * user_data;
	};

	struct Threading::mutex_t {
		HANDLE hndl;
	};

	unsigned int __stdcall call_helper(void * data) {
		Threading::thread_t * t = (Threading::thread_t*) data;
		return t->start_routine(t->user_data);
	}

	Threading::thread_t * Threading::create(std::function<unsigned int(void *)> start_routine, void * args) {
		Threading::thread_t * t = new Threading::thread_t();
		t->start_routine = start_routine;
		t->user_data = args;

		t->hndl = (HANDLE)_beginthreadex(nullptr, 0, &call_helper, t, 0, nullptr);
		return t;
	}

	unsigned int Threading::join(Threading::thread_t * thread, unsigned long timeout) {
		unsigned int ret;
		WaitForSingleObject(thread->hndl, timeout);
		if(GetExitCodeThread(thread->hndl, (LPDWORD)&ret)) {
			return ret;
		} else {
			return -1; //LALALALALA
		}

	}

	void Threading::exit(unsigned int retval) {
		_endthreadex(retval);
	}
	
	void Threading::free(Threading::thread_t * thread) {
		CloseHandle(thread->hndl);
		delete thread;
	}

	Threading::mutex_t * Threading::mutex_create() {
		mutex_t * m = new mutex_t();
		m->hndl = CreateMutex(nullptr, false, nullptr);
		return m;
	}

	bool Threading::mutex_lock(Threading::mutex_t * mutex, unsigned long timeout) {
		return(WaitForSingleObject( mutex->hndl, timeout ) == WAIT_OBJECT_0);
	}

	void Threading::mutex_unlock(Threading::mutex_t * mutex) {
		ReleaseMutex(mutex->hndl);
	}

	void Threading::mutex_free(Threading::mutex_t * mutex) {
		CloseHandle(mutex->hndl);
		delete mutex;
	}

	unsigned int Threading::num_cores() {
		SYSTEM_INFO sysinfo;
		GetSystemInfo( &sysinfo );

		return sysinfo.dwNumberOfProcessors;
	}
#else
#error "Threading is not implemented for linux yet, fix it!"
	//sysconf( _SC_NPROCESSORS_ONLN );
#endif