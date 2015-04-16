#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <pthread.h>

#ifdef WIN32
#include <windows.h>
#else
#ifdef LINUX
#include <wintype.h>
#else
#error This is a unknown platform
#endif
#endif

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

bool _pthread_terminate(pthread_t* pthread, void* status);
int _pthread_mutexattr_setname_np(pthread_mutexattr_t* attr, const char* name);

inline LONG NativeInterlockedIncrement(LONG volatile* pnCount);
inline LONG NativeInterlockedDecrement(LONG volatile* pnCount);

void XDbgPrintString(LPCTSTR lpszString);

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _PLATFORM_H_
