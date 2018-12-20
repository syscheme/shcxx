// TestDataPostHouse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <DataCommunicator.h>
#include <DataCommunicator_win.h>
#include <DataPostHouseWin.h>
#include "implementation.h"
#include <FileLog.h>

#include "DataCommunicatorSSL.h"


using namespace ZQ::DataPostHouse;

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

ZQ::common::FileLog consoleLogger;

LONG WINAPI handledExceptionFilter( struct _EXCEPTION_POINTERS* ExceptionInfo )
{
	consoleLogger.flush();
	return EXCEPTION_EXECUTE_HANDLER;
}


int test( )
{

	consoleLogger(ZQ::common::Log::L_DEBUG,"=============DataPostHouse is running============");

	IDataDialogFactoryPtr  factory = new DialogFactoryImpl();
	DataPostHouseEnv env;
	env.mLogger =&consoleLogger;
	env.dataFactory	= factory;
	DataPostDak dak(env,factory);
	if(!dak.startDak(5))
	{
		printf("failed to start dak");
		return -1;
	}

//	AServerSocketUdpPtr server =new AServerSocketUdp(dak,env);
//	if(!server->StartServer("0.0.0.0","2345"))
//	{
//		printf("failed to start server");		
//	}

 	/*AServerSocketTcpPtr server = new AServerSocketTcp(dak,env);
 	if(!server->StartServer("0.0.0.0","2345"))
 	{
 		printf("create server socket fail\n");
 		return -1;
 	}*/

	SSLServerPtr server = new SSLServer(dak, env);
	//server->setCertAndKeyFile("certificate.pem", "private.key");
	std::string strPath = ::getenv("ZQProjsPath");
	std::string strCertFile = strPath + "/Common/DataPostHouse/SSLCerts/certificate.pem";
	std::string strKeyFile = strPath + "/Common/DataPostHouse/SSLCerts/private.key";
	server->setCertAndKeyFile(strCertFile, strKeyFile);
	if(!server->startServer("192.168.81.98","2345"))
	{
		printf("create server socket fail\n");
		return -1;
	}

	SetConsoleCtrlHandler( HANDLER_ROUTINE , TRUE);
	while(!bQuit)
	{
		Sleep(500);
	}
	
	//Sleep(5000);

	server->stop();
	server = NULL;
	DWORD dwStart = GetTickCount();
	dak.stopDak();
	printf("stop dak [%d]\n",GetTickCount() - dwStart );
	return 0;
}
int _tmain(int argc, _TCHAR* argv[])
{	
	consoleLogger.open(".\\d.log",ZQ::common::Log::L_DEBUG);
	SetUnhandledExceptionFilter(handledExceptionFilter);

	//for(int i = 0 ; i < 1000 ; i++ )
		test();

	return 0;
}

