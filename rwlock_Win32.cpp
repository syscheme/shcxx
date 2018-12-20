#include "rwlock.h"
#include <windows.h>

typedef struct {
	HANDLE			count_lock;
	HANDLE			data_lock;
	volatile long	read_count;
	
} _rwlock_st;


/*
#define LOCK_COUNT()	WaitForSingleObject(rwl->count_lock, RWLOCK_DEADLOCK_TIMEO)
#define UNLOCK_COUNT()	ReleaseMutex(rwl->count_lock)

#define LOCK_DATA()		WaitForSingleObject(rwl->data_lock, RWLOCK_DEADLOCK_TIMEO)
#define UNLOCK_DATA()	ReleaseSemaphore(rwl->data_lock, 1, NULL)
*/

/*
int valid_rwlock(_rwlock_st* rwl)
{
	
}
*/

_rwlock_t create_rwlock()
{
	_rwlock_st* rwl = (_rwlock_st* )malloc(sizeof(_rwlock_st));
	if (rwl == NULL)
		return NULL;
	rwl->data_lock = CreateSemaphore(NULL, 1, 1, NULL);
	if (rwl->data_lock == NULL) {
		delete rwl;
		return NULL;
	}

	rwl->count_lock = CreateMutex(NULL, FALSE, NULL);
	if (rwl->count_lock == NULL) {
		CloseHandle(rwl->data_lock);
		delete rwl;
		return NULL;
	}
	
	rwl->read_count = 0;

	return (_rwlock_t )rwl;
}

void destroy_rwlock(_rwlock_t rwlock)
{
	if (rwlock) {
		_rwlock_st* rwl = (_rwlock_st* )rwlock;
		CloseHandle(rwl->data_lock);
		CloseHandle(rwl->count_lock);
		free(rwl);
	}
}

int rwlock_locked_read(_rwlock_t rwlock)
{
	int r = rwlock_locked_write(rwlock);
	if (r)
		return r;
	else {
		_rwlock_st* rwl = (_rwlock_st* )rwlock;
		return rwl->read_count > 0;
	}
}

int rwlock_locked_write(_rwlock_t rwlock)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;
	if (rwl->read_count)
		return 0;

	if (WaitForSingleObject(rwl->data_lock, 0) != 
		WAIT_OBJECT_0) {
		return 1;
	}

	ReleaseSemaphore(rwl->data_lock, 1, NULL);
	return 0;
}

int rwlock_lock_read(_rwlock_t rwlock, 
					 uint32 timeo /* = RWLOCK_DEADLOCK_TIMEO */)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;
	int result = 1;
	
	if (WaitForSingleObject(rwl->count_lock, INFINITE) != WAIT_OBJECT_0)
		return 0;

	if (++ rwl->read_count == 1) {
		if (WaitForSingleObject(rwl->data_lock, timeo) != WAIT_OBJECT_0) {
			-- rwl->read_count;
			result = 0;
		} 

	}

	ReleaseMutex(rwl->count_lock);
	return result;
}

int rwlock_lock_write(_rwlock_t rwlock, 
					  uint32 timeo /* = RWLOCK_DEADLOCK_TIMEO */)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	DWORD r = WaitForSingleObject(rwl->data_lock, timeo);
	if (r == WAIT_OBJECT_0) {
		
		return 1;
	} else {


		return 0;
	}
}

int rwlock_unlock_read(_rwlock_t rwlock)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	if (WaitForSingleObject(rwl->count_lock, INFINITE) != WAIT_OBJECT_0)
		return 0;

	if (rwl->read_count == 0) {
		ReleaseMutex(rwl->count_lock);
		return 0;
	}
	
	if (-- rwl->read_count == 0) {
		ReleaseSemaphore(rwl->data_lock, 1, NULL);
	}

	ReleaseMutex(rwl->count_lock);

	return 1;
}

int rwlock_unlock_write(_rwlock_t rwlock)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	return ReleaseSemaphore(rwl->data_lock, 1, NULL);
}
