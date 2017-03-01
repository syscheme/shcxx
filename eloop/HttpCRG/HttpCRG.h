#ifndef __ELOOP_HTTPCRG_H__
#define __ELOOP_HTTPCRG_H__

#include "CRGateway.h"
#ifdef ZQ_OS_MSWIN
#include "BaseZQServiceApplication.h"
#else
#include "ZQDaemon.h"
#endif

// #include "snmp/ZQSnmpMgmt.hpp"
// #include "snmp/SubAgent.hpp"

namespace ZQTianShan {

	namespace HttpCRG {

		class HttpCRGService : public ZQ::common::BaseZQServiceApplication 
		{
		public:

			HttpCRGService ();
			virtual ~HttpCRGService ();

			HRESULT OnStart(void);
			HRESULT OnStop(void);
			HRESULT OnInit(void);
			HRESULT OnUnInit(void);

			void doEnumSnmpExports(void);

			::std::string					m_strProgramRootPath;
			::std::string					_strPluginFolder;
			::std::string					_strLogFolder;

			//	::ZQ::common::FileLog			_fileLog;

			//CRGateway obj
			CRG::CRGateway*                _crg;
			//	 ZQ::Snmp::Subagent *           _httpCRGSnmp;

		public:
			uint32 dummyGet(void) { return 1; }
			void snmp_resetStat(const uint32&);
		};

	}//namespace HttpCRG

}//namespace ZQTianShan

#endif __ZQTIANSHAN_HTTPCRG_H__
