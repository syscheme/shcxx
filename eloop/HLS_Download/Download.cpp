#include "Download.h"
#include <urlstr.h>
#include <TimeUtil.h>
namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class Download
// ---------------------------------------
Download::Download(ZQ::common::Log& logger)
	:HttpClient(logger),_Logger(logger)
{

}
Download::~Download()
{

}

void Download::dohttp(std::string& url,std::string filenaem)
{
	_CurrentDownloadFileName = filenaem;
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	if (beginRequest(msg,url))
	{
		_startTime = ZQ::common::now();
		printf("downloading:%s,url:%s\n",_CurrentDownloadFileName.c_str(),url.c_str());
	}	
}

bool Download::onHeadersEnd( const HttpMessage::Ptr msg)
{
	_totalSize = 0;
	return true;
}

bool Download::onBodyData( const char* data, size_t size)
{
	_totalSize += size;
	return true;
}

void Download::onMessageCompleted()
{
	_totalTime = ZQ::common::now() - _startTime;
	double speed = _totalSize * 8/_totalSize;
	printf("download complete:%s,size:%dByte,bitrate:%dkbps\n",_CurrentDownloadFileName.c_str(),_totalSize,speed);
}

void Download::onError( int error,const char* errorDescription )
{
	printf("Download onError\n");
}
// ---------------------------------------
// class fetchm3u8
// ---------------------------------------
fetchm3u8::fetchm3u8(ZQ::common::Log& logger)
:HttpClient(logger),_Logger(logger)
{

}
fetchm3u8::~fetchm3u8()
{

}

void fetchm3u8::getm3u8(std::string& m3u8url)
{

	_baseurl = m3u8url.substr(0,m3u8url.find_last_of("/"));


	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	beginRequest(msg,m3u8url);
}
/*
int fetchm3u8::DownloadCurrentFile()
{
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	std::string path = _basePath + "/" + _CurrentDownloadFileName;
	msg->url(path);

//	msg->header("Host",host);
	msg->header("Connection", "Keep-Alive");

	printf("downloading:%s,path:%s\n",_CurrentDownloadFileName.c_str(),path.c_str());
	std::string str = msg->toRaw();
	return write(str.c_str(),str.length());
}*/


bool fetchm3u8::onHeadersEnd( const HttpMessage::Ptr msg)
{
	//_Response = msg;

	return true;
}

bool fetchm3u8::onBodyData( const char* data, size_t size)
{
	_RespBody << data;

	return true;
}

void fetchm3u8::onMessageCompleted()
{

	std::string outstr;
	while(std::getline(_RespBody,outstr))
	{
		if (outstr.find("#") != outstr.npos)
			continue;

		Download* d = new Download(_Logger);
		std::string str = _baseurl + "/" + outstr;
		d->dohttp(str,outstr);
	}
	_RespBody.str("");

}

void fetchm3u8::onError( int error,const char* errorDescription )
{
	printf("fetchm3u8 onError\n");

}


// ---------------------------------------
// class DownloadThread
// ---------------------------------------
DownloadThread::DownloadThread(std::string& url,ZQ::common::Log& logger,int count,int interval,int session)
	:_Logger(logger),
	_url(url)
{

}

DownloadThread::~DownloadThread()
{

}

int DownloadThread::run(void)
{

	return 0;
}

} }//namespace ZQ::eloop