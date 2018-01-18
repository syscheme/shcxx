#include "RTSPServer.h"
#include "getopt.h"
#include <FileLog.h>
#include <iostream>
#include "RtspHandler.h"

void usage() {
	std::cout << "Usage: httpserver [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -l <log file>					    The log file output path\n"
		<< "  -h								display this screen\n"
		<< std::endl;
}

int main(int argc,char* argv[])
{
	if (argc < 2)
	{
		usage();
		return -1;
	}

	std::string logFilePath,host;
	int ch = 0,threadCount = 0;
	while((ch = getopt(argc,argv,"l:i:t:h")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'i':
			host = optarg;
			break;
		case 't':
			threadCount = atoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);
	if (pLog == NULL)
	{
		printf("Failed to create a log file %s.\n",logFilePath.c_str());
		return -1;
	}

	std::string ip = host.substr(0,host.find(":"));
	int port = atoi((host.substr(host.find(":")+1)).c_str());

	printf("logfile = %s,workdir = %s,ip=%s,port = %d,threadCount = %d\n",logFilePath.c_str(),ip.c_str(), port, threadCount);


	ZQ::eloop::RTSPHandler::AppPtr RtspTest = new ZQ::eloop::RTSPTestHandler::RTSPTestHandleFactory(*pLog);


	ZQ::eloop::TCPServer::ServerConfig conf;
	conf.host = ip;
	conf.port = port;
	conf.mode = ZQ::eloop::TCPServer::MULTIPE_LOOP_MODE;
	conf.threadCount = threadCount;
	
	ZQ::eloop::RTSPServer* server = new ZQ::eloop::RTSPServer(conf,*pLog);



	server->mount("/RtspTest/.*",RtspTest);
	server->start();

	while(1)
		SYS::sleep(10000);
	return 0;
}