#include <xcomm.h>
#include "platform/platform.h"
#include <stdarg.h>
#include <stdio.h>
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
#ifndef DEFAULT_TRACE_SWITCH
#define DEFAULT_TRACE_SWITCH	true
#endif
// would you like to print some dubugging information?
bool g_bDbgTraceEnabled = DEFAULT_TRACE_SWITCH;
long InterlockedIncrement(long volatile* pnCount)
{
	return NativeInterlockedIncrement(pnCount);
}

long InterlockedDecrement(long volatile* pnCount)
{
	return NativeInterlockedDecrement(pnCount);
}

bool IsDbgTraceEnabled()
{
	return g_bDbgTraceEnabled;
}

void EnabledDbgTrace(bool bEnabled)
{
	g_bDbgTraceEnabled = bEnabled;
}

void XDbgTrace(LPCTSTR lpszFormat, ...)
{
	if (!g_bDbgTraceEnabled)
		return;
	TCHAR szBuff[1028];
	va_list argList;
	va_start(argList, lpszFormat);
	_vstprintf(szBuff, lpszFormat, argList);
	va_end(argList);
	XDbgPrintString(szBuff);
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
