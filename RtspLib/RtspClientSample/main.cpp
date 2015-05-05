#include "FileLog.h"
#include "RtspDak.h"
#include "RtspHandlerImpl.h"
#include "RtspClientFactory.h"
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
	ZQ::common::FileLog fileLog("c:\\log\\RtspClient.log", 7);
	ZQRtspCommon::IRtspDak* rtspDak = new ZQRtspCommon::RtspDak(fileLog, 15, 20);
	if (argc != 3)
	{
		std::cout << "RtspSeverSample.exe ServerIP SeverPort" << std::endl;
		return 0;
	}


	ZQRtspCommon::IHandler* handler = new RtspSample::RtspHandlerImpl(fileLog);
	rtspDak->registerHandler(handler);
	rtspDak->start();

	ZQRtspEngine::RtspClientFactory fac(fileLog, rtspDak);
	ZQRtspEngine::RtspClientPtr rtspClient = fac.createRtspClient(argv[1], argv[2]);
	if (rtspClient)
	{
		ZQRtspCommon::IRtspSendMsg* request = rtspClient->getRequest();
		request->setStartline("SETUP rtsp://192.168.81.120:554 RTSP/1.0");
		request->setHeader("CSeq","313");
		request->post();
		request->release();
		rtspClient = NULL;
	}
	

	SetConsoleCtrlHandler( HANDLER_ROUTINE, TRUE);
	while(!bQuit)
	{
		Sleep(500);
	}
	rtspDak->stop();
	rtspDak->release();
	return 0;
}