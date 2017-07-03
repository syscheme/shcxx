#include <FileLog.h>
#include <iostream>
#include "getopt.h"
#include "Download.h"



void usage() {

	std::cout << "Usage: httpclient [option] [arg]\n\n"
		<< "Options:\n"
		<< "  -l <log file>							The log file output path\n"

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

	std::string logFilePath,url,filename;
	int ch = 0;
	while((ch = getopt(argc,argv,"l:u:f:h")) != EOF)
	{
		switch(ch)
		{
		case 'l':
			logFilePath = optarg;
			break;
		case 'u':
			url = optarg;
			break;
		case 'f':
			filename = optarg;
			break;
		case 'h':
			usage();
			return 0;
		default:
			break;
		}
	}

	ZQ::common::Log* pLog = new ZQ::common::FileLog(logFilePath.c_str(),7, 5,10240000);

	
	ZQ::eloop::Loop loop(false);

	ZQ::eloop::DownloadClient* client = new ZQ::eloop::DownloadClient(*pLog,filename);
	client->init(loop);
	client->dohttp(url);


	loop.run(ZQ::eloop::Loop::Default);


	while(1)
		SYS::sleep(100000);
	return 0;
}
