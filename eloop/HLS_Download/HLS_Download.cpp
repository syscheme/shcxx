#include <FileLog.h>
#include <iostream>
#include <fstream>
#include "getopt.h"
#include "Download.h"
#include "eloop.h"


void usage() {

	std::cout << "Usage: HLSDownload [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -f <url file>							url file default is \"../download/hls\"\n"
		<< "  -l <log file>							The log file output path.default is \"../logs/hlsdownload.log\"\n"
		<< "  -r <bitrate>							bitrate, default is 3.75mbps\n"
		<< "  -c <loop count>						loop count default is 4"
		<< "  -i <Session Interval>					The session time interval default is 50 ms\n"
		<< "  -h									display this screen\n"
		<< std::endl;
}

int main(int argc,char* argv[])
{

/*	if (argc < 2)
	{
		usage();
		return -1;
	}
*/
	std::string logFilePath,urlFile,bitrate;
	int LoopCount= 1,SessionInterval = 50;  //ms
	urlFile = "../download/hls";
	logFilePath = "../logs/hlsdownload.log";
	bitrate = "3750000";

	int ch = 0;
	while((ch = getopt(argc,argv,"f:l:r:c:i:h")) != EOF)
	{
		switch(ch)
		{
		case 'f':
			urlFile = optarg;
			break;
		case 'l':
			logFilePath = optarg;
			break;
		case 'r':
			bitrate = optarg;
			break;
		case 'c':
			LoopCount = atoi(optarg);;
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


/*	int ThreadCount = 5;
	ZQ::eloop::DownloadThread* download[5];

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
	size_t n = 0;
	fin.open(urlFile.c_str());
	while (!fin.eof())
	{
		getline(fin,m3u8);
		if (m3u8.empty())
			continue;
		n = m3u8.find_last_not_of(" \r\n\t");
		if (n != m3u8.npos)
		{
			m3u8.erase(n+1,m3u8.size()-n);
		}
		(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"m3u8:%s"),m3u8.c_str());
		ZQ::eloop::Session* session = new ZQ::eloop::Session(*pLog,bitrate);
		session->init(loop);
		session->dohttp(m3u8);
		SYS::sleep(SessionInterval);
	}
	fin.close();

	loop.run(ZQ::eloop::Loop::Default);


	while(1)
		SYS::sleep(100000);
	return 0;
}
