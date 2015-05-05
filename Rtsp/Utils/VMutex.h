#if !defined(_VMUTEX_H_)
#define _VMUTEX_H_

#include "../../ZQ_common_conf.h"

class VMutex {
public:
	VMutex();
	virtual ~VMutex();

	void lock();
	
	void unlock();

protected:

#ifdef ZQ_OS_MSWIN
	HANDLE	_mutex;
#else
	pthread_mutexattr_t _muattr;
    pthread_mutex_t _mutex;
#endif
		
};


#endif // #if !defined(_VMUTEX_H_)
