// ===========================================================================
// Copyright (c) 2006 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// ---------------------------------------------------------------------------
// log definition
//
// ===========================================================================
#include <ZQ_common_conf.h>
#include "BaseZQServiceApplication.h"
					  
#pragma comment(lib,"Ws2_32")
// #pragma comment(lib, "ZQCfgPkg" VERLIBEXT)
// #pragma comment(lib, "ZQSnmpManPkg" VERLIBEXT)
// #pragma comment(lib, "ZQAppshell" VERLIBEXT)

#include <Tchar.h>
#include <string>
#include <ConfigHelper.h>
//#include <ZQSNMPManPkg.h>

#include "./ZQResource.h"

using std::string;
using std::wstring;
using ZQ::common::Log;
using ZQ::common::BaseZQServiceApplication;
#define MyGlog   (*m_pReporter)

//DWORD BaseZQServiceApplication::m_dwKeepAliveInterval_ms = SCSRV_DEFAULT_HEARTBEAT_ms;
//DWORD BaseZQServiceApplication::m_dwShutdownWaitTime = DEFAULT_FLUSH_TIMEOUT;
//TCHAR BaseZQServiceApplication::m_wsLogFileName[MaxPath] = _T("");
//DWORD BaseZQServiceApplication::m_dwLogWriteTimeOut = DEFAULT_FLUSH_TIMEOUT;
//DWORD BaseZQServiceApplication::m_dwLogFileSize = ZQLOG_DEFAULT_BUFFSIZE;
//DWORD BaseZQServiceApplication::m_dwLogBufferSize = ZQLOG_DEFAULT_FILESIZE;
//DWORD BaseZQServiceApplication::m_dwLogLevel = Log::L_DEBUG;
//TYPEINST* BaseZQServiceApplication::m_typeinst = NULL;
//TCHAR BaseZQServiceApplication::m_wsTypeStr[32] = _T("");
//TCHAR BaseZQServiceApplication:: m_logLevelStr[32] = _T("");
//TCHAR BaseZQServiceApplication::m_wsConfigFolder[MaxPath]=_T("");
//TCHAR BaseZQServiceApplication::m_wsLogFolder[MaxPath]=_T("");

ZQ::common::FileLog ZQ::common::ServiceMIB::_flog;
int ZQ::common::ServiceMIB::cInst =0;

extern DWORD gdwServiceType;
extern DWORD gdwServiceInstance;

extern BaseZQServiceApplication		*Application;
extern ZQ::common::Config::ILoader  *configLoader;

BaseZQServiceApplication::BaseZQServiceApplication():
	m_bShutdownFlag(false),
	m_bServiceStarted(false),
	m_bIsUnInit(false),
	m_bReportInitialized(false),
	m_instanceId(0), _pSnmpSA(NULL)
{	
	m_typeinst = new TYPEINST();
	m_pReporter = NULL;
	m_dwKeepAliveInterval_ms = ZQDEFAULT_HEARTBEAT_MS;
	m_dwShutdownWaitTime = ZQDEFAULT_SHUTDOWNWAIT_MS;
	m_dwLogWriteTimeOut = ZQLOG_DEFAULT_FLUSHINTERVAL;
	m_dwLogFileSize = ZQLOG_DEFAULT_FILESIZE;
	m_dwLogFileCount = ZQLOG_DEFAULT_FILENUM;
	m_dwLogBufferSize = ZQLOG_DEFAULT_BUFFSIZE;
    m_dwSnmpLoggingMask = 0;//added by xiaohui.chai
	m_dwLogLevel = Log::L_DEBUG;	
	_tcscpy(m_wsTypeStr, _T(""));
	_tcscpy(m_logLevelStr,_T(""));
	_tcscpy(m_wsConfigFolder,_T(""));
	_tcscpy(m_wsLogFolder,_T(""));

    _tcscpy(m_sVersion, _T(""));

    m_argc = 0;
    m_argv = NULL;
}

BaseZQServiceApplication::~BaseZQServiceApplication(void)
{
	if(!m_bIsUnInit)
		unInit();
}

HRESULT BaseZQServiceApplication::init(int argc,char *argv[])
{	
	TCHAR errBuf[512];

	HRESULT hr = S_OK;
	_tcscpy(m_sServiceName,servname);
	_tcscpy(m_sProductName,prodname);

    // argument
    m_argc = argc;
    m_argv = argv;

	TCHAR eventName[MAX_PATH];
	_stprintf(eventName, _T("%sAPP_Stop"), m_sServiceName);
	m_StopEvent = CreateEvent(NULL,TRUE,FALSE, eventName);	

	// get init 
	//initialize CFGPKG
	DWORD dwRegNum = 0;
	m_hCfg = CFG_INITEx((TCHAR*)m_sServiceName,&dwRegNum,(TCHAR*)m_sProductName);	
	
	if(NULL ==  m_hCfg )
	{// severe error 
		_stprintf(errBuf,
				_T("Config open error,No subkey %S in \\LocalMachine\\Softeware\\ZQ Interactive\\%S\\CurrentVersion\\Service"),
				m_sServiceName,
				m_sProductName);

		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL)
		{
			LPCTSTR str[] ={errBuf};
			ReportEvent(	hEventSource, // handle of event source
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
	getConfigValue(_T("LogFileCount"),&m_dwLogFileCount,m_dwLogFileCount,false,true);  
	getConfigValue(_T("LogFileSize"),&m_dwLogFileSize,m_dwLogFileSize,false,true);  
	getConfigValue(_T("LogBufferSize"),&m_dwLogBufferSize,m_dwLogBufferSize,false,true);
	getConfigValue(_T("LogWriteTimeOut"),&m_dwLogWriteTimeOut,m_dwLogWriteTimeOut,false,true);
	getConfigValue(_T("SnmpLoggingMask"),&m_dwSnmpLoggingMask,m_dwSnmpLoggingMask,false,true);
	
	getConfigValue(_T("LogLevel"),&m_dwLogLevel,m_dwLogLevel,false,true);

#if defined _UNICODE  || defined UNICODE
	mbstowcs(m_logLevelStr, Log::getVerbosityStr(m_dwLogLevel),32);
#else
	strncpy(m_logLevelStr,Log::getVerbosityStr(m_dwLogLevel),32);
#endif
	if(m_dwLogLevel>7) 
		m_dwLogLevel = 7;

	DWORD dwSize = MaxPath;
	hr = getConfigValue(_T("LogDir"),m_wsLogFolder,(TCHAR*)m_wsLogFolder,&dwSize,false,true);
	if (hr != S_OK)
	{
		_stprintf(errBuf, _T("Fail to get the registry setting of LogFileName"));
		
		HANDLE  hEventSource;
		hEventSource = RegisterEventSource(NULL, servname);
		if(hEventSource != NULL)
		{
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

	if(_tcschr(m_wsLogFolder,_T('%')))
	{
		TCHAR LogFile[MaxPath];
		::ExpandEnvironmentStrings(m_wsLogFolder,LogFile,MaxPath);
		memset(m_wsLogFolder,0,MaxPath);
		_tcscpy(m_wsLogFolder,LogFile);
	}	
	
	char strFile[MaxPath] ={0};
#if defined _UNICODE  || defined UNICODE
	WideCharToMultiByte(CP_ACP,NULL,m_wsLogFolder,-1,strFile,sizeof(strFile),NULL,NULL);
#else
	sprintf(strFile,"%s",m_wsLogFolder);
#endif

	// append the log name
	{
		char* pPtr = strFile + strlen(strFile);
		while(pPtr>strFile && *(pPtr - 1)==' ') pPtr--;

#if defined _UNICODE  || defined UNICODE
		if (pPtr == strFile || *(pPtr - 1)=='\\') 
			sprintf(pPtr, "%S.log", m_sServiceName);
		else
			sprintf(pPtr, "\\%S.log", m_sServiceName);
#else
		if (pPtr == strFile || *(pPtr - 1)=='\\') 
			sprintf(pPtr, "%s.log", m_sServiceName);
		else
			sprintf(pPtr, "\\%s.log", m_sServiceName);
#endif
	}

	m_bReportInitialized = true;
	try
	{
		m_pReporter = new ZQ::common::FileLog(strFile,Log::L_DEBUG, 
			m_dwLogFileCount,
			m_dwLogFileSize,
			m_dwLogBufferSize,
			m_dwLogWriteTimeOut,
			ZQLOG_DEFAULT_EVENTLOGLEVEL,
			m_sServiceName);
	}
	catch(ZQ::common::FileLogException& ex)
	{	
		// severe error	
#if defined _UNICODE  || defined UNICODE
		_stprintf(errBuf, _T("Fail to initialize FileLog, filename %S, size %d, error[%S]"), strFile, m_dwLogFileSize, ex.getString());
#else
		_stprintf(errBuf, _T("Fail to initialize FileLog, filename %s, size %d, error[%s]"), strFile, m_dwLogFileSize, ex.getString());
#endif
		
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

	MyGlog.setVerbosity(m_dwLogLevel);
	MyGlog(ZQ::common::Log::L_INFO,_T("===================== Loading service common interface ======================"));
	ZQ::common::setGlogger(m_pReporter);

	// remark the log
	getConfigValue(_T("KeepAliveIntervals"),&m_dwKeepAliveInterval_ms,m_dwKeepAliveInterval_ms,true,true);
	getConfigValue(_T("ShutdownWaitTime"),&m_dwShutdownWaitTime,m_dwShutdownWaitTime,true,true);

	//what is the main log?????
	MyGlog.flush();

	//DWORD dwRet=0;
	HANDLE hTempCfgHanlde=m_hCfg;
	//ZQSNMPSTATUS nStatus = SNMPOpenSession(m_sServiceName,m_sProductName,&dwRet,TRUE);
    //modified by xiaohui.chai
    const char *snmpLogFile = NULL;
    std::string snmpLogFileBuf;
    if(m_dwSnmpLoggingMask)
    {
        snmpLogFileBuf = m_wsLogFolder;
        if(snmpLogFileBuf[snmpLogFileBuf.size() - 1] != '\\')
            snmpLogFileBuf += "\\";
        snmpLogFileBuf += m_sServiceName;
        snmpLogFileBuf += ".snmp.log";

        snmpLogFile = snmpLogFileBuf.c_str();
    }

	DWORD nServiceTypeId = ServiceMIB::oidOfServiceType(m_sServiceName);
	DWORD nProcessInstanceId = Application->getInstanceId();

    // if(SNMPOpenSession(m_sServiceName, m_sProductName, ZQSNMP_OID_SVCPROCESS_SVCAPP, SnmpCallback, snmpLogFile, m_dwSnmpLoggingMask, nLogFileSize, nProcessInstanceId))
    //  MyGlog(ZQ::common::Log::L_INFO,_T("Open SNMP session successfully"));
    // else
    //    MyGlog(ZQ::common::Log::L_WARNING,"Fail to open SNMP session.");

	ServiceMIB::_flog.open(snmpLogFile, m_dwSnmpLoggingMask &0x0f, 3);
	_pServiceMib = new ServiceMIB(nServiceTypeId, nProcessInstanceId);

	MyGlog.flush();
    /////////////////////////////////////////////////////////////////////////
    // modified by xiaohui.chai
    // new SNMP interface
    //
	//SNMPManagerAddVar(_T("LogDir"), (DWORD)m_wsLogFolder, ZQSNMP_STR, 1, &dwRet);
	//SNMPManagerAddVar(_T("KeepAliveIntervals"), (DWORD)&m_dwKeepAliveInterval_ms, ZQSNMP_INT, 0, &dwRet);
	//SNMPManagerAddVar(_T("ShutdownWaitTime"), (DWORD)&m_dwShutdownWaitTime, ZQSNMP_INT, 0, &dwRet);
	//SNMPManagerAddVar(_T("LogFileSize"), (DWORD)&m_dwLogFileSize, ZQSNMP_INT, 1, &dwRet);
	//SNMPManagerAddVar(_T("LogWriteTimeOut"), (DWORD)&m_dwLogWriteTimeOut, ZQSNMP_INT, 1, &dwRet);
	//SNMPManagerAddVar(_T("LogBufferSize"), (DWORD)&m_dwLogBufferSize, ZQSNMP_INT, 1, &dwRet);
	//SNMPManagerAddVar(_T("LogLevel"), (DWORD)&m_dwLogLevel, ZQSNMP_INT, 0, &dwRet);
    //
    // added the version info
    //_tcscpy(m_sVersion, ZQ_INTERNAL_FILE_NAME ":" __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD));
    _tcscpy(m_sVersion, __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD));
	
	// SNMPManageVariable("Version", &m_sVersion, ZQSNMP_VARTYPE_STRING, TRUE);
    // SNMPManageVariable("SnmpLoggingMask", &m_dwSnmpLoggingMask, ZQSNMP_VARTYPE_INT32, TRUE);
    // SNMPManageVariable("LogDir", m_wsLogFolder, ZQSNMP_VARTYPE_STRING, TRUE);
	// SNMPManageVariable("KeepAliveIntervals", &m_dwKeepAliveInterval_ms, ZQSNMP_VARTYPE_INT32, FALSE);
    // SNMPManageVariable("ShutdownWaitTime", &m_dwShutdownWaitTime, ZQSNMP_VARTYPE_INT32, FALSE);
    // SNMPManageVariable("LogFileSize", &m_dwLogFileSize, ZQSNMP_VARTYPE_INT32, TRUE);
    // SNMPManageVariable("LogWriteTimeOut", &m_dwLogWriteTimeOut, ZQSNMP_VARTYPE_INT32, TRUE);
    // SNMPManageVariable("LogBufferSize", &m_dwLogBufferSize, ZQSNMP_VARTYPE_INT32, TRUE);
	// SNMPManageVariable("LogLevel", &m_dwLogLevel, ZQSNMP_VARTYPE_INT32, FALSE);
    
	MyGlog(ZQ::common::Log::L_INFO,_T("Begin to load configuration xml"));
	MyGlog.flush();

	m_hCfg=hTempCfgHanlde;
	//get configuration folder from registry
	{
		configLoader->setLogger(m_pReporter);
		DWORD dwSzSize=sizeof(m_wsConfigFolder)/sizeof(m_wsConfigFolder[0]);
		
		dwSzSize=sizeof(m_wsConfigFolder)/sizeof(m_wsConfigFolder[0]);
		getConfigValue(_T("configDir"),m_wsConfigFolder,m_wsConfigFolder,&dwSzSize,true,false,true);
		if(_tcsclen(m_wsConfigFolder)>0)
		{
			//make sure that the value is ok
		}
		else
		{//no configDir was found ,try HKEY_LM\Softwares\ZQ Interactive\TianShan\CurrentVersion\Services\configDir
			//how to get the registry???
#if defined _UNICODE ||defined UNICODE
			std::wstring strKey;
#else//UNICODE
			std::string	strKey;
#endif//UNICODE
			
			//strKey=_T("Softwares\ZQ Interactive\TianShan\CurrentVersion\Services\configDir");			
			strKey=_T("SOFTWARE\\ZQ Interactive\\");
			strKey+=prodname;
			strKey+=_T("\\CurrentVersion\\Services");			
			HKEY	hResultKey=NULL;
			if(ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
											strKey.c_str(),
											0,
											KEY_QUERY_VALUE,
											&hResultKey) || hResultKey == NULL)
			{
				logEvent(Log::L_ERROR,_T("Can't open registry '%s'"),strKey.c_str());
				return S_FALSE;
			}

			//get the registry of configDir
			BYTE byConfigDir[1024];
			DWORD dwcfgDirSize=sizeof(byConfigDir);
			DWORD dwRegType=0;
			if(ERROR_SUCCESS!=RegQueryValueEx(hResultKey,_T("configDir"),NULL,&dwRegType,byConfigDir,&dwcfgDirSize) )
			{
				RegCloseKey(hResultKey);
				logEvent(Log::L_ERROR,_T("Can't get configDir's value in key %s"),strKey.c_str());
				return S_FALSE;
			}

#if defined _UNICODE  || defined UNICODE
			wchar_t *pBuf=NULL;
			int iLens=strlen( (char*)byConfigDir);
			pBuf=new wchar_t[iLens+1];
			ZeroMemory(pBuf,(iLens+1)*sizeof(wchar_t));
			MultiByteToWideChar(CP_ACP,
								0,
								(char*)byConfigDir,
								iLens,
								pBuf,
								iLens);
			wcsncpy(m_wsConfigFolder,pBuf,iLens);
			delete[] pBuf;
#else
			strncpy(m_wsConfigFolder,(char*)byConfigDir,dwcfgDirSize);

#endif
			RegCloseKey(hResultKey);
		}
		
		if(!configLoader)
		{
			logEvent(ZQ::common::Log::L_ERROR,_T("No ConfigLoader instance,service down"));
			return S_FALSE;
		}

		if(m_wsConfigFolder[_tcsclen(m_wsConfigFolder)-1]!=_T('\\'))
			_tcscat(m_wsConfigFolder,_T("\\"));

		// SNMPManageVariable("configDir", m_wsConfigFolder, ZQSNMP_VARTYPE_STRING, TRUE);

		MyGlog(ZQ::common::Log::L_INFO, _T("===================== Loading service configurations ======================"));
		MyGlog.flush();
		//Now I get the configDir,but it maybe null
        
        bool loadConfigOK = configLoader->loadInFolder(m_wsConfigFolder, true);

		if(!loadConfigOK)
		{
			//log the error message in eventViewer
			logEvent(ZQ::common::Log::L_ERROR,_T("Can't load config in folder [%s]."),
				m_wsConfigFolder);
            return S_FALSE;
        }
// 			//Get current module's startup folder
// 			TCHAR szLocalFolder[1024];
// 			ZeroMemory(szLocalFolder,sizeof(szLocalFolder));
// 			GetModuleFileName(NULL,szLocalFolder,sizeof(szLocalFolder)/sizeof(szLocalFolder[0]));
// 			int iSzLen=_tcsclen(szLocalFolder);
// 			if (  iSzLen > 0 )
// 			{
// 				int iTemp=iSzLen-1;
// 				while (szLocalFolder[iTemp]!='\\' && iTemp >=0 )
// 					iTemp--;
// 				iTemp--;//skip '\'
// 				while (szLocalFolder[iTemp]!='\\' && iTemp >=0 )
// 					iTemp--;
// 				if(iTemp>0)
// 				{
// 					szLocalFolder[iTemp]='\0';
// 				}
// 				else
// 				{
// 					logEvent(ZQ::common::Log::L_ERROR,_T("Get current module's startup folder failed.service down"));
// 					return S_FALSE;
// 				}
// 				_tcsncat(szLocalFolder,_T("\\etc\\"),_tcsclen(_T("\\etc\\")));
// 				if(!configLoader->loadWithConfigFolder(szLocalFolder))
// 				{
// 					logEvent(ZQ::common::Log::L_ERROR,_T("Can't load configuration from folder %s and error is %s"),
// 						szLocalFolder,
// 						configLoader->getLastErrorDesc());
// 					return S_FALSE;
// 				}
// 				else
// 				{
// 					logEvent(ZQ::common::FileLog::L_INFO,_T("load configuration successfully"));
// 				}
// 			}			
//		}
		MyGlog(ZQ::common::Log::L_INFO,_T("load configuration from %s successfully"),configLoader->getConfigFilePath().c_str());
	}

	
//	getConfigValue(_T("Instance"),&m_typeinst->s.dwInst,gdwServiceInstance,true,true); // remark by me 20061204

	m_typeinst->s.dwType = gdwServiceType;
	_stprintf(m_wsTypeStr,_T("%d"),gdwServiceType);

	//
	//initialize sysmoniter
	//
	//  
	// Initialize the unhandled exception filter.  This allows us to do some
	// processing in the face of an exception, regardless of which thread the
	// exception was raised in.
	//

    //removed by xiaohui.chai
    //using minidump instead
	//SetUnhandledExceptionFilter(UnhandledExceptionFilter);
	hr = OnInit();
	if(hr==S_OK)
	{
		const char* pStr= ZQ_FILE_DESCRIPTION " " __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD);		
		logEvent(ZQ::common::Log::L_INFO,_T("%s started"),pStr);
	}
    else
    {
        int errRet = hr;
        MyGlog(ZQ::common::Log::L_INFO,_T("OnInit() failed with [%d]"), errRet);
    }

	ZQ::SNMP::ModuleMIB::_flags_VERBOSE = (m_dwSnmpLoggingMask >>8) & 0xff;
	doEnumSnmpExports();

	return hr;
}

void BaseZQServiceApplication::doEnumSnmpExports()
{
	// SNMPManageVariable("Version", &m_sVersion, ZQSNMP_VARTYPE_STRING, TRUE);
	SvcMIB_ExportReadOnlyVar("Version", m_sVersion, AsnType_CStr, ".1");
	SvcMIB_ExportReadOnlyVar("SnmpLoggingMask", &m_dwSnmpLoggingMask, AsnType_Int32, ".2");
	SvcMIB_ExportReadOnlyVar("LogDir", m_wsLogFolder, AsnType_CStr, ".3");
	SvcMIB_ExportReadOnlyVar("KeepAliveIntervals", &m_dwKeepAliveInterval_ms, AsnType_Int32, ".4");
	SvcMIB_ExportReadOnlyVar("ShutdownWaitTime", &m_dwShutdownWaitTime, AsnType_Int32, ".5");
	SvcMIB_ExportReadOnlyVar("LogFileSize", &m_dwLogFileSize, AsnType_Int32, ".6");
	SvcMIB_ExportReadOnlyVar("LogTimeOut", &m_dwLogWriteTimeOut, AsnType_Int32, ".7");
	SvcMIB_ExportReadOnlyVar("LogBufferSize", &m_dwLogBufferSize, AsnType_Int32, ".8");

	SvcMIB_ExportByAPI("LogLevel", uint32, AsnType_Int32, &BaseZQServiceApplication::getLogLevel_Main, &BaseZQServiceApplication::setLogLevel_Main, ".9");

	SvcMIB_ExportReadOnlyVar("configDir", m_wsConfigFolder, AsnType_CStr, ".10");
}

LONG WINAPI BaseZQServiceApplication::UnhandledExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	static long _onlyOnce = 0;
	if (InterlockedIncrement(&_onlyOnce) > 1)
		return EXCEPTION_EXECUTE_HANDLER;
	if (InterlockedIncrement(&_onlyOnce) > 1)
		return EXCEPTION_EXECUTE_HANDLER;

	Application->OnUnHandledException(pExceptionInfo);

	// Allow the Operating System to continue searching for exception handlers.
	// Most likely, none will be found and Dr. Watson will take over.
	return EXCEPTION_CONTINUE_SEARCH;
}

void BaseZQServiceApplication::exitProcess(void)
{
	::ExitProcess(-1);
}

HRESULT BaseZQServiceApplication::startService(void)
{
	HRESULT hr = S_OK;
	
	logEvent(_T("service starting"),Log::L_INFO);

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
	
	//	if( isHealth())
	//	{
	//		SetEvent(handle_array[APPALIVE]); // tell appshell we're alive
	//	}
	
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
			Sleep(m_dwShutdownWaitTime);//why sleep???
			return S_OK;
		case WAIT_TIMEOUT:
			//				Sleep(5000);
			isHealth();
			continue;

		case WAIT_FAILED:
			{
				//#ifdef _DEBUG
				DWORD err = GetLastError();
				TCHAR  buf[256];
				_stprintf(buf,_T("WAIT_FAILED for StopEvent. Last error = [%u]"),err);
				//#endif
				logEvent(buf,Log::L_ERROR);
			}
			break;
		}

	} // loop forever	
	
	return hr;
}

HRESULT BaseZQServiceApplication::OnStart(void)
{
	logEvent(_T("Service Process is running"),Log::L_INFO);
	if (NULL != _pServiceMib)
	{
		_pSnmpSA = new ZQ::SNMP::Subagent(_pServiceMib->_flog, *_pServiceMib, DEFAULT_COMM_TIMEOUT);
		_pSnmpSA->start();
	}

	return S_OK;
}
                               
HRESULT BaseZQServiceApplication::getConfigValue(TCHAR *attr_name,DWORD* value,DWORD defaultVal,bool bReport,bool bMan, bool bReadOnly)
{
	DWORD dwType = 0;
	DWORD dwSize = sizeof(DWORD);
	HRESULT hr = S_OK;
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(TCHAR*)attr_name,(BYTE*)value,&dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		hr = S_FALSE;
		*value = defaultVal;
	}

	TCHAR buf[256];
	if(bReport)
	{
		_stprintf(buf,_T("Get configuration [%s] = [%u]"),attr_name,*value);
		logEvent(buf,Log::L_INFO);
	}

	return hr;
}

HRESULT BaseZQServiceApplication::getConfigValue(TCHAR*attr_name,TCHAR* value,TCHAR* defaultVal,DWORD *dwSize,bool bReport,bool bMan, bool bReadOnly)
{
	DWORD dwType = 0;
	HRESULT hr = S_OK;
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(TCHAR*)attr_name,(BYTE*)value,dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		hr = S_FALSE;
		_tcscpy(value,defaultVal);
	}

	TCHAR buf[256];
	if(bReport)
	{
		_stprintf(buf,_T("Get configuration [%s] = [%s]"),attr_name,value);		
		logEvent(buf,Log::L_INFO);
	}
	
	return hr;	
}

HRESULT BaseZQServiceApplication::getConfigValue(TCHAR*attr_name,bool* value,bool defaultVal,bool bReport,bool bMan, bool bReadOnly)
{
	HRESULT hr = S_OK;
	DWORD dwType = 0;
	DWORD dwSize = sizeof(BOOL);
	CFGSTATUS  status = CFG_GET_VALUE(m_hCfg,(TCHAR*)attr_name,(BYTE*)value,&dwSize,&dwType);	
	if(status!= CFG_SUCCESS)
	{
		*value = defaultVal;
		hr = S_FALSE;
	}

	TCHAR buf[256];
	if(bReport)
	{
		if(*value)
		{
			_stprintf(buf,_T("Get configuration [%s] = [true]"),attr_name);
		}
		else
		{
			_stprintf(buf,_T("Get configuration [%s] = [false]"),attr_name);
		}
		logEvent(buf,Log::L_INFO);
	}

	return hr;
}

HRESULT BaseZQServiceApplication::setConfigValue(TCHAR* cfg_name,TCHAR* value,DWORD size,DWORD dwType,bool bReport)
{	
	CFGSTATUS status = CFG_SET_VALUE(m_hCfg,cfg_name,(BYTE*)value,size,dwType);
	if(status != CFG_SUCCESS)
	{
		return S_FALSE;
	}
	if(bReport)
	{
		TCHAR buf[256];
		_stprintf(buf,_T("Set configuration [%s] as [%s]"),cfg_name,value);
		logEvent(buf,Log::L_INFO);
	}
	return S_OK;
}

HRESULT BaseZQServiceApplication::hasSubKey(TCHAR* keyName, bool createIfNotExist)
{	
	CFGSTATUS status = CFG_SUBKEY_EXISTS(m_hCfg, keyName);
	if(status != CFG_SUCCESS)
	{
		if(createIfNotExist)
		{
			DWORD values = 0;
			status = CFG_SUBKEY(m_hCfg, keyName, &values);
		}
	}
	return status == CFG_SUCCESS ? S_OK : S_FALSE;
}

HRESULT BaseZQServiceApplication::OnInit(void)
{
	m_bServiceStarted = true;	
	logEvent(Log::L_INFO, _T("Service is initialized, threadid(0x%04x), this(0x%08x), instanceId[%u]"),
		GetCurrentThreadId(), this, m_instanceId);
	return S_OK;
}

HRESULT BaseZQServiceApplication::OnUnInit(void)
{
	m_bServiceStarted = false;	
	logEvent(Log::L_INFO, _T("Service is uninitialized, threadid(0x%04x), this(0x%08x)"), GetCurrentThreadId(), this);
	return S_OK;
}

HRESULT BaseZQServiceApplication::OnPause(void)
{
    SetEvent( HAppPaused );
	logEvent(_T("Recieve Pause message from service shell."),Log::L_NOTICE);
	return S_OK;
}

HRESULT BaseZQServiceApplication::OnContinue(void)
{
	SetEvent( HAppContinued );	
	logEvent(_T("Recieve Continue message from service shell."),Log::L_NOTICE);
	return S_OK;
}

HRESULT BaseZQServiceApplication::OnStop(void)
{	
	m_bShutdownFlag = true;
	if (_pSnmpSA)
		delete _pSnmpSA;
	_pSnmpSA = NULL;

	logEvent(_T("Recieve Stop message from service shell."),Log::L_NOTICE);

	SetEvent(m_StopEvent);


	return S_OK;
}

bool BaseZQServiceApplication::isHealth(void)
{
	//logEvent(_T("Take health check,the process is health."),Log::L_DEBUG);
	return true;
}

HRESULT BaseZQServiceApplication::unInit(void)
{
    //added by xiaohui.chai
    // SNMPCloseSession();

	m_bShutdownFlag =false;
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

	ZQ::common::setGlogger(NULL); // reset the gLogger to NullLogger
	if(m_pReporter)
	{
		if ( m_bReportInitialized )
		{
			try
			{
				delete m_pReporter;
				m_pReporter = NULL;
			}
			catch(...)
			{
			}
		}
	}

	if(m_typeinst)
	{
		try
		{
			delete m_typeinst;
			m_typeinst = NULL;
		}
		catch(...)
		{
		}
		
	}

	return hr;
}

void BaseZQServiceApplication::logEvent(TCHAR * msg,int loglevel)
{	
	if (!m_bIsUnInit && NULL != m_pReporter)
		(*m_pReporter)(loglevel,msg);
//	(*m_pReporter)(loglevel,msg);
}

void BaseZQServiceApplication::logEvent(int logLevel,TCHAR* fmt,...)
{
	TCHAR szMsgBuf[1024];
	ZeroMemory(szMsgBuf,sizeof(szMsgBuf));
	
	va_list args;

	va_start(args, fmt);
	_vstprintf(szMsgBuf, fmt, args);
	va_end(args);
	
	MyGlog(logLevel,szMsgBuf);

	WORD wType;
	DWORD dwEventID;
	switch(logLevel)
	{
	case ZQ::common::Log::L_INFO :
	case ZQ::common::Log::L_NOTICE :
		wType=EVENTLOG_INFORMATION_TYPE;	
		dwEventID=0x4006CC52L;
		break;

	//L_EMERG=0, L_ALERT,  L_CRIT, L_ERROR
	case ZQ::common::Log::L_WARNING :
		wType=EVENTLOG_WARNING_TYPE;
		dwEventID=0x8006CC51L;
		break;

	case ZQ::common::Log::L_EMERG :
	case ZQ::common::Log::L_ALERT :
	case ZQ::common::Log::L_CRIT :
	case ZQ::common::Log::L_ERROR :
		wType=EVENTLOG_ERROR_TYPE;
		dwEventID=0xC006CC50L;
		break;

	default:
		return;
	}
		
	HANDLE  hEventSource;
	hEventSource = RegisterEventSource(NULL, servname);
	if(hEventSource != NULL)
	{
		LPCTSTR str[] ={szMsgBuf};
		ReportEvent(hEventSource, // handle of event source
					wType,				  // event type
					0,                    // event category
					dwEventID,			           // event ID
					NULL,                 // current user's SID
					1,               // strings in lpszStrings
					0,             // no. of bytes of raw data
					str, 		        // array of error strings
					NULL);               // raw data
		
		DeregisterEventSource(hEventSource);
	}
	
}

void BaseZQServiceApplication::stopService(void)
{
	::SetEvent(m_StopEvent);
}

void BaseZQServiceApplication::OnUnHandledException(_EXCEPTION_POINTERS *pExceptionInfo)
{
}	

void BaseZQServiceApplication::OnSnmpSet(const char *varName)
{
}

void BaseZQServiceApplication::SnmpCallback(const char *varName)
{
    if(NULL == varName)
        return;

    if(0 == strcmp(varName, "LogLevel"))
    {
        Application->m_pReporter->setVerbosity(Application->m_dwLogLevel);
    }
    else
    {
        Application->OnSnmpSet(varName);
    }
}

void BaseZQServiceApplication::setInstanceId( uint32 id )
{
	m_instanceId = id;
}

uint32 BaseZQServiceApplication::getInstanceId() const
{
	return m_instanceId;
}

uint32 BaseZQServiceApplication::getLogLevel_Main()
{
	if (NULL == m_pReporter)
		return 0;

	return m_pReporter->getVerbosity();
}

void   BaseZQServiceApplication::setLogLevel_Main(const uint32& newLevel)
{
	if (m_pReporter)
		m_pReporter->setLevel(newLevel);
}
