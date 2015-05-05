#include <windows.h>
#include <pthread.h>
#include <assert.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

struct ptw32_thread_t_ {
#ifdef _UWIN
  DWORD dummy[5];
#endif
  DWORD thread;
  HANDLE threadH;		/* Win32 thread handle - POSIX thread is invalid if threadH == 0 */
};

HANDLE getWin32ThreadHandle(pthread_t* pthread)
{
	ptw32_thread_t_* ptw32_thread = (ptw32_thread_t_* )pthread->p;
	assert(ptw32_thread->threadH != NULL);
	return ptw32_thread->threadH;
}

bool _pthread_terminate(pthread_t* pthread, void* status)
{
	HANDLE hThread = getWin32ThreadHandle(pthread);
	if (hThread == NULL)
		return false;

	if (TerminateThread(hThread, (unsigned long)status) == 0) {
		return false;
	}

	return true;
}

int _pthread_mutexattr_setname_np(pthread_mutexattr_t* attr, const char* name)
{
	attr; name;
	return EINVAL;
}

LONG NativeInterlockedIncrement(LONG volatile* pnCount)
{
	return ::InterlockedIncrement((LONG* )pnCount);
}

LONG NativeInterlockedDecrement(LONG volatile* pnCount)
{
	return ::InterlockedDecrement((LONG* )pnCount);
}

void XDbgPrintString(LPCTSTR lpszString)
{
	::OutputDebugString(lpszString);
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
