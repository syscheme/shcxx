#include "SystemUtils.h"

#ifdef ZQ_OS_LINUX
#include <sys/times.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)

#endif


namespace SYS {

#ifdef ZQ_OS_MSWIN

void sleep(long millisec) {
    Sleep(millisec);
}

uint64 getTickCount() {
    return GetTickCount();
}

void* getProcAddr(void* handle, const char* routine) {
    return GetProcAddress((HMODULE)handle, routine);
}

uint32 getCurrentThreadID(void){
	return GetCurrentThreadId();
}

uint32 getLastErr(const ERRTYPE type) {
	if(type == SOCK) {
		return WSAGetLastError();
	}	
	return GetLastError();
}

std::string getErrorMessage(const ERRTYPE type) {
    char msg[1024];
    memset(msg, '\0', 1024);

	DWORD err = 0;
	if(type == SOCK) {
		err = WSAGetLastError();
	}
	else {
		err = GetLastError();
	}
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
		err,
        0,
        msg,
        1024,
        NULL);
    msg[std::strlen(msg)-1] = '\0';

    return msg;
}

#else

void sleep(long millisec) {
    struct timespec spec;
    spec.tv_sec = millisec/1000;
    spec.tv_nsec = (millisec%1000)*1000*1000;
    nanosleep(&spec, 0);
}

uint64 getTickCount() {
    static long milliPerTick = 1000/sysconf(_SC_CLK_TCK);
    return times(0)*milliPerTick;
}
#ifndef ZQ_COMMON_ANDROID
void* getProcAddr(void* handle, const char* routine) {
    return dlsym(handle, routine);
}

std::string getErrorMessage(const ERRTYPE type) {
    if(type == RTLD) {
        return dlerror();
    }

	char msg[1024] = {0};
	strerror_r(errno, msg, 1024);
	return msg;
}
#endif

uint32 getCurrentThreadID(void){
#ifndef ZQ_COMMON_ANDROID
	return (uint32)gettid();
#else
	return (uint32)pthread_self();
#endif
}

uint32 getLastErr(const ERRTYPE type) {
    return errno;
}


#endif

// implemention of SingleObject
SingleObject::SingleObject() {
#ifdef ZQ_OS_MSWIN
    _handle = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
    sem_init(&_handle, 0, 0);
#endif
}

SingleObject::~SingleObject() {
    signal();
#ifdef ZQ_OS_MSWIN
    CloseHandle(_handle);
#else
    sem_destroy(&_handle);
#endif
}


SingleObject::STATE SingleObject::wait(timeout_t timeout) {
#ifdef ZQ_OS_MSWIN
    if(timeout == TIMEOUT_INF) {
        timeout = INFINITE;
    }
    DWORD res = WaitForSingleObject(_handle, timeout);
    if(res == WAIT_OBJECT_0) {
        return SIGNALED;
    }
    if(res == WAIT_TIMEOUT) {
        return TIMEDOUT;
    }
    return UNKNOWN;
#else
    struct timespec ts;
    if(timeout != TIMEOUT_INF) {
        clock_gettime(CLOCK_REALTIME, &ts);

        long long nsec = ts.tv_nsec + timeout*1000000LL;
        ts.tv_sec += nsec/1000000000L;
        ts.tv_nsec = nsec%1000000000L;
    }

wait:
    int res = 0;
    if(timeout == TIMEOUT_INF) {
        res = sem_wait(&_handle);
    }
    else { 
        res = sem_timedwait(&_handle, &ts);
    }
    if(!res) {
        return SIGNALED;
    }
    if(res == (-1) && errno == EINTR) {
        goto wait;
    }
    if(res == (-1) && errno == ETIMEDOUT) {
        return TIMEDOUT;
    }
    return UNKNOWN; 
#endif
}

void SingleObject::signal() {
#ifdef ZQ_OS_MSWIN
    SetEvent(_handle);
#else
    sem_post(&_handle);
#endif
}

// implemention of TimeStamp
TimeStamp::TimeStamp(enum TYPE type):
year(0),month(0),day(0),hour(0),
minute(0),second(0),millisecond(0) {

#ifdef ZQ_OS_LINUX
    clock_gettime(CLOCK_REALTIME, &t_);
    struct tm time;
#endif

    if(type == LOCAL) {
#ifdef ZQ_OS_LINUX
        localtime_r(&t_.tv_sec, &time);
#else			
        GetLocalTime(&t_);
#endif
    }
    else {
#ifdef ZQ_OS_LINUX
        gmtime_r(&t_.tv_sec, &time);
#else
        GetSystemTime(&t_);
#endif
    }

#ifdef ZQ_OS_LINUX
    year = time.tm_year+1900;
    month = time.tm_mon+1;
    day = time.tm_mday;
    hour = time.tm_hour;
    minute = time.tm_min;
    second = time.tm_sec;
    millisecond = (unsigned short)round(((double)t_.tv_nsec/(1000*1000)));
#else
    year = t_.wYear;
    month = t_.wMonth;
    day = t_.wDay;
    hour = t_.wHour;
    minute = t_.wMinute;
    second = t_.wSecond;
    millisecond = t_.wMilliseconds;
#endif
}

}

// vim: ts=4 sw=4 bg=dark nu
