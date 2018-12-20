// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
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
// Ident : $Id: BaseSchangeServiceApplication.cpp,v 1.1 2004/08/05 07:07:39 wli Exp $
// Branch: $Name:  $
// Author: Kaliven.lee
// Desc  : Define Base Schange Service Application framework
//
// Revision History: 
// ---------------------------------------------------------------------------
// log definition
//
// ===========================================================================

#include "baseschangeserviceapplication.h"

#ifndef _NO_SYSMON
#pragma comment(lib, "SysMonLib" VODLIBEXT)
#endif

#pragma comment(lib, "Reporter" VODLIBEXT)
#pragma comment(lib, "ManPkgU" VODLIBEXT)
#pragma comment(lib, "CfgPkgU" VODLIBEXT)
#pragma comment(lib, "MTTcpComm" VODLIBEXT)
#pragma comment(lib,"Ws2_32")

#include "../vodversion.h"
// right now among all the VOD libs we used, only AppShell.dll has been changed to AppShellU.dll
// from Axiom SDK 4.0, we don't have to explicit link these dlls, it is done by header files automatically
#if VOD_VERSION_MAJOR < 4
#pragma comment(lib, "AppShell" VODLIBEXT)
#endif	// VOD_VERSION_MAJOR < 4


#include "thread.h"

#include "appshell.h"
#include "cfgpkg.h"

#ifndef _NO_SYSMON
#include "sysmonlib.h"
#endif

#include "manpkg.h"
#include "MtTcpComm.h"

#include <Tchar.h>
#include <string>



using std::wstring;
using ZQ::common::BaseSchangeServiceApplication;
using ZQ::common::Log;

DWORD BaseSchangeServiceApplication::m_dwPortNumber = SCSRV_DEFAULT_MGMT_PORT;
DWORD BaseSchangeServiceApplication::m_dwKeepAliveInterval_ms = SCSRV_DEFAULT_HEARTBEAT_ms;
DWORD BaseSchangeServiceApplication::m_dwShutdownWaitTime = SCREPORTER_DEFAULT_FLUSH_TIMEOUT;
wchar_t BaseSchangeServiceApplication::m_wsLogFileName[MaxPath] = _T("");
DWORD BaseSchangeServiceApplication::m_dwLogWriteTimeOut = SCREPORTER_DEFAULT_FLUSH_TIMEOUT;
DWORD BaseSchangeServiceApplication::m_dwLogFileSize = SCREPORTER_DEFAULT_FILESIZE;
DWORD BaseSchangeServiceApplication::m_dwLogBufferSize = SCREPORTER_DEFAULT_BUFSIZE;
DWORD BaseSchangeServiceApplication::m_dwLogLevel = Log::L_DEBUG;
TYPEINST* BaseSchangeServiceApplication::m_typeinst = NULL;
wchar_t BaseSchangeServiceApplication::m_wsTypeStr[16] = _T("");
wchar_t BaseSchangeServiceApplication:: m_logLevelStr[16] = _T("");


extern DWORD gdwServiceType;
extern DWORD gdwServiceInstance;

extern BaseSchangeServiceApplication *Application;
BaseSchangeServiceApplication::BaseSchangeServiceApplication():
m_bShutdownFlag(false),
m_bServiceStarted(false),

#ifndef _NO_SYSMON
m_pSysMon(NULL),
#endif

m_hManPkg(NULL),
m_bIsUnInit(false),
m_bReportInitialized(false)
{	

m_typeinst = new TYPEINST();

wcscpy(m_wsLogFileName,SCSRV_DEFAULT_LOGFILENAME);
}


BaseSchangeServiceApplication::~BaseSchangeServiceApplication(void)
{
	if(!m_bIsUnInit)
		unInit();
}

#ifndef _NO_SYSMON

void SysMonCallBack(COMMONINFOBLOCK *pCIB, SYSPECIFICINFOBLOCK *pSIB)
{
	// fill common block info, dynamic stuff
	pCIB->State = ACTIVE_STATE;
	pCIB->Load = 0;
	pCIB->MajorFaultCount = 0;
	pCIB->MinorFaultCount = 0;
}
#endif

HRESULT BaseSchangeServiceApplication::init(void)
{

	wchar_t errBuf[256];
	HRESULT hr = S_OK;
	m_sServiceName = servname;
	m_sProductName = prodname;

	// std::wstring wsStr = m_sServiceName + _T("APP_Stop");
	// m_StopEvent = CreateEvent(NULL,TRUE,FALSE,wsStr.c_str());	

	WCHAR eventName[MAX_PATH];
	wsprintfW(eventName, L"%sAPP_Stop", m_sServiceName.c_str());
	m_StopEvent = CreateEvent(NULL,TRUE,FALSE, eventName);	

	m_pReporter = new ScReporter((wchar_t*)m_sServiceName.c_str());		

	//logEvent(_T("service application start initializing"),Log::L_DEBUG);
	
	 //
	//get config handle 
	//

	//initialize CFGPKG
	DWORD dwRegNum = 0;
	m_hCfg = CFG_INITEx((WCHAR*)m_sServiceName.c_str(),&dwRegNum,(WCHAR*)m_sProductName.c_str());	
	
	if(NULL ==  m_hCfg )
	{// severe error 
		wsprintf(errBuf,
				_T("Config open error,No subkey %S in \\LocalMachine\\Softeware\\Seachange\\%S\\CurrentVersion\\Service"),
				m_sServiceName.data(),
				m_sProductName);

		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL){
			LPCTSTR str[] ={errBuf};
			ReportEvent(hEventSource, // handle of event source
				(WORD)EVENTLOG_ERROR_TYPE,				  // event type
				0,                    // event category
				0,			           // event ID
				NULL,                 // current user's SID
				1,               // strings in lpszStrings
				0,             // no. of bytes of raw data
				str, 		        // array of error strings
				NULL);               // raw data
			
			DeregisterEventSource(hEventSource);
		}

		return S_FALSE;
	}

	//
	// read configuration
	// if there is no setting in your register, we will use default setting
	//
	
	DWORD dwTemp = 0;
	if (CFG_SUCCESS == CFG_GET_MGMT_PORT(m_hCfg, &dwTemp))
	{
		m_dwPortNumber = dwTemp;
	}
	else
	{
		wsprintf(errBuf, _T("Fail to get management port of %s service, use default %d"), servname, m_dwPortNumber);
		
		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL){
			LPCTSTR str[] ={errBuf};
			ReportEvent(hEventSource, // handle of event source
				(WORD)EVENTLOG_ERROR_TYPE,				  // event type
				0,                    // event category
				0,			           // event ID
				NULL,                 // current user's SID
				1,               // strings in lpszStrings
				0,             // no. of bytes of raw data
				str, 		        // array of error strings
				NULL);               // raw data
			
			DeregisterEventSource(hEventSource);
		}
	}

	//	 init management subsystem

	DWORD dwError = 0;
	MANSTATUS manStatus;

	manStatus = ManOpenSession((LPCTSTR) m_sServiceName.c_str(), 
			m_dwPortNumber, &m_hManPkg, &dwError);

	if (manStatus!= MAN_SUCCESS)
	{
		// severe error		
		wsprintf(errBuf, _T("Fail to call ManOpenSession() with service name %s Port %d"), servname, m_dwPortNumber);
		
		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL){
			LPCTSTR str[] ={errBuf};
			ReportEvent(hEventSource, // handle of event source
				(WORD)EVENTLOG_ERROR_TYPE,				  // event type
				0,                    // event category
				0,			           // event ID
				NULL,                 // current user's SID
				1,               // strings in lpszStrings
				0,             // no. of bytes of raw data
				str, 		        // array of error strings
				NULL);               // raw data
			
			DeregisterEventSource(hEventSource);
		}
		
		return S_FALSE;
	}

	// initialize log
	getConfigValue(_T("LogFileSize"),&m_dwLogFileSize,m_dwLogFileSize,false,true);
	getConfigValue(_T("LogBufferSize"),&m_dwLogBufferSize,m_dwLogBufferSize,false,true);
	getConfigValue(_T("LogWriteTimeOut"),&m_dwLogWriteTimeOut,m_dwLogWriteTimeOut,false,true);
	
	getConfigValue(_T("LogLevel"),&m_dwLogLevel,m_dwLogLevel,false,false);
	mbstowcs(m_logLevelStr, Log::getVerbosityStr(m_dwLogLevel),32);
	if(m_dwLogLevel>7) 
		m_dwLogLevel = 7;

	manageVar(L"LogLevel",MAN_STR,(DWORD)(void*)m_logLevelStr,true,&dwError);


	DWORD dwSize = MaxPath*2;
	hr = getConfigValue(_T("LogFileName"),m_wsLogFileName,(wchar_t*)m_wsLogFileName,&dwSize,false,true);
	if (hr != S_OK)
	{
		wsprintf(errBuf, _T("Fail to get the registry setting of LogFileName"));
		
		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL){
			LPCTSTR str[] ={errBuf};
			ReportEvent(hEventSource, // handle of event source
				(WORD)EVENTLOG_ERROR_TYPE,				  // event type
				0,                    // event category
				0,			           // event ID
				NULL,                 // current user's SID
				1,               // strings in lpszStrings
				0,             // no. of bytes of raw data
				str, 		        // array of error strings
				NULL);               // raw data
			
			DeregisterEventSource(hEventSource);
		}

		return S_FALSE;
	}

	if(wcschr(m_wsLogFileName,L'%'))
	{
		wchar_t LogFile[MaxPath];
		::ExpandEnvironmentStrings(m_wsLogFileName,LogFile,MaxPath*2);
		memset(m_wsLogFileName,0,MaxPath*2);
		wcscpy(m_wsLogFileName,LogFile);
	}	


	m_bReportInitialized = true;
	m_pReporter->setReportLevel(ALL_LOGS,(_REPORTLEVEL)m_dwLogLevel);
	//before your initialize here you can not write log file 
	bool bRet = m_pReporter->initialize((wchar_t*)m_wsLogFileName,
							m_dwLogFileSize,
							m_dwLogBufferSize,
							m_dwLogWriteTimeOut);

	if (!bRet)
	{
		// severe error		
		wsprintf(errBuf, _T("Fail to call initialize ScReporter, filename %s, size %d"), m_wsLogFileName, m_dwLogFileSize);
		
		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL){
			LPCTSTR str[] ={errBuf};
			ReportEvent(hEventSource, // handle of event source
				(WORD)EVENTLOG_ERROR_TYPE,				  // event type
				0,                    // event category
				0,			           // event ID
				NULL,                 // current user's SID
				1,               // strings in lpszStrings
				0,             // no. of bytes of raw data
				str, 		        // array of error strings
				NULL);               // raw data
			
			DeregisterEventSource(hEventSource);
		}
		
		m_bReportInitialized = false;
		return S_FALSE;		
	}

	m_pReporter->writeMessage(L"===================== Loading service common interface ======================");

	_strVersion = __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD);
	MyGlog(Log::L_INFO, "========================= Service[%s] starts =========================",_strVersion.c_str());

	getConfigValue(_T("KeepAliveIntervals"),&m_dwKeepAliveInterval_ms,m_dwKeepAliveInterval_ms,true,true);
	getConfigValue(_T("ShutdownWaitTime"),&m_dwShutdownWaitTime,m_dwShutdownWaitTime,true,true);

	
	
//	getConfigValue(_T("Type"),&m_typeinst->s.dwType,m_typeinst->s.dwType,true,true);
	
	getConfigValue(_T("Instance"),&m_typeinst->s.dwInst,gdwServiceInstance,true,true);
	m_typeinst->s.dwType = gdwServiceType;
	wsprintf(m_wsTypeStr,L"%x",gdwServiceType);
	manageVar(_T("ServiceType"),MAN_STR,(DWORD)(void*)m_wsTypeStr ,true,&dwError);
	//
	//initialize sysmoniter
	//

#ifndef _NO_SYSMON
	m_pSysMon = new SysMon();
	m_pCIB = new COMMONINFOBLOCK();
	
	int sysmonStatus = m_pSysMon->Initialize(*m_typeinst,m_pCIB, 0, 
		NULL, SysMonCallBack, m_hCfg);

	if (sysmonStatus != SYSMON_SUCCESS && sysmonStatus != 
		SYSMON_ALREADY_INITIALIZED) {
		logEvent(_T("sysmon service initialize failed"),Log::L_ERROR);
		//return S_FALSE;
	}

	wchar_t szServer[MAX_COMPUTERNAME_LENGTH  + 1];
	dwSize = MAX_COMPUTERNAME_LENGTH  + 1;
	::GetComputerName(szServer,&dwSize);	
	char sServer [MAX_COMPUTERNAME_LENGTH+1];
	wcstombs(sServer,szServer,MAX_COMPUTERNAME_LENGTH+1);
	hostent* host = ::gethostbyname(sServer);
	// fill common block info, static stuff
	wcscpy(m_pCIB->HostName,szServer);
	wcscpy(m_pCIB->ServiceName,m_sServiceName.data());

	// Resolve the string node name in FQDN or dotted notation to 
	// address in DWORD(NT byte order) 

	DWORD dwIPAddr=INADDR_NONE;
	int dwTcpErrCode;
	m_pCIB->PrimaryIp = INADDR_NONE;
	m_pCIB->SecondaryIp = INADDR_NONE;	
	if(MtCommResolveNodeName(host->h_addr_list[0], &dwIPAddr, 
		&dwTcpErrCode) == TCPSTATE_SUCCESS)	{
		m_pCIB->PrimaryIp = dwIPAddr;
	}

	if(MtCommResolveNodeName(host->h_addr_list[1], &dwIPAddr, 
		&dwTcpErrCode) == TCPSTATE_SUCCESS)	{
		m_pCIB->SecondaryIp = dwIPAddr;
	}

	m_pCIB->MgmtPort		= (WORD)m_dwPortNumber;
	m_pCIB->PeerGrpId		= 0;
	m_pCIB->SpecificType	= NONE_SPECIFIED;
	m_pCIB->SystemInstance	= 0;
	
#endif // #ifndef _NO_SYSMON

	//  
	// Initialize the unhandled exception filter.  This allows us to do some
	// processing in the face of an exception, regardless of which thread the
	// exception was raised in.
	//
	
	//removed by Jason, the minidump will take the job, 2007-07-19
//	SetUnhandledExceptionFilter(UnhandledExceptionFilter);

	hr = OnInit();
	return hr;

}


LONG WINAPI BaseSchangeServiceApplication::UnhandledExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	static long _onlyOnce = 0;
	if (InterlockedIncrement(&_onlyOnce) > 1)
		return EXCEPTION_EXECUTE_HANDLER;
	if (InterlockedIncrement(&_onlyOnce) > 1)
		return EXCEPTION_EXECUTE_HANDLER;

	Application->OnUnHandledException(pExceptionInfo);

	//
	// Allow the Operating System to continue searching for exception handlers.
	// Most likely, none will be found and Dr. Watson will take over.
	//
	return EXCEPTION_CONTINUE_SEARCH;
}



void BaseSchangeServiceApplication::exitProcess(void)
{
	::ExitProcess(-1);
}
HRESULT BaseSchangeServiceApplication::startService(void)
{
	HRESULT hr = S_OK;

	logEvent(_T("service starting"),Log::L_DEBUG);


	ResetEvent(m_StopEvent);
	//your main process
	hr = OnStart();
	if(hr != S_OK)
	{
		// exist the service thread
		return hr;
	}
	
	for (;;)
	{
		//
		// The app must signal the APPALIVE event back to the service shell
		// every m_dwKeepAliveInterval_ms milliseconds, else the shell
		// will think we're hung and nuke our process.  Typically, a service
		// would do some type of sanity checking to insure that everything is
		// working and then signal the event.  Note that setting the alive event
		// should be implemented as a task of the main dispatching loop of the
		// app to be sure that task dispatching is working correctly.  It's a
		// bad idea to create a separate thread to set the alive event, because
		// this doesn't catch hangs in the main dispatcher, or a task dispatched
		// from it.
		//
		
		//if( isHealth())
		//{
		//	SetEvent(handle_array[APPALIVE]); // tell appshell we're alive
		//}

		if(m_bShutdownFlag)
		{
			Sleep(m_dwShutdownWaitTime);
			return S_OK;
		}

		DWORD waitResults = WaitForSingleObject
			(			  
			m_StopEvent,            // pointer to the object-handle array			                    
			m_dwKeepAliveInterval_ms     // time-out interval in milliseconds
			);

		switch (waitResults)
		{

		case WAIT_ABANDONED:			
		case WAIT_OBJECT_0:		
			{				
				Sleep(m_dwShutdownWaitTime);
				return S_OK;
				break;
			}
		case WAIT_TIMEOUT:
			{
//				Sleep(5000);
				isHealth();
			continue;
				break;
			}
		case WAIT_FAILED:
			{
				wchar_t buf[256];
#ifdef _DEBUG
				swprintf(buf,_T("WAIT_FAILED Last error[%u]"),GetLastError());
#endif
				logEvent(buf,Log::L_ERROR);
				break;
			}
		}
		
	} // loop forever	
	
	return hr;
}
HRESULT BaseSchangeServiceApplication::OnStart(void)
{

	
	logEvent(_T("Service Process is running"),Log::L_DEBUG);
	return S_OK;
}
HRESULT BaseSchangeServiceApplication::getConfigValue(wchar_t*attr_name,DWORD* value,DWORD defaultVal,bool bReport,bool bMan, bool bReadOnly)
{
	DWORD dwType = 0;
	DWORD dwSize = sizeof(DWORD);
	HRESULT hr = S_OK;
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(WCHAR*)attr_name,(BYTE*)value,&dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		hr = S_FALSE;
		*value = defaultVal;
	}
	wchar_t buf[256];
	if(bReport)
	{
		wsprintf(buf,_T("Get configuration [%s] = [%u]"),attr_name,*value);

		logEvent(buf,Log::L_DEBUG);
	}
	
	if(bMan)
	{		
		DWORD dwError = 0;
		MANSTATUS ManStatus =  ManManageVar(m_hManPkg, attr_name, MAN_INT, (DWORD)value, bReadOnly?TRUE:FALSE, &dwError);	
		if(ManStatus != MAN_SUCCESS)
		{
			memset(buf,0,512);
			wsprintf(buf,_T("Error when set manual mange variable %s"),attr_name);
			logEvent(buf,Log::L_ERROR);
		}
	}	
	return hr;
}
HRESULT BaseSchangeServiceApplication::getConfigValue(wchar_t*attr_name,wchar_t* value,wchar_t* defaultVal,DWORD *dwSize,bool bReport,bool bMan, bool bReadOnly)
{
	DWORD dwType = 0;
	HRESULT hr = S_OK;
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(WCHAR*)attr_name,(BYTE*)value,dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		hr = S_FALSE;
		wcscpy(value,defaultVal);
	}
	
	// add some additional buff, coz the real size is big the *dwSize for wsprintf()
	DWORD bufsize = (*dwSize > 0) ? (*dwSize+256) : 256;  
	
	// change to use allocated buffer to avoid the case: the configured string size is more than 256
	// and previous code wsprintf() has not restriction on size, and got problem if this situation happens.
	wchar_t *buf = new wchar_t[bufsize];

	if(bReport)
	{
		wsprintf(buf,_T("Get configuration [%s] = [%s]"),attr_name,value);		
		logEvent(buf,Log::L_DEBUG);
	}

	if(bMan)
	{		
		DWORD dwError = 0;
		MANSTATUS manStatus = ManManageVar(m_hManPkg, attr_name, MAN_STR, (DWORD)value, bReadOnly?TRUE:FALSE, &dwError);	
		if(manStatus != MAN_SUCCESS )
		{
			memset(buf,0,bufsize);
			wsprintf(buf,_T("Error when set manual manage variable %s"),attr_name);		
			logEvent(buf,Log::L_ERROR);
		}
	}
	
	delete []buf;
	
	return hr;	
}
HRESULT BaseSchangeServiceApplication::getConfigValue(wchar_t*attr_name,bool* value,bool defaultVal,bool bReport,bool bMan, bool bReadOnly)
{
	HRESULT hr = S_OK;
	DWORD dwType = 0;
	DWORD dwSize = sizeof(BOOL);
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(WCHAR*)attr_name,(BYTE*)value,&dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		*value = defaultVal;
		hr = S_FALSE;
	}
	wchar_t buf[256];
	if(bReport)
	{
		if(*value)
		{
			wsprintf(buf,_T("Get configuration [%s] = [true]"),attr_name);
		}
		else
		{
			wsprintf(buf,_T("Get configuration [%s] = [false]"),attr_name);
		}

		logEvent(buf,Log::L_DEBUG);
	}

	if(bMan)
	{		
		DWORD dwError = 0;
		MANSTATUS manstatus = ManManageVar(m_hManPkg, attr_name, MAN_BOOL, (DWORD)value, bReadOnly?TRUE:FALSE, &dwError);	
		if (manstatus != MAN_SUCCESS)
		{
			memset(buf,0,512);
			wsprintf(buf,_T("Error when set manual manage variable %s"),attr_name);		
			logEvent(buf,Log::L_ERROR);
		}
	}
	return hr;
}

HRESULT BaseSchangeServiceApplication::setConfigValue(wchar_t* cfg_name,wchar_t* value,DWORD size,DWORD dwType,bool bReport)
{
	
	CFGSTATUS status = CFG_SET_VALUE(m_hCfg,cfg_name,(BYTE*)value,size,dwType);
	if(status != CFG_SUCCESS)
	{
		return S_FALSE;
	}
	if(bReport)
	{
		wchar_t buf[256];
		wsprintf(buf,_T("Set configuration [%s] as [%s]"),cfg_name,value);
		logEvent(buf,Log::L_DEBUG);
	}
	return S_OK;
}

HRESULT BaseSchangeServiceApplication::hasSubKey(wchar_t* keyName)
{	
	CFGSTATUS status = CFG_SUBKEY_EXISTS(m_hCfg, keyName);
	return status == CFG_SUCCESS ? S_OK : S_FALSE;
}

HRESULT BaseSchangeServiceApplication::setSubKey(wchar_t* subkey)
{
	CFGSTATUS status = CFG_SUBKEY(m_hCfg, subkey, NULL);
	return status == CFG_SUCCESS ? S_OK : S_FALSE;
}

HRESULT BaseSchangeServiceApplication::OnInit(void)
{
	m_bServiceStarted = true;	

	logEvent(_T("Service is initialized"),Log::L_DEBUG);
	return S_OK;
}
HRESULT BaseSchangeServiceApplication::OnUnInit(void)
{
	m_bServiceStarted = false;	

	logEvent(_T("Service start uninitializing"),Log::L_NOTICE);
	return S_OK;
}
HRESULT BaseSchangeServiceApplication::OnPause(void)
{
    SetEvent( HAppPaused );
	logEvent(_T("Recieve Pause message from service shell."),Log::L_NOTICE);
	return S_OK;
}
HRESULT BaseSchangeServiceApplication::OnContinue(void)
{
	SetEvent( HAppContinued );	
	logEvent(_T("Recieve Continue message from service shell."),Log::L_NOTICE);
	return S_OK;
}
HRESULT BaseSchangeServiceApplication::OnStop(void)
{	
	m_bShutdownFlag = true;
	logEvent(_T("Recieve SCMgrStop message from service shell."),Log::L_NOTICE);
	SetEvent(m_StopEvent);
	return S_OK;
}
bool BaseSchangeServiceApplication::isHealth(void)
{
	//logEvent(_T("Take health check,the process is health."),Log::L_DEBUG);
	return true;
}

HRESULT BaseSchangeServiceApplication::unInit(void)
{
	m_bIsUnInit = true;
	HRESULT hr = S_OK;
	if(m_bServiceStarted)
		hr = OnUnInit();
	if(m_hCfg != NULL)
	{
		CFGSTATUS status = CFG_TERM(m_hCfg);
		if(status != S_OK)
			hr = status;
	}
	if(m_StopEvent != NULL)
	{
		if(!::CloseHandle(m_StopEvent))
			hr = S_FALSE;
	}
	if(m_hManPkg != NULL)
	{	
		DWORD errCode = 0;
		MANSTATUS manStatus =  ManCloseSession(m_hManPkg,&errCode);
		if(manStatus != MAN_SUCCESS)
			hr = S_FALSE;
	}

	if(m_typeinst)
	{
		delete m_typeinst;
		m_typeinst = NULL;
	}
	if(m_pReporter)
	{
		if(m_bReportInitialized)
			m_pReporter->unInitialize();
		delete m_pReporter;
		m_pReporter = NULL;
	}

#ifndef _NO_SYSMON
	if(m_pCIB)
	{
		delete m_pCIB;
		m_pCIB = NULL;
	}
	if(m_pSysMon)
	{
		delete m_pSysMon;
		m_pSysMon = NULL;
	}
#endif

	return hr;
}
void BaseSchangeServiceApplication::logEvent(wchar_t* msg,int loglevel)
{
	m_pReporter->writeMessage(msg,loglevel);
}
void BaseSchangeServiceApplication::stopService(void)
{
	::SetEvent(m_StopEvent);
}
void BaseSchangeServiceApplication::OnUnHandledException(_EXCEPTION_POINTERS *pExceptionInfo)
{

}