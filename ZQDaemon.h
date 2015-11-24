#ifndef __ZQ_DAEMON_H__
#define __ZQ_DAEMON_H__

#include <string>

#include <semaphore.h>

#include "FileLog.h"
#include "XMLPreferenceEx.h"
#include "snmp/ZQSnmp.h"

#define HRESULT bool
#define S_FALSE false
#define S_OK true

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif // PATH_MAX

//__BEGIN_ZQ_COMMON
namespace ZQ {
namespace common {

// -----------------------------
// ClassWrapper ServiceMIB
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

#define ServiceMIB_ExportVarEx(SVCMIB, VARNAME, VARADDR, VARTYPE, SUBOID, READONLY)	\
	SVCMIB->addObject(new ZQ::SNMP::SNMPObject(VARNAME, VARADDR, ZQ::SNMP::VARTYPE, READONLY), SUBOID)

#define ServiceMIB_ExportByAPI(SVCMIB, VARNAME, CLASS, OBJ, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)	\
	SVCMIB->addObject(new ZQ::SNMP::SNMPObjectByAPI<CLASS, BASET>(VARNAME, OBJ, ZQ::SNMP::ASNTYPE, METHOD_GET, METHOD_SET), SUBOID)

// -----------------------------
// class BaseZQServiceApplication
// -----------------------------
class BaseZQServiceApplication
{

public:
	BaseZQServiceApplication();
	virtual ~BaseZQServiceApplication();

public:
	bool init(int, char**);
	bool start();
	void stop();
	void unInit();

public:
	virtual bool isHealth();

	virtual bool OnInit();
	virtual bool OnStart();
	virtual bool OnStop();
	virtual bool OnUnInit();

	// As a steps after OnInit(), the BaseServiceApp give the implemetation a 
	// chance to enumerate its variable that wish to export thru SNMP
	virtual void doEnumSnmpExports(void);

public:
	void setServiceName(const std::string&);
	std::string getServiceName() const;

	void setInstanceId(uint32 id);
	uint32 getInstanceId() const;

private:
	std::string _serviceName;

	sem_t* _appStopEvent;

protected:
	FileLog* _logger;
	FileLog* m_pReporter;

	int m_argc;
	char** m_argv;

	ServiceMIB::Ptr _pServiceMib;

	// variables to be managed by snmp
	std::string _strVersion;
	std::string _logDir;
	uint32 _logSize;
	uint16 _logTimeout;
	uint32 _logBufferSize;
	int _logLevel;
	int _snmpLoggingMask;
	uint32	_instanceId;
	ZQ::SNMP::Subagent* _pSnmpSA;
	uint32 m_dwKeepAliveInterval_ms;// wait time out option in start()
	uint32 m_dwShutdownWaitTime; //shutdownWaitTime
	std::string _configDir;

public: // APIs to SNMP access
	uint32 getLogLevel_Main();
	void   setLogLevel_Main(const uint32& newLevel);
};

#define SvcMIB_ExportReadOnlyVar(VARNAME, VARADDR, VARTYPE, SUBOID)\
	ServiceMIB_ExportVarEx(_pServiceMib, VARNAME, VARADDR, VARTYPE, SUBOID, true)

#define SvcMIB_ExportByAPI(VARNAME, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)\
	ServiceMIB_ExportByAPI(_pServiceMib, VARNAME, BaseZQServiceApplication, *this, BASET, ASNTYPE, METHOD_GET, METHOD_SET, SUBOID)

}} //namespace ZQ::common

#endif //__ZQ_DAEMON_H__

// vim: ts=4 sw=4 bg=dark nu
