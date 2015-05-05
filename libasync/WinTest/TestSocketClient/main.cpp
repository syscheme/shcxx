#include "TestSocket.h"
#include <FileLog.h>

int main(int argc, char**argv)
{
	ZQ::common::FileLog log("testSocketClient.log", ZQ::common::Log::L_DEBUG);

	const std::string ip	= "10.15.10.50";
	const unsigned int port = 9999;

	std::string sendStr = "timeout=10.5;codec=gb2312";

	LibAsync::EventLoop evloop;
	evloop.start();
	TestSocketPtr	testSockPtr = new TestSocket(log, evloop);


	bool bRet = testSockPtr->connect(ip.c_str(), port);

	while(true)
	{
		Sleep(10);
	}
	return 0;
}