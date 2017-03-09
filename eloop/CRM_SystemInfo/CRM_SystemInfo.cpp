#include <CRMInterface.h>
#include <FileLog.h>
#include "SystemInfoRequestHandler.h"


#ifdef ZQ_OS_MSWIN
BOOL APIENTRY DllMain( HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
#endif

#define SYSTEMINFOURI "/mvar/.*"

ZQ::common::FileLog* pLog;
static SystemInfoRequestHandler* pSystemInfoRequestHandler;

extern "C"
{
	__EXPORT bool CRM_Entry_Init(CRG::ICRMManager* mgr)
	{
		std::string logpath = mgr->getLogFolder() + "CRM_SystemInfo.log";
		try
		{
			pLog = new ZQ::common::FileLog(logpath.c_str(),7, 5,10240000,204800);
		}catch(ZQ::common::FileLogException& e)
		{
			mgr->superLogger()(ZQ::common::Log::L_ERROR, CLOGFMT(SystemInfo, "Failed to create logger at %s. detail:%s"), logpath.c_str(), e.getString());
			return false;
		}

		pSystemInfoRequestHandler = new SystemInfoRequestHandler(*pLog);

		mgr->registerContentHandler(SYSTEMINFOURI,pSystemInfoRequestHandler);
		printf("CRM_Entry_Init\n");
		return true;
	}

	__EXPORT void CRM_Entry_Uninit(CRG::ICRMManager* mgr)
	{
		printf("CRM_Entry_Uninit\n");

		mgr->unregisterContentHandler(SYSTEMINFOURI,pSystemInfoRequestHandler);

		if(pSystemInfoRequestHandler)
		{
			delete pSystemInfoRequestHandler;
			pSystemInfoRequestHandler = NULL;
		}

		if(pLog)
		{
			delete pLog;
			pLog = NULL;
		}

	}
}