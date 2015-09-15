#include "ZQDaemon.h"
#include "ConfigHelper.h"
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "strHelper.h"
// #include "SnmpManPkg.h"
#include "./ZQResource.h" // to include the version and file information

extern ZQ::common::Config::ILoader* configLoader;
extern ZQ::common::XMLPreferenceEx* getPreferenceNode(const std::string& path, ZQ::common::XMLPreferenceDocumentEx& config);

namespace {
	const char* ConfigFile = "/etc/TianShan.xml";

	std::string eventName;

	const unsigned long ZQDEFAULT_HEARTBEAT_MS = (15 * 1000);
	const unsigned long ZQDEFAULT_SHUTDOWNWAIT_MS = 100;
}

namespace ZQ{
namespace common{

// -----------------------------
// ClassWrapper ServiceMIB
// -----------------------------
ZQ::common::FileLog ZQ::common::ServiceMIB::_flog;
int ZQ::common::ServiceMIB::cInst = 0;

// -----------------------------
// class BaseZQServiceApplication
// -----------------------------
BaseZQServiceApplication::BaseZQServiceApplication()
	: _logger(0), m_argc(0), m_argv(0),
	_logSize(ZQLOG_DEFAULT_FILESIZE), _logTimeout(ZQLOG_DEFAULT_FLUSHINTERVAL), _logBufferSize(ZQLOG_DEFAULT_BUFFSIZE), _logLevel(Log::L_DEBUG),
	_snmpLoggingMask(true), _instanceId(0), _pSnmpSA(NULL)
{
}

BaseZQServiceApplication::~BaseZQServiceApplication() {
}


void BaseZQServiceApplication::setServiceName(const std::string& name) {
	_serviceName = name;
}

void BaseZQServiceApplication::setInstanceId(uint32 id)
{
	_instanceId = id;
}
uint32 BaseZQServiceApplication::getInstanceId() const
{
	return _instanceId;
}

std::string BaseZQServiceApplication::getServiceName() const {
	return _serviceName;
}

bool BaseZQServiceApplication::init(int argc, char* argv[])
{
	m_argc = argc;
	m_argv = argv;

	// initialize system log
	openlog(_serviceName.c_str(), 0, LOG_DAEMON);

	// initialize event
	eventName = _serviceName + "APP_Stop";
	_appStopEvent = sem_open(eventName.c_str(), 0, 0644, 0);

	if (_appStopEvent == SEM_FAILED) {
		syslog(LOG_ERR, "failed to create stop event: (%s)", strerror(errno));
		return false;
	}

	// basic load configuration
	XMLPreferenceDocumentEx doc;
	if (!doc.open(ConfigFile)) {
		syslog(LOG_ERR, "failed to open configuration (%s)", ConfigFile);
		return false;
	}

	// log configuration
	XMLPreferenceEx* node = getPreferenceNode(_serviceName + "/log", doc);
	if (!node) {
		syslog(LOG_ERR, "failed to get log configuration");
		return false;
	}

	char value[255];
	memset(value, '\0', sizeof(value));

	bool res = false;

	// path
	res = node->getAttributeValue("path", value);
	if (!res) {
		syslog(LOG_ERR, "failed to get log path");
		return false;
	}
	_logDir = value;
#ifdef _DEBUG
	fprintf(stderr, "logDir: %s\n", _logDir.c_str());
#endif

	std::istringstream is;

	// SNMP logging mask
	res = node->getAttributeValue("snmpLoggingMask", value);
	if (res) {
		is.str(value);
		is >> _snmpLoggingMask;

#ifdef _DEBUG
		fprintf(stderr, "snmpLoggingMask: %d\n", _snmpLoggingMask);
#endif
	}

	// size
	res = node->getAttributeValue("size", value);
	if (res) {
		is.clear();
		is.str(value);
		is >> _logSize;
#ifdef _DEBUG
		fprintf(stderr, "logSize: %u\n", _logSize);
#endif
	}

	// count
	unsigned short logCount = ZQLOG_DEFAULT_FILENUM;
	res = node->getAttributeValue("count", value);
	if (res) {
		is.clear();
		is.str(value);
		is >> logCount;
#ifdef _DEBUG
		fprintf(stderr, "logCount: %d\n", logCount);
#endif
	}

	// level
	res = node->getAttributeValue("level", value);
	if (res) {
		is.clear();
		is.str(value);
		is >> _logLevel;
#ifdef _DEBUG
		fprintf(stderr, "logLevel: %d\n", _logLevel);
#endif
	}

	// bufferSize
	res = node->getAttributeValue("bufferSize", value);
	if (res) {
		is.clear();
		is.str(value);
		is >> _logBufferSize;
#ifdef _DEBUG
		fprintf(stderr, "logBufferSize: %u\n", _logBufferSize);
#endif
	}

	// timeout
	res = node->getAttributeValue("timeout", value);
	if (res) {
		is.clear();
		is.str(value);
		is >> _logTimeout;
#ifdef _DEBUG
		fprintf(stderr, "logTimeout: %d\n", _logTimeout);
#endif
	}
	node->free();
	node = 0;

	std::string logFilename;
	if (_logDir[_logDir.length() - 1] != '/')
		_logDir += "/";

	logFilename = _logDir + _serviceName + ".log";

	try {
		_logger = new FileLog(
			logFilename.c_str(),
			_logLevel,
			logCount,
			_logSize,
			_logBufferSize,
			_logTimeout);

		m_pReporter = _logger;
	}
	catch (const FileLogException& ex)
	{
		std::ostringstream os;
		os << "failed to initialize log ("
			<< logFilename << ") :"
			<< "(" << ex.getString() << ")";

		syslog(LOG_ERR, os.str().c_str());

		return false;
	}

	//_strVersion = std::string(ZQ_INTERNAL_FILE_NAME) + " v" __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD);
	_strVersion = __STR1__(ZQ_PRODUCT_VER_MAJOR) "." __STR1__(ZQ_PRODUCT_VER_MINOR) "." __STR1__(ZQ_PRODUCT_VER_PATCH) "." __STR1__(ZQ_PRODUCT_VER_BUILD);

	(*_logger)(Log::L_INFO, "========================= Service[%s] starts =========================",_strVersion.c_str());
	ZQ::common::setGlogger(_logger);

	//unsigned int svcInstanceId = getInstanceId();
	//syslog(LOG_DEBUG, "opening SNMP session, loggingMask[%d]", _snmpLoggingMask);
	//if (SNMPOpenSession(_serviceName, 2, 0, _snmpLoggingMask, _logDir.c_str(), svcInstanceId))
	//	(*_logger)(Log::L_INFO, "SNMP session opened, svcInstanceId[%d], snmpLoggingMask[%d]", svcInstanceId, _snmpLoggingMask);
	//else
	//	(*_logger)(Log::L_WARNING, "failed to open SNMP session, snmpLoggingMask[%d]", _snmpLoggingMask);

	//// manage variables
	//SNMPManageVariable("Version", &_strVersion, ZQSNMP_VARTYPE_STDSTRING, true);
	//SNMPManageVariable("snmpLoggingMask", &_snmpLoggingMask, ZQSNMP_VARTYPE_INT32, true);
	//SNMPManageVariable("logDir", &_logDir, ZQSNMP_VARTYPE_STDSTRING, true);
	//SNMPManageVariable("KeepAliveIntervals", &m_dwKeepAliveInterval_ms, ZQSNMP_VARTYPE_INT32, false);
	//SNMPManageVariable("ShutdownWaitTime", &m_dwShutdownWaitTime, ZQSNMP_VARTYPE_INT32, false);
	//SNMPManageVariable("logFileSize", &_logSize, ZQSNMP_VARTYPE_INT32, true);
	//SNMPManageVariable("logTimeout", &_logTimeout, ZQSNMP_VARTYPE_INT32, true);
	//SNMPManageVariable("logBufferSize", &_logBufferSize, ZQSNMP_VARTYPE_INT32, true);
	//SNMPManageVariable("logLevel", &_logLevel, ZQSNMP_VARTYPE_INT32, false);

	std::string serviceName = _serviceName;
	size_t pos = serviceName.find_first_of("0123456789");
	if (std::string::npos != pos)
	{
		serviceName = serviceName.substr(0, pos);
	}
	uint32 nServiceTypeId = ServiceMIB::oidOfServiceType((char*)serviceName.c_str());
	std::string snmpLogFileName = _logDir + _serviceName + ".snmp.log";
	ServiceMIB::_flog.open(snmpLogFileName.c_str(), _snmpLoggingMask & 0x0f, 3);
	_pServiceMib = new ServiceMIB(nServiceTypeId, getInstanceId());

	// load service configuration xml
	configLoader->setLogger(_logger);
	node = getPreferenceNode(_serviceName + "/config", doc);
	res = node->getAttributeValue("path", value);
	node->free();

	if (!res)
	{
		(*_logger)(Log::L_ERROR, "failed to read the path name of configuration");
		return false;
	}

	_configDir = value;
	if (_configDir.at(_configDir.length() - 1) != '/')
		_configDir += "/";

#ifdef _DEBUG
	fprintf(stderr, "configDir: %s\n", _configDir.c_str());
#endif

	// SNMPManageVariable("configDir", &_configDir, ZQSNMP_VARTYPE_STDSTRING, false);

	(*_logger)(Log::L_DEBUG, "loading service configuration in folder[%s]", _configDir.c_str());

	_logger->flush();
	if (!configLoader->loadInFolder(_configDir.c_str(), true))
	{
		(*_logger)(Log::L_ERROR, "failed to load config in folder[%s]", _configDir.c_str());
		_logger->flush();
		return false;
	}

	(*_logger)(Log::L_INFO, "loaded configuration from folder[%s]", _configDir.c_str());

	res = OnInit();
	if (res)
		(*_logger)(Log::L_INFO, "(%s/%u) initialized", _serviceName.c_str(), _instanceId);

	ZQ::SNMP::ModuleMIB::_flags_VERBOSE = (_snmpLoggingMask >>8) & 0xff;
	doEnumSnmpExports();
	
	_logger->flush();

	return res;
}

bool BaseZQServiceApplication::start() {
	(*_logger)(Log::L_INFO, "starting service[%s]", _serviceName.c_str());

	bool res = OnStart();
	if (!res) {
		_logger->flush();
		return res;
	}
	_logger->flush();

	int ret = 0;

wait:
	ret = sem_wait(_appStopEvent);
	if (ret == (-1) && errno == EINTR) {
		goto wait;
	}

	sem_close(_appStopEvent);

	return ret == 0;
}

void BaseZQServiceApplication::stop() {
	OnStop();
}

void BaseZQServiceApplication::unInit() {

	OnUnInit();

	ZQ::common::setGlogger(NULL); // reset the gLogger to NullLogger

	// close system log
	closelog();

	// destroy semaphore
	sem_unlink(eventName.c_str());

	// destroy file log
	if (_logger)
		delete _logger;

	_logger = NULL;
	m_pReporter = NULL;

	// SNMPCloseSession();
}

bool BaseZQServiceApplication::isHealth() {
	return true;
}

bool BaseZQServiceApplication::OnInit() {
	(*_logger)(Log::L_INFO, "service[%s] initialized", _serviceName.c_str());

	return true;
}

bool BaseZQServiceApplication::OnStart() {
	(*_logger)(Log::L_INFO, "service[%s] started, initializing SNMP subagent", _serviceName.c_str());

	if (NULL != _pServiceMib)
	{
		_pSnmpSA = new ZQ::SNMP::Subagent(_pServiceMib->_flog, *_pServiceMib, DEFAULT_COMM_TIMEOUT);
		if (_pSnmpSA)
			_pSnmpSA->start();
	}


	return true;
}

bool BaseZQServiceApplication::OnStop() {
	(*_logger)(Log::L_INFO, "Recieve Stop message from service shell. ");
	if (_pSnmpSA)
		delete _pSnmpSA;
	_pSnmpSA = NULL;

	return true;
}

bool BaseZQServiceApplication::OnUnInit() {
	if (_logger) {
		(*_logger)(Log::L_INFO, "service[%s] unitialized", _serviceName.c_str());
		_logger->flush();
	}
	return true;
}

void BaseZQServiceApplication::doEnumSnmpExports()
{
	SvcMIB_ExportReadOnlyVar("Version", &_strVersion, AsnType_String, ".1");
	SvcMIB_ExportReadOnlyVar("SnmpLoggingMask", &_snmpLoggingMask, AsnType_Int32, ".2");
	SvcMIB_ExportReadOnlyVar("LogDir", &_logDir, AsnType_String, ".3");
	SvcMIB_ExportReadOnlyVar("KeepAliveIntervals", &m_dwKeepAliveInterval_ms, AsnType_Int32, ".4");
	SvcMIB_ExportReadOnlyVar("ShutdownWaitTime", &m_dwShutdownWaitTime, AsnType_Int32, ".5");
	SvcMIB_ExportReadOnlyVar("LogFileSize", &_logSize, AsnType_Int32, ".6");
	SvcMIB_ExportReadOnlyVar("LogTimeOut", &_logTimeout, AsnType_Int32, ".7");
	SvcMIB_ExportReadOnlyVar("LogBufferSize", &_logBufferSize, AsnType_Int32, ".8");

	SvcMIB_ExportByAPI("LogLevel", uint32, AsnType_Int32, &BaseZQServiceApplication::getLogLevel_Main, &BaseZQServiceApplication::setLogLevel_Main, ".9");

	SvcMIB_ExportReadOnlyVar("configDir", &_configDir, AsnType_String, ".10");
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

} } //namespace ZQ::common

// vim: ts=4 sw=4 bg=dark nu
