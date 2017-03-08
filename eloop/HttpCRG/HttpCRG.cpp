//include std header


#include <boost/thread.hpp>
#include <sstream>

//include tianshan header
#include "TianShanDefines.h"
#ifdef ZQ_OS_MSWIN
#include "MiniDump.h"
#else // linux
#include <sys/resource.h> // for the fnlimit
#endif

//include 
#include "HttpCRGConfig.h"
#include "HttpCRG.h"
// #include "snmp/ZQSnmpMgmt.hpp"
// #include "HttpCRGSnmpExt.h"



::ZQTianShan::HttpCRG::HttpCRGService g_HttpCRGService;
ZQ::common::BaseZQServiceApplication  *Application = &g_HttpCRGService;

ZQ::common::Config::Loader<ZQTianShan::HttpCRG::HttpCRGConfig> gConfig("HttpCRG.xml");
ZQ::common::Config::ILoader *configLoader = &gConfig;

extern const char* DUMP_PATH;

#ifdef ZQ_OS_MSWIN
DWORD gdwServiceType = 1 ;
DWORD gdwServiceInstance = 1;

// crash dump
ZQ::common::MiniDump _crashDump;
static void WINAPI CrashExceptionCallBack(DWORD ExceptionCode, PVOID ExceptionAddress)
{
	DWORD dwThreadID = GetCurrentThreadId();	
	glog( ZQ::common::Log::L_ERROR,  "Crash exception callback called,ExceptonCode 0x%08x, ExceptionAddress 0x%08x, Current Thread ID: 0x%04x",ExceptionCode, ExceptionAddress, dwThreadID);	
	glog.flush();
}

static bool validatePath( const char *     szPath )
{
	if (-1 != ::GetFileAttributesA(szPath))
		return true;

	DWORD dwErr = ::GetLastError();
	if ( dwErr == ERROR_PATH_NOT_FOUND || dwErr == ERROR_FILE_NOT_FOUND )
	{
		if (!::CreateDirectoryA(szPath, NULL))
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
#endif

namespace ZQTianShan{
	namespace HttpCRG{


		/// constructor
		HttpCRGService::HttpCRGService()
			:_crg(NULL) //,_httpCRGSnmp(NULL)
		{
		}
		/// destructor
		HttpCRGService::~HttpCRGService()
		{
		}

		static void fixupDir(std::string &path)
		{
			if(path.empty())
			{
				return;
			}
			else
			{
				if(path[path.size() - 1] != FNSEPC)
					path.push_back(FNSEPC);
			}
		}
		static void fixupConfig(HttpCRGConfig &config)
		{
			// get the program root
			std::string tsRoot = ZQTianShan::getProgramRoot();
#ifdef ZQ_OS_MSWIN
			// fixup the dump path
			if(gConfig._crashDump.enabled && gConfig._crashDump.path.empty())
			{
				// use the Root/logs as default dump folder
				gConfig._crashDump.path = tsRoot + FNSEPS + "logs" + FNSEPS;
			}
			else
			{
				fixupDir(gConfig._crashDump.path);
			}
#else
			DUMP_PATH = gConfig._crashDump.path.c_str();
#endif

			// fixup the plug-in's config path
			if(gConfig._pluginsConfig.configDir.empty())
			{
				// use the Root/etc as default config folder
				gConfig._pluginsConfig.configDir = tsRoot + FNSEPS + "etc" + FNSEPS;
			}
			else
			{
				fixupDir(gConfig._pluginsConfig.configDir);
			}

			// fixup the plug-in's log path
			if(gConfig._pluginsConfig.logDir.empty())
			{
				// get the program root
				std::string tsRoot = ZQTianShan::getProgramRoot();
				// use the Root/logs as default log folder
				gConfig._pluginsConfig.logDir = tsRoot + FNSEPS + "logs" + FNSEPS;
			}
			else
			{
				fixupDir(gConfig._pluginsConfig.logDir);
			}

			// expand the plug-in's populate path
			if(!gConfig._pluginsConfig.populatePath.empty())
			{
				gConfig._pluginsConfig.populate(gConfig._pluginsConfig.populatePath);
			}

			// fixup EventChannel endpoint
			//if(gConfig._eventChannel.endPoint.empty())
			//{
			//	gConfig._eventChannel.endPoint = DEFAULT_ENDPOINT_TopicManager;
			//}
		}


		HRESULT HttpCRGService::OnInit(void)
		{
			// step 1: fixup the config
			fixupConfig(gConfig);

			//create logger
			//	_fileLog.open(gConfig._logFile.path.c_str(), gConfig._logFile.level, gConfig._logFile.maxCount, gConfig._logFile.size, gConfig._logFile.bufferSize, gConfig._logFile.flushTimeout);

#ifdef ZQ_OS_MSWIN
			// step 2: crash dump
			if(gConfig._crashDump.enabled)
			{
				if(!validatePath(gConfig._crashDump.path.c_str()))
				{
					glog(::ZQ::common::Log::L_ERROR, CLOGFMT(HttpCRGService, "OnInit() bad dump path [%s]"), gConfig._crashDump.path.c_str());
					return S_FALSE;
				}
				// enable crash dump

				_crashDump.setDumpPath((char*)gConfig._crashDump.path.c_str());
				_crashDump.enableFullMemoryDump(true);
				_crashDump.setExceptionCB(CrashExceptionCallBack);
			}
#endif

			//	gConfig.snmpRegister("HTTPCRG");

			ZQ::eloop::HttpServer::HttpServerConfig conf;
			conf.host = gConfig._bind.ip;
			conf.port = gConfig._bind.port;
			conf.threadCount = 4;

			//step 3. init HttpCRG
			_crg = new CRG::CRGateway(*m_pReporter,conf);


			_crg->setModEnv(gConfig._pluginsConfig.configDir, gConfig._pluginsConfig.logDir);

			//_crg->setCapacity(gConfig._threadPoolConfig.size, gConfig._threadPoolConfig.maxPendingRequest);

			//_crg->enableMessageDump(!gConfig.hexDumpMode, true, true);

			return BaseZQServiceApplication::OnInit();
		}

		void HttpCRGService::doEnumSnmpExports()
		{
			BaseZQServiceApplication::doEnumSnmpExports();

			//{".2.1.100", "httpReqTbl" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100)
			//{".2.1.100.1", "httpReqEntry" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1)
			//{".2.1.100.1.1", "reqMethod" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqMethod(1)
			//{".2.1.100.1.2", "reqCount" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqCount(2)
			//{".2.1.100.1.3", "reqLatencyAvg" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqLatencyAvg(3)
			//{".2.1.100.1.4", "reqLatencyMax" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).reqLatencyMax(4)
			//{".2.1.100.1.5", "resp2xx" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp2xx(5)
			//{".2.1.100.1.6", "resp400" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp400(6)
			//{".2.1.100.1.7", "resp404" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp404(7)
			//{".2.1.100.1.8", "resp500" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp500(8)
			//{".2.1.100.1.9", "resp503" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpReqEntry(1).resp503(9)
			//{".2.1.100.2", "httpStat-Since" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpStat-Since(2)
			//{".2.1.100.3", "httpStat-Measure-Reset" }, //{enterprises}.zqInteractive(22839).tianShanComponents(4).tianShanService(1).httpCRG(2200).httpSvcApp(2).httpCrgAttr(1).httpReqTbl(100).httpStat-Measure-Reset(3)

			static ZQ::SNMP::Oid subOidTbl;
			if (subOidTbl.isNil())
				_pServiceMib->reserveTable("httpReqTbl", 9, subOidTbl);
			if (subOidTbl.isNil())
			{
				(*m_pReporter)(ZQ::common::Log::L_WARNING, CLOGFMT(HttpCRGService,"snmp_refreshStatTbl() failed to locate httpReqTbl in MIB"));
				return;
			}

			ServiceMIB_ExportByAPI(_pServiceMib, "httpStat-Measure-Reset", HttpCRGService, *this, uint32, AsnType_Int32, &HttpCRGService::dummyGet, &HttpCRGService::snmp_resetStat, "");
/*
			ZQHttp::EngineStatistics& stat = ZQHttp::getEngineStatistics();
			uint32 idxRow =0;
			for (idxRow =1; idxRow< ZQHttp::METHOD_MAX; idxRow++) 
			{
				try
				{
					_pServiceMib->addTableCell(subOidTbl,  1, idxRow, new ZQ::SNMP::SNMPObjectDupValue("reqMethod",     ZQHttp::EngineStatistics::nameOfMethod(idxRow)));
					_pServiceMib->addTableCell(subOidTbl,  2, idxRow, new ZQ::SNMP::SNMPObject("reqCount",				stat._counters[idxRow].totalCount));
					_pServiceMib->addTableCell(subOidTbl,  3, idxRow, new ZQ::SNMP::SNMPObject("reqLatencyAvg",         stat._counters[idxRow].avgLatencyInMs_Header));
					_pServiceMib->addTableCell(subOidTbl,  4, idxRow, new ZQ::SNMP::SNMPObject("reqLatencyMax",			stat._counters[idxRow].maxLatencyInMs_Header));
					_pServiceMib->addTableCell(subOidTbl,  5, idxRow, new ZQ::SNMP::SNMPObject("resp2xx",               stat._counters[idxRow].respCount[ZQHttp::EngineStatistics::RESP_2XX]));
					_pServiceMib->addTableCell(subOidTbl,  6, idxRow, new ZQ::SNMP::SNMPObject("resp400",               stat._counters[idxRow].respCount[ZQHttp::EngineStatistics::RESP_400]));
					_pServiceMib->addTableCell(subOidTbl,  7, idxRow, new ZQ::SNMP::SNMPObject("resp404",               stat._counters[idxRow].respCount[ZQHttp::EngineStatistics::RESP_404]));
					_pServiceMib->addTableCell(subOidTbl,  8, idxRow, new ZQ::SNMP::SNMPObject("resp500",               stat._counters[idxRow].respCount[ZQHttp::EngineStatistics::RESP_500]));
					_pServiceMib->addTableCell(subOidTbl,  9, idxRow, new ZQ::SNMP::SNMPObject("resp503",               stat._counters[idxRow].respCount[ZQHttp::EngineStatistics::RESP_503]));
				}
				catch (...) 
				{
					(*m_pReporter)(ZQ::common::Log::L_WARNING, CLOGFMT(HttpCRGService, "doEnumSnmpExports() failed to register rowNo[%d] of httpReqTbl(%d)"), idxRow, ZQHttp::METHOD_MAX);
				}
			}

			// reset the stat by the way
			snmp_resetStat(0);

			(*m_pReporter)(ZQ::common::Log::L_INFO, CLOGFMT(HttpCRGService, "doEnumSnmpExports() registered httpReqTbl(%d)"), ZQHttp::METHOD_MAX);*/
		}

		HRESULT HttpCRGService::OnUnInit(void)
		{

			if(_crg)
			{
				try{
					delete _crg;
					_crg = 0;
				}catch(...)
				{
				}
			}

			//if(NULL != _httpCRGSnmp)
			//{
			//	try{
			//		delete _httpCRGSnmp;
			//		_httpCRGSnmp = 0;
			//	}catch(...)
			//	{
			//	}
			//}

			return BaseZQServiceApplication::OnUnInit();
		}


		HRESULT HttpCRGService::OnStart(void)
		{
#ifdef ZQ_OS_LINUX
			if(gConfig.fnLimit > 1024) {
				struct rlimit rlim;
				rlim.rlim_cur = gConfig.fnLimit;
				rlim.rlim_max = 640 * 1024;
				if(setrlimit( RLIMIT_NOFILE, &rlim) == 0) {
					(*m_pReporter)(::ZQ::common::Log::L_NOTICE, CLOGFMT(HttpCRGService, "OnStart() Adjust the max open file count to [%d] successfully"), gConfig.fnLimit);
				} else {
					int err = errno;
					(*m_pReporter)(::ZQ::common::Log::L_WARNING, CLOGFMT(HttpCRGService, "OnStart() Failed to adjust the max open file count to [%d]. error=%d:%s"), gConfig.fnLimit, err, strerror(err));
				}
			}
#endif
			try{
				for (std::set<std::string>::iterator iter = gConfig._pluginsConfig.modules.begin();
					iter != gConfig._pluginsConfig.modules.end(); iter++)
				{
					_crg->addModule(iter->c_str());
				}
				for (std::vector<PluginsConfig::ModuleConfigHolder>::iterator iter= gConfig._pluginsConfig._modules.begin();
					iter != gConfig._pluginsConfig._modules.end(); iter++)
				{
					_crg->addModule(iter->image.c_str());
				}
				_crg->start();

				//if (NULL == _httpCRGSnmp)
				//{
				//	_httpCRGSnmp = new ZQ::Snmp::Subagent(2200, 4);
				//          _httpCRGSnmp->setLogger(m_pReporter);
				//	 (*m_pReporter)(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpCRGService, "OnStart() create httpcrg snmp serviceId[2200], moduleId[4]"));
				//}

				//assert( _httpCRGSnmp != NULL );

				//using namespace  ZQ::Snmp;
				//int registerCount = 0;
				// ZQ::Snmp::ManagedPtr locateRequestTable(new TableMediator<LocateRequestTable, ZQHttp::EngineStatistics>(m_pReporter, ZQ::Snmp::Oid("1.1.1"), _httpCRGSnmp, ZQHttp::getEngineStatistics() ));  ++registerCount;
				//_httpCRGSnmp->addObject(ZQ::Snmp::Oid("1.1.1"), ZQ::Snmp::ManagedPtr(locateRequestTable));

				//   	_httpCRGSnmp->addObject( ZQ::Snmp::Oid("1.2"), ZQ::Snmp::ManagedPtr(new ZQ::Snmp::SimpleObject(VariablePtr(new HttpCRGMeasureSince(ZQHttp::getEngineStatistics())),  AsnType_Octets, aReadOnly))); ++registerCount;
				//_httpCRGSnmp->addObject( ZQ::Snmp::Oid("1.3"), ZQ::Snmp::ManagedPtr(new ZQ::Snmp::SimpleObject(VariablePtr(new HttpCRGMeasureReset(ZQHttp::getEngineStatistics())),  AsnType_Integer,  aReadWrite)));  ++registerCount;

				// (*m_pReporter)(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpCRGService, "OnStart() httpcrg snmp serviceId[2200], moduleId[4], register object[%d]"), registerCount);

				// _httpCRGSnmp->start();
			}
			catch (...)
			{
				return S_FALSE;
			}
			return BaseZQServiceApplication::OnStart();
		}

		HRESULT HttpCRGService::OnStop(void)
		{
			try{
				if (_crg)
					_crg->stop();

				//if (NULL != _httpCRGSnmp)
				//	_httpCRGSnmp->stop();
			}catch(...)
			{
				return S_FALSE;
			}

			return BaseZQServiceApplication::OnStop();
		}

		void HttpCRGService::snmp_resetStat(const uint32&)
		{
/*			ZQHttp::EngineStatistics& stat = ZQHttp::getEngineStatistics();
			stat.reset();

			static char stampStrSince[48];
			ZQ::common::TimeUtil::TimeToUTC(stat._mesureSince, stampStrSince, sizeof(stampStrSince)-2, true);
			_pServiceMib->addObject(new ZQ::SNMP::SNMPObject("httpStat-Since", stampStrSince));
			(*m_pReporter)(ZQ::common::Log::L_INFO, CLOGFMT(HttpCRGService, "snmp_resetStat() counters has been reset"));*/
		}

	} // namespace HttpCRG
} // namespace ZQTianShan
