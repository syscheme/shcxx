#include <FileLog.h>
#include "HttpServer.h"
#include "ServerAgent.h"
#include "getopt.h"
#include <iostream>

void usage() {

	std::cout << "Usage: httpserver [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -i ip:port\n"
		<< "  -l <log file>         The log file output path\n"
		<< "  -p <home path>        This is the home dir\n"
		<< "  -s <resource path>    Thsi is the resource dir\n"
		<< "  -h                    display this screen\n"
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
	int ch = 0,threadCount = 4;
	std::string homedir;
	std::string sourcedir;
	while((ch = getopt(argc,argv,"l:i:t:h:p:s:")) != EOF)
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
		case 'p':
			homedir = optarg;
			break;
		case 's':
			sourcedir = optarg;
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


	ZQ::eloop::HttpHandler::Properties appProps;
	appProps.insert(make_pair("homedir",homedir));
	if(sourcedir.empty())
		sourcedir = homedir;

	appProps.insert(make_pair("sourcedir",sourcedir));

	ZQ::eloop::HttpHandler::AppPtr getVar = new ZQ::eloop::ServerAgent::App(*pLog);
	ZQ::eloop::HttpHandler::AppPtr loadfile = new ZQ::eloop::LoadFile::App(*pLog, appProps);
	
	ZQ::eloop::HttpServer::HttpServerConfig conf;
	conf.host = ip;
	conf.port = port;

	ZQ::eloop::HttpServer* server = new ZQ::eloop::HttpServer(conf,*pLog);;

	server->mount("/svar/.*",getVar);
	server->mount("/fvar/.*",loadfile);
	server->mount(".*",loadfile);
	server->startAt();

	while(1)
		SYS::sleep(10000);
	return 0;
}
