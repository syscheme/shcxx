#include "VMutex.h"

#ifdef ZQ_OS_MSWIN

VMutex::VMutex()
{
	_mutex = CreateMutex(NULL, FALSE, NULL);
}

VMutex::~VMutex()
{
	CloseHandle(_mutex);
}

void VMutex::lock()
{
	WaitForSingleObject(_mutex, INFINITE);
}
	
void VMutex::unlock()
{
	ReleaseMutex(_mutex);
}

#else

VMutex::VMutex() {
	pthread_mutexattr_init(&_muattr);
	pthread_mutexattr_settype(&_muattr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_mutex, &_muattr);
}

VMutex::~VMutex() {
	pthread_mutexattr_destroy(&_muattr);
    pthread_mutex_destroy(&_mutex);
}

void VMutex::lock() {
    pthread_mutex_lock(&_mutex);
}

void VMutex::unlock() {
    pthread_mutex_unlock(&_mutex);
}

#endif
