#include "rwlock.h"

extern "C"
{
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/timeb.h>
}

typedef struct {
	pthread_mutex_t		count_lock;//count mutex
	sem_t				data_lock; //date semaphore
	volatile long		read_count;
	
} _rwlock_st;


_rwlock_t create_rwlock()
{
	_rwlock_st* rwl = (_rwlock_st* )malloc(sizeof(_rwlock_st));
	if (rwl == NULL)
		return NULL;

	pthread_mutexattr_t muattr;
	pthread_mutexattr_init(&muattr);
	pthread_mutexattr_settype(&muattr,PTHREAD_MUTEX_RECURSIVE);
	
	if(pthread_mutex_init(&rwl->count_lock,&muattr) != 0)//set recursive mutex
		return NULL;

	if(sem_init(&rwl->data_lock,0,1) != 0)//initialize sem value is 1
	{
		pthread_mutex_destroy(&rwl->count_lock);
		return NULL;
	}

	rwl->read_count = 0;

	return (_rwlock_t )rwl;
}

void destroy_rwlock(_rwlock_t rwlock)
{
	if(rwlock)
	{
		_rwlock_st* rwl = (_rwlock_st* )rwlock;
		try
		{
			pthread_mutex_destroy(&rwl->count_lock);
			sem_destroy(&rwl->data_lock);
			free(rwl);
		}
		catch(...){}
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

	int res = sem_trywait(&rwl->data_lock);

	if(res != 0)//error 
		return 1;

	sem_post(&rwl->data_lock);

	return 0;
}

int rwlock_lock_read(_rwlock_t rwlock, uint32 timeo /* = RWLOCK_DEADLOCK_TIMEO */)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;
	int result = 1;	

	if(0 != pthread_mutex_lock(&rwl->count_lock))
		return 0;

	if(++rwl->read_count == 1)
	{
		int res = 0;
		struct timespec ts;
		struct timeb tb;
		
		ftime(&tb);
		int sec = timeo/1000;
		int msec = timeo%1000;
		
		tb.time += sec;	
		tb.millitm += msec;
		if(tb.millitm > 999)
		{
			tb.millitm -= 1000;
			tb.time += 1;
		}
		ts.tv_sec = tb.time;
		ts.tv_nsec = tb.millitm * 1000000;
		res = sem_timedwait(&rwl->data_lock,&ts);

		if(res != 0)//time out
		{
			-- rwl->read_count;
			result = 0;
		}
	}

	pthread_mutex_unlock(&rwl->count_lock);

	return result;
}

int rwlock_lock_write(_rwlock_t rwlock, uint32 timeo /* = RWLOCK_DEADLOCK_TIMEO */)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	int res = 0;
	struct timespec ts;
	struct timeb tb;
	
	ftime(&tb);
	int sec = timeo/1000;
	int msec = timeo%1000;
	
	tb.time += sec;	
	tb.millitm += msec;
	if(tb.millitm > 999)
	{
		tb.millitm -= 1000;
		tb.time += 1;
	}
	ts.tv_sec = tb.time;
	ts.tv_nsec = tb.millitm * 1000000;
	res = sem_timedwait(&rwl->data_lock,&ts);

	if(res == 0)
		return 1;
	else
		return 0;

}

int rwlock_unlock_read(_rwlock_t rwlock)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	if(0 != pthread_mutex_lock(&rwl->count_lock))
		return 0;

	if(rwl->read_count == 0)
	{
		pthread_mutex_unlock(&rwl->count_lock);
		return 0;
	}

	if(-- rwl->read_count == 0)
		sem_post(&rwl->data_lock);

	pthread_mutex_unlock(&rwl->count_lock);

	return 1;
}

int rwlock_unlock_write(_rwlock_t rwlock)
{
	_rwlock_st* rwl = (_rwlock_st* )rwlock;

	return sem_post(&rwl->data_lock);
}
