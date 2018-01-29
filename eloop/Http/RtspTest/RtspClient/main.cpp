#include "getopt.h"
#include <FileLog.h>
#include <iostream>
#include "eloop.h"
#include "RtspClientTest.h"

void usage() {

	std::cout << "Usage: RTSPClient [option] [arg]\n\n"
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

	ZQ::eloop::RTSPClientTest* client = new ZQ::eloop::RTSPClientTest(*pLog);
	client->init(loop);

	ZQ::eloop::RTSPMessage::Ptr req = new ZQ::eloop::RTSPMessage();

	req->url(url);
	req->method("OPTIONS");
	req->cSeq(1);
	req->header("Accept","application/sdp");




	std::string reqStr = req->toRaw();
	printf("sendRequest:%s",reqStr.c_str());
	

	return loop.run(ZQ::eloop::Loop::Default);
}
