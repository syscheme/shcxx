#include <FileLog.h>
#include <iostream>
#include "getopt.h"
#include "DownloadClient.h"



void usage() {

	std::cout << "Usage: httpclient [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -l <log file>							The log file output path\n"
		<< "  -t <thread count>					    thread count\n"
		<< "  -s <Session count>					session count\n"
		<< "  -i <Session Interval>					The session time interval\n"
		<< "  -n <Thread Interval>					The thread time interval\n"
		<< "  -h									display this screen\n"
		<< std::endl;
}

int main(int argc,char* argv[])
{

	if (argc < 2)
	{
		usage();
		return -1;
	}

	std::string logFilePath,url;
	int ThreadCount= 10,ThreadInterval = 0,SessionCount = 1000,SessionInterval = 5;
	int ch = 0;
	while((ch = getopt(argc,argv,"l:u:t:s:i:h")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'u':
			url = optarg;
			break;
		case 't':
			ThreadCount = atoi(optarg);;
			break;
		case 's':
			SessionCount = atoi(optarg);
			break;
		case 'n':
			ThreadInterval = atoi(optarg);
			break;
		case 'i':
			SessionInterval = atoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);

	//ZQ::eloop::DownloadThread* download[100];

	//printf("ThreadCount = %d,SessionCount = %d,SessionInterval = %d\n",ThreadCount,SessionCount,SessionInterval);
	(*pLog)(ZQ::common::Log::L_DEBUG,CLOGFMT(main,"ThreadCount = %d,SessionCount = %d,SessionInterval = %d"),ThreadCount,SessionCount,SessionInterval);
	for(int i = 0;i< ThreadCount;i++)
	{
		ZQ::eloop::DownloadThread* download = new ZQ::eloop::DownloadThread(url,*pLog,i,SessionInterval,SessionCount);
		download->start();
		(*pLog)(ZQ::common::Log::L_DEBUG,CLOGFMT(main,"the %d thread threadid = %d"),i,download->id());
		//SYS::sleep(400);
		//(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"%d the %d thread connection is successful"),download[i]->getCount(),i);
	}

/*	ZQ::eloop::Loop loop(false);

	ZQ::eloop::DownloadClient* client = new ZQ::eloop::DownloadClient(*pLog);
	client->init(loop);
	//client->bind4("192.168.81.28",40001);
	std::string downloadurl = url.substr(0,url.find_last_of("/"));
	downloadurl = downloadurl + "/download/hunan1.ts";
	client->dohttp(downloadurl);

	/*ZQ::eloop::EmptyClient* client = new ZQ::eloop::EmptyClient(*pLog,NULL);
	client->init(loop);
	client->dohttp(url);*/
//	loop.run(ZQ::eloop::Loop::Default);


	while(1)
		SYS::sleep(100000);
	return 0;
}
