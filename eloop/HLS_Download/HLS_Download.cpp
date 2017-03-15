#include <FileLog.h>
#include <iostream>
#include <fstream>
#include "getopt.h"
#include "Download.h"
#include "eloop.h"


void usage() {

	std::cout << "Usage: httpclient [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -l <log file>							The log file output path\n"
		<< "  -r <bitrate>							bitrate, default is 3.75mbps\n"
		<< "  -t <thread count>					    thread count\n"
		<< "  -s <Session count>					session count\n"
		<< "  -i <Session Interval>					The session time interval\n"
		<< "  -n <Thread Interval>					The thread time interval\n"
		<< "  -r <Download speed>					Download speed\n"
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

	std::string logFilePath,urlFile;
	int ThreadCount= 10,ThreadInterval = 0,SessionCount = 1000,SessionInterval = 5;
	std::string bitrate = "3750000";
	urlFile = "../download/hls";
	logFilePath = "../logs/hlsdownload.log";
	int ch = 0;
	while((ch = getopt(argc,argv,"f:l:r:t:s:i:h:")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'f':
			urlFile = optarg;
			break;
		case 'r':
			bitrate = optarg;
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
//	std::string m3u8 = "http://192.168.1.173:12000/assets?file=/HLS/HUNAN2999/2500000/index.m3u8&rate=375400";
	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);

/*
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
*/

	ZQ::eloop::Loop loop(false);

	std::ifstream fin;
	std::string m3u8;
	fin.open(urlFile.c_str());
	while (!fin.eof())
	{
		getline(fin,m3u8);
		if (m3u8.empty())
			continue;
		(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"m3u8:%s\n"),m3u8.c_str());
		ZQ::eloop::Session* session = new ZQ::eloop::Session(*pLog,bitrate);
		session->init(loop);
		session->dohttp(m3u8);
	}
	fin.close();

	loop.run(ZQ::eloop::Loop::Default);


	while(1)
		SYS::sleep(100000);
	return 0;
}
