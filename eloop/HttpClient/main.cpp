#include <FileLog.h>
#include <iostream>
#include "getopt.h"
#include "downloadClient.h"



void usage() {

	std::cout << "Usage: httpclient [option] [arg]\n\n"
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

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);

//	for(int i = 0;i< 3;i++)
//	{
		DownloadThread* download = new DownloadThread(*pLog);
		download->start();
		//Sleep(1000);
//	}

	Loop loop(false);

	DownloadClient* client = new DownloadClient(*pLog);
	client->init(loop);
	client->dohttp();

	loop.run(Loop::Default);
	while(1);
	return 0;
}