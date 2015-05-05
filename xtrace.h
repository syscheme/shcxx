/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * xtrace.h by Hector J. Rivas, torjo2k@hotmail.com from "ExtendedTrace"
 * by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
 * 
 * A Win32 VC++ 6.0 implementation of the __FUNCTION__ macro that works for me.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _XTRACE_H_
#define _XTRACE_H_

#if defined(_DEBUG) && defined(WIN32)

#include "ZQ_common_conf.h"
#include <windows.h>
#include <tchar.h>

#pragma comment(lib, "imagehlp.lib")
#pragma warning(disable : 4172)			// returning address of a temp

#define XTRACE_BUFFERSIZE	512

ZQ_COMMON_API LPCTSTR XtraceGetFuncName();
#define __FUNCTION__		XtraceGetFuncName()
BOOL	XtraceInitSymInfo(PCSTR lpszInitialSymbolPath = NULL, BOOL bSysPath = FALSE);
BOOL	XtraceKillSymInfo();

#else // #if defined(_DEBUG) && defined(WIN32)

#define XtraceInitSymInfo
#define XtraceKillSymInfo

// BOOL XtraceKillSymInfo() {};

#define __FUNCTION__		_T("")

#endif // #if defined(_DEBUG) && defined(WIN32)

#endif // #ifndef _XTRACE_H_
