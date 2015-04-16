
#if defined(_DEBUG)

#include "cpLog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
extern "C" void __stdcall OutputDebugStringA(const char* );
#endif

void cpLog (int pri, const char *fmt, ...)
{
	return;

	char buf[1025+128];
	ZeroMemory(buf,sizeof(buf));

	if (pri > LAST_PRIORITY)
		return;

	va_list vlist;
	va_start(vlist, fmt);

	static char* dbgLevel[] = {
		"EMERG:", 
		"ALERT:", 
		"CRIT:", 
		"ERR:", 
		"WARNING:", 
		"NOTICE:", 
		"INFO:", 
		"DEBUG:", 
		"DEBUG_STACK:", 
		"DEBUG_OPER:", 
		"DEBUG_HB:", 
	};
	
	strcpy(buf, "[RtspParser]");
	strcat(buf, dbgLevel[pri]);
	int ilen = strlen(buf);
	int iRet = _vsnprintf(&buf[ilen],1024, fmt, vlist);
	//strcat(buf, "\n");
	buf[ilen+iRet] ='\n';
	
#ifdef _WIN32
//	OutputDebugStringA(buf);
#else
	fprintf(stderr, buf);
#endif
}

#endif // #ifdef _DEBUG

