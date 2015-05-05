#include "TestSocketServer.h"
#include <InetAddr.h>
#include <FileLog.h>
#include <libasync/http.h>

ZQ::common::FileLog log("C:\\testSocketServer.log", ZQ::common::Log::L_DEBUG);

int main(int argc, char **argv)
{
	LibAsync::HttpProcessor::setup(4);
	LibAsync::EventLoop evloop;
	evloop.start();

	LibAsync::TestSocketServerPtr serverPtr = new LibAsync::TestSocketServer(log, evloop);

	//std::string strServerIP = ZQ::common::InetHostAddress::getLocalAddress().getHostAddress();
	std::string strServerIP = "192.168.86.140";
	const unsigned int serverPort = 8321;

	if (!serverPtr->bind(strServerIP, serverPort))
	{
		log(ZQ::common::Log::L_DEBUG, CLOGFMT(Main, "failed to bind ip"));
		return 0;
	}

	if (!serverPtr->accept())
	{
		log(ZQ::common::Log::L_DEBUG, CLOGFMT(Main, "failed to accept"));
		return 0;
	}

	while(true)
	{	
		Sleep(100);
	}
	return 0;
}