
#include <pthread.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#ifndef SIGKILL
#define SIGKILL			9 
#endif

bool _pthread_terminate(pthread_t* pthread, void* )
{
	int err = pthread_kill(*pthread, SIGKILL);
	if (0 != err)
		return false;

	return true;
}

int _pthread_mutexattr_setname_np(pthread_mutexattr_t* attr, const char* name)
{
#if 0
	return pthread_mutexattr_setname_np(attr, name);
#endif
	return 0;
}

/*
MOV ECX,DWORD PTR pnCount
MOV EAX,1
LOCK XADD DWORD PTR DS:[ECX],EAX         ; Ëø¶¨Ç°×º
INC EAX
*/

LONG NativeInterlockedIncrement(LONG volatile* pnCount)
{
	return ++ *pnCount;
}

LONG NativeInterlockedDecrement(LONG volatile* pnCount)
{
	return -- *pnCount;
}

void XDbgPrintString(LPCTSTR lpszString)
{
	printf(lpszString);
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
