// FileName : main.cpp
// Author   : Zheng Junming
// Date     : 2009-11
// Desc     : 

#include "FileLog.h"
#include "RtspDak.h"
#include "RtspEngine.h"
#include "RtspHandlerImpl.h"

#include <iostream>

bool bQuit = false;

BOOL WINAPI HANDLER_ROUTINE( DWORD CtrlType )
{
	if ( CtrlType == CTRL_C_EVENT )
	{
		bQuit = true;
		return TRUE;
	}
	return FALSE;
}

int main(int argc, char* argv[])
{
	ZQ::common::FileLog fileLog("c:\\log\\RtspEngineNew.log",  ::ZQ::common::Log::L_INFO, 10, 1024*1024*10, 1024, 1);
	if (argc != 3)
	{
		std::cout << "RtspSeverSample.exe ServerIP SeverPort" << std::endl;
		return 0;
	}

	// start rtpDak
	ZQRtspCommon::IRtspDak* rtspDak = new ZQRtspCommon::RtspDak(fileLog, 15, 20);
	ZQRtspCommon::IHandler* handler = new RtspSample::RtspHandlerImpl(fileLog);
	rtspDak->registerHandler(handler);
	rtspDak->start();

	ZQRtspEngine::RtspEngine* rtspEngine = new ZQRtspEngine::RtspEngine(fileLog, rtspDak);
	rtspEngine->startTCPRtsp(argv[1], "::", argv[2]);
	SetConsoleCtrlHandler( HANDLER_ROUTINE , TRUE);
	while(!bQuit)
	{
		Sleep(500);
	}
	rtspEngine->stopAllCommunicators();
	rtspEngine->release();

	rtspDak->stop();
	rtspDak->release();
	return 0;
	
}