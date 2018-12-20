#include <assert.h>
#include <tchar.h>

#ifndef _XCOMM_H_
#define _XCOMM_H_
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#ifndef _xassert
#define _xassert(e)		assert((e))
#endif

long InterlockedIncrement(long volatile* pnCount);
long InterlockedDecrement(long volatile* pnCount);

bool IsDbgTraceEnabled();
void EnableDbgTrace(bool bEnabled);

#ifdef _DEBUG
#define DbgTrace	XDbgTrace
#else
#define DbgTrace	
#endif

void XDbgTrace(LPCTSTR lpszFormat, ...);

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif #ifndef _XCOMM_H_
