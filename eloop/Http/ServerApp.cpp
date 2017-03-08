#include <FileLog.h>
#include "HttpServer.h"
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
char gchar64_1024[64*1024] = {0};
//extern gchar64_1024;
int main(int argc,char* argv[])
{
	if (argc < 2)
	{
		usage();
		return -1;
	}

	std::string logFilePath,host;
	int ch = 0,threadCount = 4;
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

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);
	if (pLog == NULL)
	{
		printf("Failed to create a log file %s.\n",logFilePath.c_str());
		return -1;
	}

	FILE* fp = fopen("D:\\vedio\\test.txt","rb");
	//FILE* fp = fopen("/home/zhixiang.zhu/temp/test.txt","rb");
	int ret = fread(gchar64_1024,1,65536,fp);
	fclose(fp);
	//std::string body = gchar64_1024;
	printf("send body size = %d,ret = %d\n",sizeof(gchar64_1024),ret);

	std::string ip = host.substr(0,host.find(":"));
	int port = atoi((host.substr(host.find(":")+1)).c_str());

	ZQ::eloop::HttpBaseApplication::Ptr Test = new ZQ::eloop::TestHttpHandle::TestHttpHandleFactory();
	ZQ::eloop::HttpBaseApplication::Ptr Download = new ZQ::eloop::DownloadeHandle::DownloadeHandleFactory();
	ZQ::eloop::HttpBaseApplication::Ptr util = new ZQ::eloop::UtilHandle::UtilHandleFactory();
	ZQ::eloop::HttpBaseApplication::Ptr empty = new ZQ::eloop::EmptyHttpHandle::EmptyHttpHandleFactory();
	

	ZQ::eloop::HttpServer::HttpServerConfig conf;
	conf.host = ip;
	conf.port = port;
	conf.threadCount = threadCount;
	ZQ::eloop::HttpServer* server;
	if(threadCount <= 0)
		server = new ZQ::eloop::SingleLoopHttpServer(conf,*pLog);
	else
		server = new ZQ::eloop::MultipleLoopHttpServer(conf,*pLog);


	server->mount("/",Test);
	server->mount("/index.html",Test);
	server->mount("/download/hunan1.ts",Download);
	server->mount("/getValue/cpu",util);
	server->mount("/empty",empty);


	server->startAt();
	while(1)
		SYS::sleep(10000);
	return 0;
}