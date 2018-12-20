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
// Ident : $Id: BaseSchangeServiceApplication.h,v 1.1 2004/08/05 07:07:39 wli Exp $
// Branch: $Name:  $
// Author: Kaliven.lee
// Desc  : Define Base Schange Service Application framework
//
// Revision History: 
// ---------------------------------------------------------------------------
// log definition
//
// ===========================================================================
#ifndef __ZQ_COMMON_BASESCHANGESERVICEAPPLICATION_H_
#define __ZQ_COMMON_BASESCHANGESERVICEAPPLICATION_H_


#include "screporter.h"
#include "appshell.h"

#ifndef _NO_SYSMON
#include "sysmonlib.h"
#endif

#include "ManPkg.h"
#include <stdio.h>
//#include <stdlib.h>
#include <string>
#include <locale>

#define SCSRV_DEFAULT_LOGFILENAME	_T("c:\\ServiceApp.log")
#define SCSRV_DEFAULT_HEARTBEAT_ms	(1*	1000)
#define SCSRV_DEFAULT_MGMT_PORT		1792



#ifndef MaxPath
#define MaxPath 260
#endif

using std::wstring;


extern WCHAR   servname[MAX_APPSHELL_NAME_LENGTH]; // 10/10/95 CJH - extern this so everyone can use it
extern WCHAR   prodname[MAX_APPSHELL_NAME_LENGTH]; // product name to which this service belongs (set on input in argv)
extern "C" void app_main(int argc,char *argv[]);
extern "C" void app_service_control(int SCode);

namespace ZQ{
	namespace common{

//extern ZQ::common::Log* pProglog = &ZQ::common::NullLogger;
class ZQ_COMMON_API BaseSchangeServiceApplication;
class BaseSchangeServiceApplication
{
	
	friend void ::app_main (int argc, char *argv[]);
	friend void ::app_service_control(int SCode);
public:
	BaseSchangeServiceApplication();	
	virtual ~BaseSchangeServiceApplication(void);
//virtual function implement by inheritance
protected:
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnInit
	/// This function is fired just between your service initialized and start.
	/// this function set the m_bServiceStarted flag to true
	/// So you can implement your initalize code in your subclass to open your
	/// resource for your service. and you must call the function of base class
	/// before you code at first
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnInit(void); 
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnStop
	/// This function is fired when appshell recieve SCMgrSTOP from srvShell.
	/// And SCMgrSTOP is sent by SCManager when user stop the service
	/// So you can implement your code in your subclass.And you must call the 
	/// the function in your subclass before your code.The function will single 
	/// stop event to waitforMultiObject in function start() so that the process
	/// can go on.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnStop(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnPause
	/// This function is fired when appshell recieve SCMgrPAUSE from srvShell.
	/// And SCMgrPAUSE is sent by SCManager when user stop the service
	/// So you can implement your code in your subclass.And you must call the 
	/// the function in your subclass before your code.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnPause(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnContinue
	/// This function is fired when appshell recieve SCMgrCONTINUE from srvShell.
	/// And SCMgrCONTINUE is sent by SCManager when user stop the service
	/// So you can implement your code in your subclass.And you must call the 
	/// the function in your subclass before your code.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnContinue(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnUnHandledException
	/// This function is fired when appshell recieve unhandled exception.	
	/// So you can implement your code in your subclass.this function in base
	/// class only write a log messge.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual void OnUnHandledException(_EXCEPTION_POINTERS *pExceptionInfo);
	//////////////////////////////////////////////////////////////////////////
	///
	/// isHealth
	/// This function is fired before your service sent heartbeat to srvshell 
	/// and appShell. if the function return true,the service will send the 
	/// heartbeat.Otherwise the service will not send the heartbeat. SrvShell 
	/// will restart the service.
	/// So you can implement your code in your subclass.this function in base
	/// class only write a log message.	 
	/// Note:this function is only to use light load code. 
	//////////////////////////////////////////////////////////////////////////	
	virtual bool isHealth(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnStart
	/// This function is fired after app_main start your service.So your can 
	/// implement your main process of your service in your subclass.And this 
	/// function in base class only write a log messge.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnStart(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnUnInit
	/// This function is fired after your service uninitialized.So your can 
	/// implement your code in your subclass to release the resource create 
	/// by your service.And this function in base class  write a log messge 
	/// and set the m_bShutdownFlag to false so your function must call the 
	/// function of base class..
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnUnInit(void);


public:
	bool isServiceStarted(void){return m_bServiceStarted;}
	bool getShutdownFlag(void){return m_bShutdownFlag;}

	void logEvent(wchar_t* msg,int loglevel);
	
	//////////////////////////////////////////////////////////////////////////
	/// 
	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\SEACHANGE\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the DWORD value 
	/// defaultval	[in]the default DWORD value
    /// bReport		[in]flag whether report the config 
    /// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	/// 
	//////////////////////////////////////////////////////////////////////////
	
	HRESULT getConfigValue(wchar_t*cfg_name,DWORD* value,DWORD defaultVal,bool bReport,bool bMan = false, bool bReadOnly = true);
	//////////////////////////////////////////////////////////////////////////
	/// 
	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\SEACHANGE\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the string value 
	/// defaultval	[in]the default string value
	/// dwSize		[in]
	/// bReport		[in]flag whether report the config 
	/// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	//////////////////////////////////////////////////////////////////////////

	HRESULT getConfigValue(wchar_t*cfg_name,wchar_t* value,wchar_t* defaultVal,DWORD* dwSize,bool bReport,bool bMan = false, bool bReadOnly = true);
	//////////////////////////////////////////////////////////////////////////
	/// 
	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\SEACHANGE\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the bool value 
	/// defaultval	[in] default bool value
	/// bReport		[in]flag whether report the config 
	/// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	/// 	
	//////////////////////////////////////////////////////////////////////////
	HRESULT getConfigValue(wchar_t*cfg_name,bool* value,bool defaultVal,bool bReport,bool bMan = false, bool bReadOnly = true);


	HRESULT setConfigValue(wchar_t*cfg_name,wchar_t*value,DWORD size,DWORD type ,bool bReport);


	//////////////////////////////////////////////////////////////////////////
	/// 
	/// hasSubKey 
	/// Check whether there is specified sub key under 
	/// \\LOCALMACHINE\\SOFTWARE\\SEACHANGE\\<ProductName>\\CurrentVersion\\Services\\
	/// keyName   	[in]subkey name
	/// return      S_FALSE - No such sub key
	///             S_OK    - Has such sub key
	/// 	
	//////////////////////////////////////////////////////////////////////////
	HRESULT hasSubKey(wchar_t* keyName);

	//////////////////////////////////////////////////////////////////////////
	/// 
	/// setSubKey 
	/// setSubKey makes the specified subkey the current key from which to retrieve values
	/// subkey (IN) -- the subkey of the current key, can be multiple levels,
	///              E.g. "subkey1\\subkey2\\bottomKey"
	///              Set this to NULL to reset the current key to the root of the
	///              application
 	/// return		S_FALSE - failed	
	///				S_OK   - success
	//////////////////////////////////////////////////////////////////////////
	HRESULT setSubKey(wchar_t* subkey);

	//////////////////////////////////////////////////////////////////////////
	///
	/// setLogFileName
	/// this function is to set LogFileName. And you must call the function 
	/// before initialize() was called.Perfered in your construct function 
	/// 
	//////////////////////////////////////////////////////////////////////////	
	HRESULT setLogFileName(wchar_t* wsLogFileName){wcsncpy(m_wsLogFileName, wsLogFileName,MaxPath*2);}

	MANSTATUS manageVar(wchar_t* attr_name,WORD type,DWORD dwAddress,bool readOnly,DWORD* pdwError)
	{
		return ManManageVar(m_hManPkg,attr_name,type,dwAddress,readOnly,pdwError);
	}
	void stopService(void);
protected:	
//	ScLog* m_pReporter;
	ScReporter * m_pReporter;
	bool	m_bReportInitialized;	
private:
	HRESULT init(void);
	HRESULT unInit(void);	
	HRESULT startService(void);
	
	void exitProcess(void);
	
private:
	static LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo);


//------------------------member variables defines-----------------------------------
protected:
	std::wstring m_sServiceName;// service name input when class is constructing.
	std::wstring m_sProductName;// product name input when initializing by get cfg from register.
	
	
	bool m_bShutdownFlag;// the flag to shut down service
	bool m_bServiceStarted;// the flag to service start

	HANDLE m_hCfg;

#ifndef _NO_SYSMON
	SysMon *m_pSysMon;// pointer to sysmonlib
	COMMONINFOBLOCK *m_pCIB;
#endif 

	bool m_bIsUnInit;
	HANDLE m_hManPkg;

protected:
	HANDLE m_StopEvent;// stop event waited in start()
protected:	
	// Service configuration

	// Note:
	// if you want to reset the default values you must set the configuration values in your 
	// construction function. And if serive app shell can not read the configuations in register
	// service class will use the default values.
	static DWORD m_dwPortNumber;// Management port number defaultly 0
	static DWORD m_dwKeepAliveInterval_ms;// wait time out option in start()
	static DWORD m_dwShutdownWaitTime; //shutdownWaitTime
		
	// when transfer to use reporter change the variables
	static wchar_t m_wsLogFileName[MaxPath]; // log file name
	static DWORD m_dwLogFileSize;// max log file size
	static DWORD m_dwLogBufferSize;// buffer size
	static DWORD m_dwLogLevel;// log level
	static DWORD m_dwLogWriteTimeOut;// log write time out
	
	//Note: you can assign this value as default value in the construct function of your sub class 
	static TYPEINST* m_typeinst;//our typeinst:unique identifier to your service

	static wchar_t m_wsTypeStr[16];
	static wchar_t m_logLevelStr[16];

};
	}//common
}//ZQ
#endif//__ZQ_COMMON_BASESCHANGESERVICEAPPLICATION_H_