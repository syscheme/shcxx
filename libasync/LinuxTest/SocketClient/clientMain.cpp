#include <stdlib.h>
#include <stdio.h>

#include "client.h"
#include "eventloop.h"
#include  "FileLog.h"
#include <sys/resource.h>
#include <writeThread.h>
#include <vector>

using namespace LibAsync;

int main()
{
	ZQ::common::FileLog testLog("Socket.log",7);
	SockClient::_lopCenter.startCenter(4);
	struct rlimit  rl;
	rl.rlim_cur = 64 * 1024;
	rl.rlim_max = 640 * 1024;
	if(setrlimit( RLIMIT_NOFILE, &rl) == 0)
	{
		testLog(ZQ::common::Log::L_NOTICE, CLOGFMT(Main, "set rlimit successful."));
	}		
	else
	{
		int err = errno;
		testLog(ZQ::common::Log::L_WARNING, CLOGFMT(Main, "set rlimit failed with error[%d]."), err);
	}
	writeThread::Ptr writePtr = new LibAsync::writeThread(testLog);
	writePtr->start();
	std::vector<SockClientPtr>  clientPtrs;
	for(int i = 0; i < 5000; i++)
	{
		char name[64];
		memset(name, '\0', sizeof(name));
		snprintf(name, sizeof(name), "./bin/hello_%d.bin", i);
		std::string strName(name);
		SockClientPtr clientPtr = new SockClient(testLog, "10.15.10.50", 12345, SockClient::_lopCenter.getLoop(), strName, writePtr);
		clientPtr->doConnect();
		clientPtrs.push_back(clientPtr);
		SYS::sleep(250);
	}
	//while(1);
	SYS::sleep(300000);
	writePtr->stop();
	while( !clientPtrs.empty() )
	{
		std::vector<SockClientPtr>::iterator iter = clientPtrs.begin();
		(*iter) = NULL;
		clientPtrs.erase(iter);
	}

	SockClient::_lopCenter.stopCenter();
	return 0;
}
