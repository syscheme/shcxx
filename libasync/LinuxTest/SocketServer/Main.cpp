#include "SocketServer.h"
#include "FileLog.h"
#include <SystemUtils.h>
using namespace LibAsync;

int main()
{
	SocketServer::_lopCenter.startCenter(10);

	ZQ::common::FileLog   serverlog("./log/server.log", 7);
	SocketServerPtr server = new SocketServer(SocketServer::_lopCenter.getLoop(), "10.6.6.190", 12345, serverlog);
	
	if (server->startServer(10000) )
	{
		while(!server->getError())
		{
			SYS::sleep(1000);
		}
		SYS::sleep(5000);
		serverlog(ZQ::common::Log::L_INFO, CLOGFMT(Main, "main() will exit."));
	}
	server = NULL;
	SocketServer::_lopCenter.stopCenter();
	return 0;
}