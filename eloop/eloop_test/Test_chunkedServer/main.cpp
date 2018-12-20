#include <FileLog.h>
#include "HttpServer.h"
#include "HttpHandle.h"
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

	std::string logFilePath,host,workdir;
	int ch = 0,threadCount = 0;
	while((ch = getopt(argc,argv,"l:i:t:w:h")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'i':
			host = optarg;
			break;
		case 'w':
			workdir = optarg;
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

	printf("logfile = %s,workdir = %s,ip=%s,port = %d,threadCount = %d\n",logFilePath.c_str(),workdir.c_str(),ip.c_str(),port,threadCount);

	ZQ::eloop::HttpHandler::Properties appProps;
	appProps.insert(make_pair("workdir",workdir));

	ZQ::eloop::HttpBaseApplication::Ptr Download = new ZQ::eloop::DownloadeHandle::DownloadeHandleFactory(appProps);


	ZQ::eloop::HttpServer::HttpServerConfig conf;
	conf.host = ip;
	conf.port = port;
	conf.mode = ZQ::eloop::HttpServer::MULTIPE_LOOP_MODE;
	conf.threadCount = threadCount;
	conf.keepalive_timeout = 5000;

	conf.keepalive_timeout = 0;
	ZQ::eloop::HttpServer* server = new ZQ::eloop::HttpServer(conf,*pLog);


	server->mount("/download/.*",Download);


	server->startAt();


	while(1)
		SYS::sleep(10000);
	return 0;
}