#include <ZQ_common_conf.h>
#include <FileLog.h>
#include <iostream>
#include <fstream>
#include "getopt.h"
#include "Download.h"
#include "eloop.h"

#ifdef ZQ_OS_LINUX
#include <sys/resource.h>
#endif


void usage() {

	std::cout << "Usage: HLSDownload [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -f <url file>							url file default is \"../download/hls\"\n"
		<< "  -l <log file>							The log file output path.default is \"../logs/hlsdownload.log\"\n"
		<< "  -t <thread count>					    thread count.default is 5.\n"
		<< "  -r <bitrate>							bitrate, default is 3750000bit/s\n"
		<< "  -c <loop count>						loop count default is 1.\n"
		<< "  -i <Session Interval>					The session time interval default is 50 ms\n"
		<< "  -o <Server Object>					The server object is Aqua or EdgeFE. default is \"EdgeFE\"\n"
		<< "  -s <speed limit>						Local speed limit, the default is -1 that is not limited\n"
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
	std::string objServer = "EdgeFE";
	int64 limit = -1; 

	int ch = 0;
	while((ch = getopt(argc,argv,"f:l:r:t:c:i:o:s:h")) != EOF)
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
		case 'o':
			objServer = optarg;
			break;
		case 's':
			limit = atoi(optarg);
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7,20,52428800);

	ZQ::eloop::Download::ObjServer obj;
	if (objServer.find("EdgeFe") != objServer.npos)
		obj = ZQ::eloop::Download::EdgeFE;
	else if (objServer.find("Aqua") != objServer.npos)
		obj = ZQ::eloop::Download::Aqua;
	else
		obj =  ZQ::eloop::Download::Def;
	
	

#ifdef ZQ_OS_LINUX
	struct rlimit rlim;
	rlim.rlim_cur = 64*1024;
	rlim.rlim_max = 640*1024;
	setrlimit( RLIMIT_NOFILE, &rlim);
#endif

	ZQ::eloop::DownloadThread::M3u8List m3u8list;
	std::list<ZQ::eloop::DownloadThread::M3u8List> ThreadList;
	std::ifstream fm3u8;
	std::string m3u8;
	int total = 1;
	fm3u8.open(urlFile.c_str());
	if (!fm3u8)
	{
		printf("open file %s failed!\n",urlFile.c_str());
		return -1;
	}

	(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"urlFile = %s,"),urlFile.c_str());
	(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"urlFile = %s,objServer=%s,limit = %d,threadCount = %d,bitrate = %s,LoopCount=%d,SessionInterval=%d"),urlFile.c_str(),objServer.c_str(),limit,threadCount,bitrate.c_str(),LoopCount,SessionInterval);
	
	while (!fm3u8.eof())
	{
		getline(fm3u8,m3u8);
		if (m3u8.empty())
			continue;
		else
			total++;
	}
	fm3u8.close();

	if (total > threadCount)
	{
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
		//printf("total = %d,threadCount = %d,meanValue = %d,ThreadList = %d\n",total,threadCount,meanValue,ThreadList.size());

		std::vector<ZQ::common::NativeThread*> ThreadVec;
		while(!ThreadList.empty())
		{
			ZQ::eloop::DownloadThread* download = new ZQ::eloop::DownloadThread(*pLog,ThreadList.front(),bitrate,SessionInterval,LoopCount,limit,obj);
			download->start();
			ThreadList.pop_front();
			ThreadVec.push_back(download);
		}
		for(int m =0;m<ThreadVec.size();m++)
		{
			ThreadVec[m]->waitHandle(-1);
			(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"The %d thread exit! thread id:%d"),m,ThreadVec[m]->id());
		}
	}
	else
	{
		
		ZQ::eloop::Loop loop(false);

		ZQ::eloop::LoopRateMonitor loopRate;
		loopRate.init(loop);
		loopRate.startAt();

		std::ifstream fin;
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
			for (int i=0;i< LoopCount;i++)
			{
				(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"m3u8:%s"),m3u8.c_str());
				ZQ::eloop::Session* session = new ZQ::eloop::Session(*pLog,bitrate,limit,obj,loopRate);
				session->init(loop);
				session->dohttp(m3u8);
				SYS::sleep(SessionInterval);
			}
		}
		fin.close();

		loop.run(ZQ::eloop::Loop::Default);
	}
	(*pLog)(ZQ::common::Log::L_DEBUG, CLOGFMT(main,"The main thread to exit!"));
	pLog->flush();
	return 0;
}
