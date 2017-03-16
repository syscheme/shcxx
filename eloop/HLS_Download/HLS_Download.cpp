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
		<< "  -t <thread count>					    thread count.default is 5.\n"
		<< "  -r <bitrate>							bitrate, default is 3.75mbps\n"
		<< "  -c <loop count>						loop count default is 4.\n"
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
	int threadCount = 5,LoopCount= 1,SessionInterval = 50;  //ms
	urlFile = "../download/hls";
	logFilePath = "../logs/hlsdownload.log";
	bitrate = "3750000";
	int meanValue = 300;

	int ch = 0;
	while((ch = getopt(argc,argv,"f:l:r:t:c:i:h")) != EOF)
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
		case 't':
			threadCount = atoi(optarg);
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

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);

	ZQ::eloop::DownloadThread::M3u8List m3u8list;
	std::list<ZQ::eloop::DownloadThread::M3u8List> ThreadList;
	std::ifstream fm3u8;
	std::string m3u8;
	int total = 1;
	fm3u8.open(urlFile.c_str());
	while (!fm3u8.eof())
	{
		getline(fm3u8,m3u8);
		if (m3u8.empty())
			continue;
		else
			total++;
	}
	fm3u8.close();
	meanValue = total/threadCount + 1;


	std::ifstream fin;
	fin.open(urlFile.c_str());
	int i = 1,n = 0;
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
		m3u8list.push_back(m3u8);
		if (i%meanValue == 0)
		{
			ThreadList.push_back(m3u8list);
			m3u8list.clear();
		}
		i++;
	}
	if (!m3u8list.empty())
	{
		ThreadList.push_back(m3u8list);
		m3u8list.clear();
	}
	printf("total = %d,threadCount = %d,meanValue = %d,ThreadList = %d\n",total,threadCount,meanValue,ThreadList.size());

	(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"urlFile = %d,"),urlFile.c_str());
	(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"thereadCount = %d,bitrate = %s,LoopCount=%d,SessionInterval=%d"),ThreadList.size(),bitrate.c_str(),LoopCount,SessionInterval);

	std::list<ZQ::eloop::DownloadThread::M3u8List>::iterator it;
	for (it = ThreadList.begin();it != ThreadList.end();it++)
	{
		ZQ::eloop::DownloadThread* download = new ZQ::eloop::DownloadThread(*pLog,*it,bitrate,SessionInterval,LoopCount);
		download->start();
	}

/*
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
*/

	while(1)
		SYS::sleep(100000);
	return 0;
}