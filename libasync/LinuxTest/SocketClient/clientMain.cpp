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
	SockClient::_lopCenter.startCenter(11);
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
	for(int i = 6000; i < 8500; i++)
	{
		int na = i;
		while(na > 7499)
		{
			na = na % 7500 + 6000;
		}
		char name[64];
		memset(name, '\0', sizeof(name));
		snprintf(name, sizeof(name), "/mnt/fms/cdnss/%dABCD302131112172sctv.com.0X0000", na);
		std::string strName(name);
		SockClientPtr clientPtr = new SockClient(testLog, "10.6.6.190", 12345, SockClient::_lopCenter.getLoop(), strName, writePtr);
		clientPtr->doConnect();
		clientPtrs.push_back(clientPtr);
		SYS::sleep(10);
	}
	//while(1);
	//SYS::sleep(300000);
	testLog(ZQ::common::Log::L_WARNING, CLOGFMT(Main, "get client[%d]."), clientPtrs.size());
	int okNum = 0;
	while( okNum < clientPtrs.size() )
	{
		std::vector<SockClientPtr>::iterator iter = clientPtrs.begin();
		for(; iter != clientPtrs.end(); iter++)
		{
			if((*iter) != NULL && (*iter)->status() )
			{
				(*iter) = NULL;
				okNum ++ ;
			}
		}
		SYS::sleep(50);
	}
	clientPtrs.clear();
    writePtr->stop();
	SockClient::_lopCenter.stopCenter();
	return 0;
}
