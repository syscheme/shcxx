   // ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: MiniDump.h,v 1.20 2004/07/26 10:56:35 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : A mini dump for the application crash
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/MiniDump.h $
// 
// 2     12/19/12 3:46p Hui.shao
//  PRINTFLIKE
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 9     09-12-22 14:56 Jie.zhang
// merge from TianShan1.10
// 
// 9     09-11-17 15:06 Jie.zhang
// add DumpState for crash dump generating
// 
// 8     09-07-30 18:24 Xiaohui.chai
// add handler for: invalid parameter, pure call
// 
// 7     08-06-13 17:47 Jie.zhang
// 
// 6     07-08-15 15:09 Jie.zhang
// merge
// 
// 6     07-07-26 10:53 Jie.zhang
// add member function "resetUnhandledExceptionFilter"
// 
// 5     07-04-19 15:25 Cary.xiao
// ===========================================================================
/**** usage example ****
void IllegalOp1()
{
    _asm {cli};
}

void IllegalOp2()
{
	char* p= NULL;

	p[0] = 0;
	int a = 1;
}

ZQ::common::MiniDump dumper("c:\\temp");

void main()
{
	IllegalOp1();
}
************************/

#ifndef __MINIDUMP_H__
#define __MINIDUMP_H__

#include "ZQ_common_conf.h"
#include "Locks.h"
extern "C"
{
#ifdef ZQ_OS_MSWIN	
	#include <dbghelp.h>
#else
	#include <cerrno>
	#include <fcntl.h>
	#include <semaphore.h>
#endif 
}


#define MINIDUMP_SYMBOL_PATH		512
#define MINIDUMP_SYMBOL_SIZE		512

namespace ZQ {
namespace common {

class ZQ_COMMON_API MiniDump;

// -----------------------------
// class MiniDump
// -----------------------------
class MiniDump
{
public:

		bool setDumpPath(TCHAR* DumpPath);

#ifdef ZQ_OS_MSWIN

	typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

	MiniDump(TCHAR* DumpPath =NULL, TCHAR* ModuleName =NULL);

	void setServiceName(const TCHAR* szServiceName);
	

	void enableFullMemoryDump(BOOL bEnable){ _bEnableFullMemoryDump = bEnable;}

	// because MiniDump set the the unhandled exception filter at the constructor, 
	// if some other code has changed the unhandled exception filter, MiniDump would not work
	// this function is to reset the unhandled exception filter to the one in MiniDump
	void resetUnhandledExceptionFilter();

	typedef void (WINAPI *ExceptionCallBack)(DWORD ExceptionCode, PVOID ExceptionAddress);
	
	//this callback will be called after the minidump created
	void setExceptionCB(ExceptionCallBack cb);

	// added by Cary for debugging symbol
	void initSym(LPSTR symbolPath = NULL, bool sysSym = true);
	bool resolveSym(const void* addr, LPSTR symName, size_t symLen, 
		LPSTR fileName, size_t fileNameLen, int* line);
	
protected:

#ifdef UNICODE
	static void trace(const TCHAR *fmt, ...);
#else
	static void trace(const TCHAR *fmt, ...) PRINTFLIKE(1, 2);
#endif // UNICODE
	static LONG WINAPI _OnUnhandledException( struct _EXCEPTION_POINTERS *pExceptionInfo );
	static TCHAR _moduleName[128], _dumpPath[_MAX_PATH];
	static TCHAR _moduleFullPath[_MAX_PATH];
	static TCHAR _serviceName[128];
	static LONG _hOldFilter;
	static BOOL _bEnableFullMemoryDump;
	static ExceptionCallBack		_pExceptionCB;
	static ZQ::common::Mutex		_lock;

    static void _OnInvalidParameter(const wchar_t* expression,
                                    const wchar_t* function, 
                                    const wchar_t* file, 
                                    unsigned int line, 
                                    uintptr_t pReserved);
    static void _OnPureCall();

#else

	/*void terminate(int signal, siginfo_t* info, void* data);

	void coredump(int signal, siginfo_t* info, void* data);*/

	bool initSignals(bool shellmode);

#endif
};

} // namespace common
} // namespace ZQ

#endif // __MINIDUMP_H__
