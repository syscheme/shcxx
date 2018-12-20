// SSM_Process.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "SSM_Process.h"
#include <streamsmithmodule.h>

using namespace ZQ::common;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


// This is an example of an exported variable.
SSM_PROCESS_API int nSSM_Process=0;

// This is an example of an exported function.
SSM_PROCESS_API int fnSSM_Process(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see SSM_Process.h for the class definition
CSSM_Process::CSSM_Process()
{ 
	return; 
}

SSM_PROCESS_API RequestProcessResult SSM_ProcessFixupRequest(IStreamSmithSite* pSite
									  , IClientRequestWriter* pReq)
{
	return RequestProcessed;
}

SSM_PROCESS_API RequestProcessResult SSM_ProcessContentHandle(IStreamSmithSite* pSite
									   , IClientRequest* pReq)
{
	switch(pReq->getVerb())
	{
	case RTSP_MTHD_SETUP:{
		//IServerResponse * pResponse = pReq->getResponse();
		//pResponse->printf_preheader("response SETUP")
		//
	}
	case RTSP_MTHD_PLAY:{
		//
	}
	case RTSP_MTHD_PAUSE:{
		//
	}
	case RTSP_MTHD_TEARDOWN:{
		//
	}
	}
	return RequestProcessed;
}

SSM_PROCESS_API RequestProcessResult SSM_ProcessFixupResponse(IStreamSmithSite* pSite
													   , IClientRequest* pReq)
{
	return RequestProcessed;
}

SSM_PROCESS_API void SSM_ProcessSinkEvent(IStreamSmithSite* pSite, 
						EventType Type, ZQ::common::Variant& params)
{
	switch(Type)
	{
	case E_PLAYLIST_STARTED:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"Start PlayList.");
	}
	case E_PLAYLIST_STATECHANGED:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList state changed.");
	}
	case E_PLAYLIST_SPEEDCHANGED:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList speed changed.");
	}
	case E_PLAYLIST_INPROGRESS:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList in process.");
	}
	case E_PLAYLIST_ITEMLOADED:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList item loaded.");
	}
	case E_PLAYLIST_ITEMDONE:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList item done.");
	}
	case E_PLAYLIST_DONE:{
		pSite->CoreLog(ZQ::common::Log::L_DEBUG,"PlayList done.");
	}
	}
}

SSM_PROCESS_API void SSM_ProcessModuleInit(IStreamSmithSite* pSite)
{	
	pSite->CoreLog(Log::L_DEBUG, "SSM_Process.dll: Module Enter ModuleInit");
	LPTSTR handlerName;
	handlerName = "MOD";
	pSite->RegisterFixupRequest((SSMH_FixupRequest)SSM_ProcessFixupRequest);
	pSite->RegisterContentHandle(handlerName,(SSMH_ContentHandle)SSM_ProcessContentHandle);
	pSite->RegisterFixupResponse((SSMH_FixupResponse)SSM_ProcessFixupResponse);
	
	//hook some interested events.
	pSite->SinkEvent(E_PLAYLIST_STARTED | E_PLAYLIST_STATECHANGED | 
           E_PLAYLIST_SPEEDCHANGED | E_PLAYLIST_INPROGRESS | E_PLAYLIST_ITEMLOADED| 
		   E_PLAYLIST_ITEMDONE | E_PLAYLIST_DONE,(SSMH_EventSink)SSM_ProcessSinkEvent);
}

SSM_PROCESS_API void SSM_ProcessModuleUninit(IStreamSmithSite* pSite)
{
	pSite->CoreLog(Log::L_DEBUG, "SSM_Process.dll: Leave ModuleInit");
}