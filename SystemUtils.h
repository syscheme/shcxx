#ifndef __SYSTEM_UTILS__
#define __SYSTEM_UTILS__

#include "ZQ_common_conf.h"
#ifdef ZQ_OS_LINUX 
#include <semaphore.h>
#endif
#include <string>
#include <time.h>
#include <math.h>
#include <errno.h>

namespace SYS {

enum ERRTYPE { NORM, RTLD, SOCK };

void ZQ_COMMON_API sleep(long millisec);

uint64 ZQ_COMMON_API getTickCount();

void ZQ_COMMON_API *getProcAddr(void*, const char*);

uint32 ZQ_COMMON_API getCurrentThreadID(void);

uint32 ZQ_COMMON_API getLastErr(const ERRTYPE=NORM);

std::string ZQ_COMMON_API getErrorMessage(const ERRTYPE=NORM);

class ZQ_COMMON_API SingleObject {
public:
    SingleObject();
    virtual ~SingleObject();
    enum STATE {SIGNALED, TIMEDOUT, UNKNOWN};
    STATE wait(timeout_t timeout=TIMEOUT_INF);
    void signal();
private:
#ifdef ZQ_OS_MSWIN
    HANDLE _handle;
#else
    sem_t _handle;
#endif
};


class ZQ_COMMON_API TimeStamp {

public:

    enum TYPE {LOCAL, UTC};
    explicit TimeStamp(enum TYPE type = LOCAL);
    uint16 year, month, day;
    uint16 hour, minute, second, millisecond;

private:
#ifdef ZQ_OS_LINUX
    struct timespec t_;
#else
    SYSTEMTIME t_;
#endif
};

}

#endif
