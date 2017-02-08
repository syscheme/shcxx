#include <FileLog.h>
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

	std::string logFilePath;
	int ch = 0;
	while((ch = getopt(argc,argv,"l:h:")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	Loop loop(true);

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);
	if (pLog == NULL)
	{
		printf("Failed to create a log file %s.\n",logFilePath.c_str());
		return -1;
	}


	HttpApplication::Ptr Test = new TestHttpHandleFactory();


	HttpServer::HttpServerConfig conf;
	HttpServer server(conf,*pLog);

	server.registerApp("/",Test);
	server.registerApp("/index.html",Test);


	server.init(loop);
	server.startAt("192.168.81.28",9978);


	loop.run(Loop::Default);
	return 0;
}