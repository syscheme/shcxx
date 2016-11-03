#include "eloop_lock.h"

namespace ZQ {
	namespace eloop {
		// -----------------------------
		// class Mutex
		// -----------------------------

		Mutex::Mutex()
		{
			uv_mutex_init(&_mutex);
		}
		Mutex::~Mutex()
		{
			uv_mutex_destroy(&_mutex);
		}

		void Mutex::lock() const
		{
			uv_mutex_lock((LPCRITICAL_SECTION)&_mutex);
		}

		void Mutex::unlock() const
		{
			uv_mutex_unlock((LPCRITICAL_SECTION)&_mutex);
		}

		//Try to lock the mutex, return True if the lock was acquired, False otherwise
		int Mutex::tryLock() const
		{
			return uv_mutex_trylock((LPCRITICAL_SECTION)&_mutex);
		}
	}
}