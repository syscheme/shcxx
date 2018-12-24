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
// Ident : $Id: MiniDump.cpp,v 1.20 2004/07/26 10:56:35 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : A mini dump for the application crash
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/MiniDump.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 17    09-12-22 14:56 Jie.zhang
// merge from TianShan1.10
// 
// 16    09-11-03 18:42 Xiaohui.chai
// merge from 1.10
// add event log trace
// 
// 15    09-11-03 13:06 Xiaohui.chai
// invoke callback before dump
// 
// 14    09-11-03 12:18 Xiaohui.chai
// always generate a dump without the full memory
// 
// 13    09-07-30 18:16 Xiaohui.chai
// add handler for: invalid parameter, pure call
// 
// 12    08-06-13 17:47 Jie.zhang
// 
// 11    08-02-21 12:35 Build
// 
// 10    08-02-19 15:54 Hui.shao
// 
// 9     07-08-15 15:09 Jie.zhang
// merge
// 
// 9     07-07-26 10:53 Jie.zhang
// add member function "resetUnhandledExceptionFilter"
// 
// 8     07-06-15 14:46 Guan.han
// 
// 7     07-04-26 12:07 Cary.xiao
// 
// 6     07-04-19 15:25 Cary.xiao
// 
// 5     05-11-24 14:31 Jie.zhang
// add full memory dump
// 
// 4     05-09-09 16:37 Jie.zhang
// 
// 3     05-09-09 11:12 Jie.zhang
// 
// 2     05-09-09 9:37 Jie.zhang
// 
// 1     8/25/05 5:25p Hui.shao
// ===========================================================================
#include "MiniDump.h"
#include "limits.h"
#include <stdio.h>
#include <assert.h>
#include "Log.h"
#ifdef ZQ_OS_MSWIN
#include <tchar.h>
#include "dumpstate.h"
#include "dumpstate.cpp"
#else
#include <sys/resource.h>
#endif

#ifdef ZQ_OS_LINUX
static void  terminate(int signal, siginfo_t* info, void* data) 
{
	//sem_post(stopEvent);
}

static void  coredump(int signal, siginfo_t* info, void* data) {
	glog.flush();

	struct rlimit rlim;

	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);

	/* ignore any error here */
	int fd = open("/proc/sys/kernel/core_pattern", O_WRONLY); 
	if(fd != (-1))
		write(fd, "core.%s.%e_%p.%t", sizeof("core.%s.%e_%p.%t")-1);


	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_handler=SIG_DFL;

	sigaction(signal, &act, 0);
	raise(signal);
}
#endif


namespace ZQ {
	namespace common {

#ifdef ZQ_OS_MSWIN
		//check the path, if not exist then create it, if create fail then return false
		static bool validatePath(const TCHAR *     wszPath )
		{
			if (-1 != ::GetFileAttributes(wszPath))
				return true;
			
			DWORD dwErr = ::GetLastError();
			if ( dwErr == ERROR_PATH_NOT_FOUND || dwErr == ERROR_FILE_NOT_FOUND )
			{
				if (!::CreateDirectory(wszPath, NULL))
				{
					dwErr = ::GetLastError();
					if ( dwErr != ERROR_ALREADY_EXISTS)
					{
						return false;
					}
				}
			}
			else
			{
				return false;
			}
			
			return true;
		}


#define DBGHELP_DLL _T("DBGHELP.DLL")
#define DUMP_METHOD "MiniDumpWriteDump"

		LONG MiniDump::_hOldFilter = NULL;
		TCHAR MiniDump::_moduleName[128], MiniDump::_dumpPath[_MAX_PATH], MiniDump::_moduleFullPath[_MAX_PATH];
		TCHAR MiniDump::_serviceName[128] = {0};
		MiniDump::ExceptionCallBack	MiniDump::_pExceptionCB = NULL;
		BOOL MiniDump::_bEnableFullMemoryDump = FALSE;
		ZQ::common::Mutex MiniDump::_lock;

		void MiniDump::trace(const TCHAR *fmt, ...)
		{
#ifdef _DEBUG
			TCHAR msg[2048];
			va_list args;
			
			va_start(args, fmt);
			_vstprintf(msg, fmt, args);
			va_end(args);
			
			/*::MessageBox(NULL, msg, _moduleName, MB_OK);*/
#endif // _DEBUG
		}

		MiniDump::MiniDump(TCHAR* DumpPath, TCHAR* ModuleName)
		{
			if (NULL != _hOldFilter) // already set
				return;
			
			::GetModuleFileName(NULL, _moduleFullPath, _MAX_PATH);
			
			TCHAR* pos = _tcsrchr(_moduleFullPath, _T('\\'));
			if (ModuleName == NULL)
			{
				_tcscpy(_moduleName, (NULL != pos) ? pos+1 : _moduleFullPath);
				pos  = _tcsrchr(_moduleName, _T('.'));	//remove ".exe"
				if (pos)
					*pos = _T('\0');
			}
			else
				_tcscpy(_moduleName, ModuleName);
			
			if (DumpPath == NULL)
			{
				// work out a good place for the dump file
				if (GetTempPath( _MAX_PATH, _dumpPath ) == 0)
					_tcscpy( _dumpPath, _T("c:\\temp\\") );
			}
			else
			{
				if (!validatePath(DumpPath))
				{
					// work out a good place for the dump file
					if (GetTempPath( _MAX_PATH, _dumpPath ) == 0)
						_tcscpy( _dumpPath, _T("c:\\temp\\") );

					trace(_T("Dump path %s is error, use default %s as current dump path"), DumpPath, _dumpPath);
				}
				else
				{
					_tcscpy(_dumpPath, DumpPath);

					// check if the last char if is '\\'
					{
						TCHAR* pPtr = _dumpPath;
						while (*pPtr) pPtr++;	// to the end
						pPtr--;
						while (*pPtr==_T(' '))pPtr--;	//remove the ' ' if any
						if (*pPtr!=_T('\\'))
						{
							pPtr++;
							*pPtr++ = _T('\\');
							*pPtr =  _T('\0');
						}
						else
						{
							pPtr++;
							*pPtr =  _T('\0');
						}
					}
				}
			}
			
			// now register the unhandled exception handler
			_hOldFilter = (LONG) ::SetUnhandledExceptionFilter( _OnUnhandledException );
            _set_invalid_parameter_handler(MiniDump::_OnInvalidParameter); // not care the old handler
            _set_purecall_handler(MiniDump::_OnPureCall); // not care the old handler
		}

		bool MiniDump::setDumpPath(TCHAR* DumpPath)
		{
			if (!DumpPath)
				return false;

			// verify the file
			if (!validatePath(DumpPath))
				return false;

			_tcscpy(_dumpPath, DumpPath);

			// check if the last char if is '\\'
			{
				TCHAR* pPtr = _dumpPath;
				while (*pPtr) pPtr++;	// to the end
				pPtr--;
				while (*pPtr==_T(' '))pPtr--;	//remove the ' ' if any
				if (*pPtr!=_T('\\'))
				{
					pPtr++;
					*pPtr++ = _T('\\');
					*pPtr =  _T('\0');
				}
				else
				{
					pPtr++;
					*pPtr =  _T('\0');
				}
			}

			return true;
		}

		void MiniDump::setServiceName(const TCHAR* szServiceName)
		{
			if (szServiceName)
			{
				strncpy(_serviceName, szServiceName, sizeof(szServiceName)-1);
			}
		}
	
		void MiniDump::resetUnhandledExceptionFilter()
		{
			// now register the unhandled exception handler
			_hOldFilter = (LONG) ::SetUnhandledExceptionFilter( _OnUnhandledException );
		}

        static bool generateMiniDump(MiniDump::MINIDUMPWRITEDUMP writeDump, struct _EXCEPTION_POINTERS *pEx, DWORD dwFlag, const char* folder, const char* moduleName, ZQ::common::Log& log)
        {
            // generate the file name
			TCHAR  tszFileName[512];
            SYSTEMTIME st;
            GetLocalTime(&st);
            _stprintf(tszFileName, _T("%s%s_%d-%02d-%02d_%02d.%02d.%02d%s.dmp"), folder, moduleName,
                      st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                      (dwFlag & MiniDumpWithFullMemory ? ".full" : ""));

            // record in the event log
			if (pEx && pEx->ExceptionRecord)
			{
				DWORD ExceptionCode;
				PVOID ExceptionAddress;
				DWORD dwThreadID = GetCurrentThreadId();	
				ExceptionCode = pEx->ExceptionRecord->ExceptionCode;
				ExceptionAddress = pEx->ExceptionRecord->ExceptionAddress;
                log(Log::L_ERROR, CLOGFMT("MiniDump", "Module[%s] crashed, ExceptonCode 0x%08x, ExceptionAddress 0x%08x, Current Thread ID: 0x%04x, writing crash dump to file %s"),
					moduleName, ExceptionCode, ExceptionAddress, dwThreadID, tszFileName);
			}
			else
			{
				log(Log::L_ERROR, CLOGFMT("MiniDump", "Module[%s] crashed, writing crash dump to file %s"), moduleName, tszFileName);
			}

			TCHAR Scratch[512];
            bool retval = false;
            HANDLE hFile = ::CreateFile( tszFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			
            if (hFile!=INVALID_HANDLE_VALUE)
            {
                _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                ExInfo.ThreadId = ::GetCurrentThreadId();
                ExInfo.ExceptionPointers = pEx;
                ExInfo.ClientPointers = FALSE;

                // write the dump
                BOOL bOK = writeDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, (MINIDUMP_TYPE) (dwFlag), &ExInfo, NULL, NULL );
                if (bOK)
                {
                    _stprintf(Scratch, _T("Saved dump file to '%s'"), tszFileName);
                    //                    trace(_moduleName, Scratch);
                    log(Log::L_INFO, Scratch);
                    retval = true;
                }
                else
                {
                    _stprintf( Scratch, _T("Failed to save dump file to '%s' (error %d)"), tszFileName, GetLastError() );
                    //                    trace(_moduleName, Scratch);
                    log(Log::L_ERROR, Scratch);
                }

                ::CloseHandle(hFile);
            }
            else
            {
                _stprintf( Scratch, _T("Failed to create dump file '%s' (error %d)"), tszFileName, GetLastError() );
                //                trace(_moduleName, Scratch);
                log(Log::L_ERROR, Scratch);
            }
            return retval;
        }

		LONG WINAPI MiniDump::_OnUnhandledException(struct _EXCEPTION_POINTERS *pExceptionInfo )
		{
			ZQ::common::Guard<ZQ::common::Mutex> opt(_lock);

			DumpState appDumpState(_serviceName);
			DumpState::DumpHepler dumpHelper(appDumpState);

			LONG retval = EXCEPTION_CONTINUE_SEARCH;
			SysLog	sysLog(_moduleName, Log::L_INFO);

            // call the callback if not NULL
            if (_pExceptionCB)
            {
                DWORD dwExceptionCode = 0;
                PVOID pExceptionAddr = 0;

                if (pExceptionInfo && pExceptionInfo->ExceptionRecord)
                {
                    dwExceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
                    pExceptionAddr = pExceptionInfo->ExceptionRecord->ExceptionAddress;
                }

                _pExceptionCB(dwExceptionCode, pExceptionAddr);
            }

			// firstly see if dbghelp.dll is in the same dirctory of the application
			HMODULE hDll = NULL;
			TCHAR DbgHelpPath[_MAX_PATH];
			
			TCHAR *pos = _tcsrchr(_moduleFullPath, _T('\\'));
			if (pos)
			{
				_tcscpy(DbgHelpPath, _moduleFullPath);
				_tcscpy(DbgHelpPath + (pos - _moduleFullPath +1), DBGHELP_DLL );
				trace(_T("loading %s"), DbgHelpPath);
				hDll = ::LoadLibrary( DbgHelpPath );
			}
			
			if (NULL == hDll)
			{
				// load any version in the search path
				trace(_T("loading %s"), DBGHELP_DLL);
				hDll = ::LoadLibrary(DBGHELP_DLL);
			}
			
			MINIDUMPWRITEDUMP pDump = NULL;
			if (NULL == hDll || NULL == (pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, DUMP_METHOD)))
			{
				trace(_T("failed to load expected %s"), DBGHELP_DLL);
				try
				{
					// TODO: place code here to do some last minute processing in the face
					// of an unhandled exception before Dr. Watson kills us and creates a
					// crash dump.
					//#pragma message("TODO: Implement code to flush logs etc. in the face of an unhandled exception")
					
					retval = EXCEPTION_CONTINUE_SEARCH;
				}
				catch (...)
				{
				}

				return retval;
			}

			trace(_T("hDll=%08x; pDump=%08x"), hDll, pDump);

            DWORD dwFlag = MiniDumpWithDataSegs|MiniDumpNormal;
            if(generateMiniDump(pDump, pExceptionInfo, dwFlag, _dumpPath, _moduleName, sysLog))
            {
                retval = EXCEPTION_EXECUTE_HANDLER;
            }

            if (_bEnableFullMemoryDump)
            { // generate the full memory dump
                dwFlag = MiniDumpWithDataSegs|MiniDumpNormal | MiniDumpWithFullMemory;
                if(generateMiniDump(pDump, pExceptionInfo, dwFlag, _dumpPath, _moduleName, sysLog))
                {
                    retval = EXCEPTION_EXECUTE_HANDLER;
                }
            }
			
			return retval;
		}

		void MiniDump::setExceptionCB(ExceptionCallBack cb)
		{
			_pExceptionCB = cb;
		}

//////////////////////////////////////////////////////////////////////////
// added by Cary for debugging symbol

#pragma comment(lib, "dbghelp.lib")

void MiniDump::initSym(LPSTR symbolPath, bool sysSym)
{
	char symPath[MINIDUMP_SYMBOL_PATH] = "";

	if (sysSym) {
		// environment variable _NT_SYMBOL_PATH
		if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", symPath, 
			MINIDUMP_SYMBOL_PATH))
		{
			strcat(symPath, ";");
		}
	}
	
	// Add any user defined path
	if (strlen(symbolPath))
	{
		strcat(symPath, symbolPath);
	}

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
	BOOL initRes = SymInitialize(GetCurrentProcess(), symPath, TRUE);

	if (initRes == FALSE) {
		assert(false);
	}
}

bool MiniDump::resolveSym(const void* addr, LPSTR symName, size_t symLen, 
						  LPSTR fileName, size_t fileNameLen, int* line)
{
	BOOL ret = FALSE;
	
	if (symName && symLen) {

#ifdef WIN64
		DWORD64 disp = 0, symSize = 10000;
#else
		DWORD disp = 0, symSize = 10000;
#endif // WIN64
		
		PIMAGEHLP_SYMBOL symInfo = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 
			symSize);

		ZeroMemory(symInfo, symSize);
		
		symInfo->SizeOfStruct  = (DWORD) symSize;
		symInfo->MaxNameLength = (DWORD) symSize - sizeof(IMAGEHLP_SYMBOL);

		// Set the default to unknown
		strcpy(symName, "?");

		// Get symbol info
		if (SymGetSymFromAddr(GetCurrentProcess(), (ULONG)addr, &disp, symInfo)) {
			// Make the symbol readable for humans
			UnDecorateSymbolName(symInfo->Name,
								 symName,
								 MINIDUMP_SYMBOL_SIZE,
								 UNDNAME_COMPLETE			  |
								 UNDNAME_NO_THISTYPE		  |
								 UNDNAME_NO_SPECIAL_SYMS	  |
								 UNDNAME_NO_MEMBER_TYPE		  |
								 UNDNAME_NO_MS_KEYWORDS		  |
								 UNDNAME_NO_ACCESS_SPECIFIERS |
								 UNDNAME_NO_ARGUMENTS);


			ret = TRUE;
		}

		GlobalFree(symInfo);
	}

	
	if ((fileName && fileNameLen) || line) {
		DWORD displacement;
		IMAGEHLP_LINE lineInfo;

		strcpy(fileName, "<unknown_file>");

		if (SymGetLineFromAddr(GetCurrentProcess(), (ULONG )addr, 
			&displacement, &lineInfo)) {

			if (fileName && fileNameLen)
				strncpy(fileName, lineInfo.FileName, fileNameLen);

			if (line)
				*line = lineInfo.LineNumber;
			
			ret = TRUE;
		}
	}

	return ret;
}
void MiniDump::_OnInvalidParameter(const wchar_t* expression,
                                   const wchar_t* function, 
                                   const wchar_t* file, 
                                   unsigned int line, 
                                   uintptr_t pReserved)
{
    // just raise an exception to generate a dump file
    RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
}
void MiniDump::_OnPureCall()
{
    // just raise an exception to generate a dump file
    RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
}
#else 
 
     bool MiniDump::setDumpPath(char* DumpPath)
	 {
		 if(DumpPath) {
			 chdir(DumpPath);
		 }
		 else {
			 char path1[64] = {0};
			 sprintf(path1, "/proc/%d/exe", getpid());

			 char path2[PATH_MAX] = {0};
			 readlink(path1, path2, PATH_MAX-1);

			 char* p = strstr(path2, "/bin");
			 if(p) {
				 *p = '\0';
				 strcat(path2, "/logs/crashdump");
				 chdir(path2);
			 }
		 }
		 return true;
	 }

	

	bool  MiniDump::initSignals(bool shellmode) {
		/*
		* install signals
		*/
		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;

		if(!shellmode) {
			act.sa_flags |= SA_SIGINFO;
			act.sa_sigaction = ::terminate;
		}
		else {
			act.sa_handler = SIG_IGN;
		}

		if(sigaction(SIGTERM, &act, 0) < 0 || 
			sigaction(SIGINT,  &act, 0) < 0 ||  
			sigaction(SIGHUP,  &act, 0) < 0) { 
				return false;
		}

		act.sa_sigaction=::coredump;
		act.sa_flags |= SA_SIGINFO;
		if(sigaction(SIGSEGV, &act, 0) < 0) {
			return false;
		}
		if(sigaction(SIGABRT, &act, 0) < 0) {
			return false;
		}

		return true;
	}
#endif
} // namespace common
} // namespace ZQ
