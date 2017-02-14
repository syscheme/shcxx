#include <FileLog.h>
#include "httpServer.h"
#include "TestHttpHandle.h"
#include "getopt.h"
#include <iostream>

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
	int ch = 0,threadCount = 5;
	while((ch = getopt(argc,argv,"l:i:t:h:")) != EOF)
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

	ZQ::eloop::Loop loop(true);

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);
	if (pLog == NULL)
	{
		printf("Failed to create a log file %s.\n",logFilePath.c_str());
		return -1;
	}

	std::string ip = host.substr(0,host.find(":"));
	int port = atoi((host.substr(host.find(":")+1)).c_str());

	ZQ::eloop::HttpBaseApplication* Test = new ZQ::eloop::TestHttpHandle::App();


	ZQ::eloop::HttpServer::HttpServerConfig conf;
	ZQ::eloop::HttpServer* server = new ZQ::eloop::SingleLoopHttpServer(conf,*pLog);

	server->mount("/",Test);
	server->mount("/index.html",Test);


	server->startAt(ip.c_str(),port);
	return 0;
}