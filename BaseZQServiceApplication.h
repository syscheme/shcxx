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

// log definition
//
// ===========================================================================
#ifndef __ZQ_COMMON_BASEZQSERVICEAPPLICATION_H_
#define __ZQ_COMMON_BASEZQSERVICEAPPLICATION_H_

#include <ZQ_common_conf.h>
#include "Log.h"

#include <zqappshell.h>
#include <zqcfgpkg.h>
#include "FileLog.h"
#include "snmp/ZQSnmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <locale>
#include <tchar.h>
#include <time.h>

#define ZQDEFAULT_HEARTBEAT_MS			(15*1000) // this is the old value
#define ZQDEFAULT_SHUTDOWNWAIT_MS		100

#ifndef MaxPath
#define MaxPath 260
#endif

using std::wstring;
using std::ofstream;
using std::endl;

#ifdef _DEBUG
	#ifdef _UNICODE
		#  define VERLIBEXT "U_d.lib"
	#else
		#  define VERLIBEXT "_d.lib"
    #endif
#else
	#ifdef _UNICODE
		#  define VERLIBEXT "U.lib"
	#else
		#  define VERLIBEXT ".lib"
	#endif
#endif


//-----------------------------------------------------------------------------
//  TYPEINST
//
//  TYPEINST Is a unique identifier for the caller (CM or AS).  The identifier
//  is composed of an application type and and application instance number.
//
typedef union _TYPEINST
{
    uint64      qwTypeInst;
    struct
    {
        uint32  dwInst;
        uint32  dwType;
    }s;
} TYPEINST, *PTYPEINST, *LPTYPEINST;

extern TCHAR   servname[MAX_APPSHELL_NAME_LENGTH]; 
extern TCHAR   prodname[MAX_APPSHELL_NAME_LENGTH]; 
extern "C" void app_main(int argc,char *argv[]);
extern "C" void app_service_control(int SCode);

namespace ZQ{
namespace common{

class ZQ_COMMON_API BaseZQServiceApplication;

// -----------------------------
// class ServiceMIB
// -----------------------------
class ServiceMIB : public ZQ::SNMP::ModuleMIB, public SharedObject
{
public:
	typedef Pointer< ServiceMIB > Ptr;

	ServiceMIB(uint32 serviceTypeId, uint32 serviceInstanceId)
		: ModuleMIB(_flog, serviceTypeId, ZQ::SNMP::ModuleMIB::ModuleOid_Application, serviceInstanceId)
	{
		cInst++;
	}

	virtual ~ServiceMIB()
	{
		// if (--cInst <=0)
		//	_flog.clear();
	}

	static FileLog _flog;

private:
	static int cInst;
};

#define ServiceMIB_ExportVarEx(SVCMIB, VARNAME, VARADDR, SUBOID, READONLY)	\
	SVCMIB->addObject(new ZQ::SNMP::SNMPObject(VARNAME, VARADDR, READONLY), SUBOID)

#define ServiceMIB_ExportByAPI(SVCMIB, VARNAME, CLASS, OBJ, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)	\
	SVCMIB->addObject(new ZQ::SNMP::SNMPObjectByAPI<CLASS, BASET>(VARNAME, OBJ, ZQ::SNMP::##ASNTYPE, METHOD_GET, METHOD_SET), SUBOID)

//extern ZQ::common::Log* pProglog = &ZQ::common::NullLogger;

// -----------------------------
// class BaseZQServiceApplication
// -----------------------------
class BaseZQServiceApplication
{	
	friend void ::app_main (int argc, char *argv[]);
	friend void ::app_service_control(int SCode);

public:
	BaseZQServiceApplication();	
	virtual ~BaseZQServiceApplication(void);

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

	// As a steps after OnInit(), the BaseServiceApp give the implemetation a 
	// chance to enumerate its variable that wish to export thru SNMP
	virtual void doEnumSnmpExports(void);

	//////////////////////////////////////////////////////////////////////////
	///
	/// OnStop
	/// This function is fired when appshell recieve SCMgrSTOP from zqsrvShell.
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
	/// This function is fired when appshell recieve SCMgrPAUSE from zqsrvShell.
	/// And SCMgrPAUSE is sent by SCManager when user stop the service
	/// So you can implement your code in your subclass.And you must call the 
	/// the function in your subclass before your code.
	/// 
	//////////////////////////////////////////////////////////////////////////	
	virtual HRESULT OnPause(void);
	//////////////////////////////////////////////////////////////////////////
	///
	/// OnContinue
	/// This function is fired when appshell recieve SCMgrCONTINUE from zqsrvShell.
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
	/// This function is fired before your service sent heartbeat to zqsrvshell 
	/// and appShell. if the function return true,the service will send the 
	/// heartbeat.Otherwise the service will not send the heartbeat. zqsrvShell 
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

    //////////////////////////////////////////////////////////////////////////
    ///
    /// OnSnmpSet
    /// This function is fired after a set-request through snmp. So you can 
    /// implement you additional operation in your derived class to reflect
    /// the set-request.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnSnmpSet(const char *varName);

public:
	bool isServiceStarted(void){return m_bServiceStarted;}
	bool getShutdownFlag(void){return m_bShutdownFlag;}

	void logEvent(TCHAR * msg, int loglevel);
	void logEvent(int logLevel, TCHAR* fmt, ...) PRINTFLIKE(3, 4);

	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\ZQ\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the DWORD value 
	/// defaultval	[in]the default DWORD value
    /// bReport		[in]flag whether report the config 
    /// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	HRESULT getConfigValue(TCHAR *cfg_name,DWORD* value,DWORD defaultVal,bool bReport,bool bMan = false, bool bReadOnly = false);

	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\ZQ\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the string value 
	/// defaultval	[in]the default string value
	/// dwSize		[in]
	/// bReport		[in]flag whether report the config 
	/// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	HRESULT getConfigValue(TCHAR *cfg_name,TCHAR * value,TCHAR * defaultVal,DWORD* dwSize,bool bReport,bool bMan = false, bool bReadOnly = false);

	/// getConfigValue 
	/// retrieve configuration values from register 
	/// \\LOCALMACHINE\\SOFTWARE\\ZQ\\<ProductName>\\CurrentVersion\\Services\\
	/// cfg_name	[in]Key name of register 
	/// value		[out]buffer to storage the bool value 
	/// defaultval	[in] default bool value
	/// bReport		[in]flag whether report the config 
	/// bMan		[in]flag whether configuration can be display in ManUtil
	/// return value if failed read value from the register return S_FALSE and set default
	/// value as value,otherwise return S_OK
	HRESULT getConfigValue(TCHAR *cfg_name,bool* value,bool defaultVal,bool bReport,bool bMan = false, bool bReadOnly = false);

	HRESULT setConfigValue(TCHAR *cfg_name,TCHAR *value,DWORD size,DWORD type ,bool bReport);


	/// hasSubKey 
	/// Check whether there is specified sub key under 
	/// \\LOCALMACHINE\\SOFTWARE\\ZQ\\<ProductName>\\CurrentVersion\\Services\\
	/// keyName   	[in]subkey name
	/// return      S_FALSE - No such sub key
	///             S_OK    - Has such sub key
	HRESULT hasSubKey(TCHAR * keyName, bool createIfNotExist = false);

	void stopService(void);

	void setInstanceId( uint32 id );

	uint32 getInstanceId() const;

protected:	
	ZQ::common::FileLog * m_pReporter;
	bool	m_bReportInitialized;

	ServiceMIB::Ptr _pServiceMib;
	ZQ::SNMP::SubAgent* _pSnmpSA;

    // argument
    int m_argc;
    char **m_argv;

public:
	HRESULT init(int argc,char *argv[]);
	HRESULT unInit(void);	
	HRESULT startService(void);
	
	void exitProcess(void);

private:

	static LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo);

	//added by xiaohui.chai
    static void SnmpCallback(const char *varName);

//------------------------member variables defines-----------------------------------
protected:
	TCHAR   m_sServiceName[MAX_APPSHELL_NAME_LENGTH];// service name input when class is constructing.
	TCHAR   m_sProductName[MAX_APPSHELL_NAME_LENGTH];

	
	bool m_bShutdownFlag;// the flag to shut down service
	bool m_bServiceStarted;// the flag to service start

	HANDLE m_hCfg;
	
	bool m_bIsUnInit;

protected:
	HANDLE m_StopEvent;// stop event waited in start()

protected:	// Service configuration
	// Note:
	// if you want to reset the default values you must set the configuration values in your 
	// construction function. And if serive app shell can not read the configuations in register
		// service class will use the default values.
	/*static*/ DWORD m_dwKeepAliveInterval_ms;// wait time out option in start()
	/*static*/ DWORD m_dwShutdownWaitTime; //shutdownWaitTime
		
	// when transfer to use reporter change the variables
	/*static*/ TCHAR m_wsLogFolder[MaxPath];
	/*static*/ DWORD m_dwLogFileSize;// max log file size
	/*static*/ DWORD m_dwLogBufferSize;// buffer size
	/*static*/ DWORD m_dwLogLevel;// log level
	/*static*/ DWORD m_dwLogWriteTimeOut;// log write time out
	/*static*/ DWORD m_dwLogFileCount;// max log file count

	//added by xiaohui.chai
	DWORD m_dwSnmpLoggingMask;//for snmp

	//add variable to record configuration folder
	/*static*/ TCHAR m_wsConfigFolder[MaxPath];	

	//Note: you can assign this value as default value in the construct function of your sub class 
	/*static*/ TYPEINST* m_typeinst;//our typeinst:unique identifier to your service

	/*static*/ TCHAR m_wsTypeStr[32];
	/*static*/ TCHAR m_logLevelStr[32];
	TCHAR m_sVersion[128];

	std::string _strVersion;
	uint32	m_instanceId;

public:
	uint32 getLogLevel_Main();
	void   setLogLevel_Main(const uint32& newLevel);
};

#define SvcMIB_ExportReadOnlyVar(VARNAME, VARADDR, SUBOID) \
	ServiceMIB_ExportVarEx(_pServiceMib, VARNAME, VARADDR, SUBOID, true)

#define SvcMIB_ExportByAPI(VARNAME, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)\
	ServiceMIB_ExportByAPI(_pServiceMib, VARNAME, BaseZQServiceApplication, *this, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)

}} // namespaces

#endif//