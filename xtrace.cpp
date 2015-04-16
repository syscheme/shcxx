/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * xtrace.cpp by Hector J. Rivas, torjo2k@hotmail.com from "ExtendedTrace"
 * by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
 * 
 * A Win32 VC++ 6.0 implementation of the __FUNCTION__ macro that works for me.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if defined(_DEBUG) && defined(WIN32)

#include "xtrace.h"
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <ImageHlp.h>

void	XtraceInitSymPath(PSTR lpszSymbolPath, PCSTR lpszIniPath, BOOL bSysPath);
BOOL	XtraceGetFuncInfo(ULONG fnAddress, ULONG stackAddress, LPTSTR lpszSymbol);

// Unicode safe char* -> TCHAR* conversion
void XtracePCSTR2LPTSTR(PCSTR lpszIn, LPTSTR lpszOut)
{
#if defined(UNICODE) || defined(_UNICODE)
	
	ULONG index = 0; 
	PCSTR lpAct = lpszIn;
	
	for(;; lpAct++)
	{
		lpszOut[index++] = (TCHAR)(*lpAct);
		
		if (*lpAct == 0) break;
	} 
#else
	strcpy(lpszOut, lpszIn);
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * InitSymPath(): figure  out  the  path for the symbol files; the search path is:
 *
 * 		. +
 *		__FILE__ (path) + Debug +
 * 		%_NT_SYMBOL_PATH% +
 * 		%_NT_ALTERNATE_SYMBOL_PATH% +
 * 		%SYSTEMROOT% +
 * 		%SYSTEMROOT%\System32 +
 * 		lpszIniPath
 *
 * NOTES: There is no size check for lpszSymbolPath. If you want to limit the macro to
 * symbols in your debug executable, you can omit the environment variables (default).
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void XtraceInitSymPath(PSTR lpszSymbolPath, PCSTR lpszIniPath, BOOL bSysPath)
{
	CHAR lpszPath[XTRACE_BUFFERSIZE] = "";
	CHAR lpszTemp[XTRACE_BUFFERSIZE] = "";
	
	// create the default path
	strcpy(lpszSymbolPath, ".;");

	// get the current path
	sprintf(lpszTemp, __FILE__);

	strcpy(lpszPath, strrev(strchr(strrev(lpszTemp), '\\')));

	strcat(lpszPath, "Debug");

	strcat(lpszSymbolPath, lpszPath);

	if (bSysPath)
	{
		// environment variable _NT_SYMBOL_PATH
		if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", lpszPath, 
			XTRACE_BUFFERSIZE))
		{
			strcat(lpszSymbolPath, ";");
			strcat(lpszSymbolPath, lpszPath);
		}
		
		// environment variable _NT_ALTERNATE_SYMBOL_PATH
		if (GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", lpszPath, 
			XTRACE_BUFFERSIZE))
		{
			strcat(lpszSymbolPath, ";");
			strcat(lpszSymbolPath, lpszPath);
		}
		
		// environment variable SYSTEMROOT
		if (GetEnvironmentVariableA("SYSTEMROOT", lpszPath, 
			XTRACE_BUFFERSIZE))
		{
			strcat(lpszSymbolPath, ";");
			strcat(lpszSymbolPath, lpszPath);
			
			// SYSTEMROOT\System32
			strcat(lpszSymbolPath, ";");
			strcat(lpszSymbolPath, lpszPath);
			strcat(lpszSymbolPath, "\\System32");
		}
	}
	
	// Add any user defined path
	if (lpszIniPath != NULL)
	{
		if (lpszIniPath[0] != '\0')
		{
			strcat(lpszSymbolPath, ";");
			strcat(lpszSymbolPath, lpszIniPath);
		}
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * InitSymInfo(): initializes the symbol files
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BOOL XtraceInitSymInfo(PCSTR lpszInitialSymbolPath, BOOL bSysPath)
{
	CHAR	lpszSymbolPath[XTRACE_BUFFERSIZE];
	
	DWORD	symOptions = SymGetOptions();
	
	// set current image help API options; according to the SDK docs, with
	// SYMOPT_DEFERRED_LOADS: "Symbols are not loaded until a reference is made
	// requiring the symbols be loaded. This is the fastest, most efficient way to use
	// the symbol handler.". SYMOPT_UNDNAME is excluded to do the undecoration
	// ourselves.
	symOptions |= SYMOPT_DEFERRED_LOADS; 
	symOptions &= ~SYMOPT_UNDNAME;
	
	SymSetOptions(symOptions);
	
	// get the search path for the symbol files
	XtraceInitSymPath(lpszSymbolPath, lpszInitialSymbolPath, bSysPath);
	
	return SymInitialize(GetCurrentProcess(), lpszSymbolPath, TRUE);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * GetFuncInfo(): Get function prototype from address and stack address
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BOOL XtraceGetFuncInfo(ULONG fnAddress, ULONG stackAddress, LPTSTR lpszSymbol)
{
	BOOL ret = FALSE;
	
	DWORD dwDisp = 0, dwSymSize = 10000;
	
	TCHAR lpszUDSymbol[XTRACE_BUFFERSIZE] = _T("?");
	
	CHAR lpszANSIUDSymbol[XTRACE_BUFFERSIZE] = "?";
		
	PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, dwSymSize);

	ZeroMemory(pSym, dwSymSize);
	
	pSym->SizeOfStruct  = dwSymSize;
	pSym->MaxNameLength = dwSymSize - sizeof(IMAGEHLP_SYMBOL);

	// Set the default to unknown
	_tcscpy(lpszSymbol, _T("?"));

	// Get symbol info
	if (SymGetSymFromAddr(GetCurrentProcess(), (ULONG)fnAddress, &dwDisp, pSym))
	{
		// Make the symbol readable for humans
		UnDecorateSymbolName(pSym->Name,
							 lpszANSIUDSymbol,
							 XTRACE_BUFFERSIZE,
							 UNDNAME_COMPLETE			  |
							 UNDNAME_NO_THISTYPE		  |
							 UNDNAME_NO_SPECIAL_SYMS	  |
							 UNDNAME_NO_MEMBER_TYPE		  |
							 UNDNAME_NO_MS_KEYWORDS		  |
							 UNDNAME_NO_ACCESS_SPECIFIERS |
							 UNDNAME_NO_ARGUMENTS);

		// Symbol information is ANSI string
		XtracePCSTR2LPTSTR(lpszANSIUDSymbol, lpszUDSymbol);

		lpszSymbol[0] = _T('\0');

		_tcscat(lpszSymbol, lpszUDSymbol);
   
		ret = TRUE;
	}

	GlobalFree(pSym);

	return ret;
}

#ifndef _MANUAL_INIT

class InitXTrace {
public:
	InitXTrace()
	{
		XtraceInitSymInfo();
	}

	~InitXTrace()
	{
		XtraceKillSymInfo();
	}

} initXTrace;

#endif // #ifndef _MANUAL_INIT

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * GetFuncName(): return the undecorated function name
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

LPCTSTR XtraceGetFuncName()
{
	BOOL           bResult = FALSE;

	STACKFRAME     callStack;

	TCHAR          lpszFnInfo[XTRACE_BUFFERSIZE];
	
	HANDLE         hProcess = GetCurrentProcess();
	HANDLE         hThread  = GetCurrentThread();

	// initialize a stack frame struct in preparation to walk the stack
	ZeroMemory(&callStack, sizeof(callStack));


	__asm {
		call	getEIP;
getEIP:
		pop		callStack.AddrPC.Offset;
		mov		callStack.AddrStack.Offset, Esp;
		mov		callStack.AddrFrame.Offset, Ebp;
	}
	
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrFrame.Mode   = AddrModeFlat;

	// obtain a stack trace of the calling function (i.e., omit this one)
	for (ULONG n = 0; n < 2; n++) 
	{
		bResult = StackWalk(IMAGE_FILE_MACHINE_I386,
							hProcess,
							hThread,
							&callStack,
							NULL,
							NULL,
							SymFunctionTableAccess,
							SymGetModuleBase,
							NULL);
	}

	if (bResult && callStack.AddrFrame.Offset != 0) 
	{
		XtraceGetFuncInfo(callStack.AddrPC.Offset, callStack.AddrFrame.Offset, lpszFnInfo);

		// from now on its all personal display preferences with string manipulation

		// tokenize the undecorated returned symbol to omit the class name
		CHAR* lpszToken = strtok(lpszFnInfo, "::");
		CHAR  lpszLast[XTRACE_BUFFERSIZE] = "";

		while (lpszToken != NULL)
		{
			strcpy(lpszLast, lpszToken);
			lpszToken = strtok(NULL, "::");
		}

		// append a delimiter, so that our display in printf instructions is 
		// 'functionname: message' for debug builds and 'message' for release builds,
		// using the format string "%smessage" and __FUNCTION__ as an argument
		strcat(lpszLast, ": ");

		return lpszLast;
	}

	return "";
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * KillSymInfo(): uninitialize the loaded symbol files
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
BOOL XtraceKillSymInfo() { return SymCleanup(GetCurrentProcess()); }

#endif //_DEBUG && WIN32

