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
	
	int64 speed = _totalSize * 8/_totalTime;
	printf("download complete:%s,size:%dByte,bitrate:%dkbps\n",_CurrentDownloadFileName.c_str(),_totalSize,speed);
}

void Download::onError( int error,const char* errorDescription )
{
	if (error != elpe__EOF)
	{
		printf("Download Error,errorCode[%d],Description:%s\n",error,errorDescription);
	}
	shutdown();
}
// ---------------------------------------
// class Session
// ---------------------------------------
Session::Session(ZQ::common::Log& logger,std::string bitrate)
:HttpClient(logger),_Logger(logger),_bitrate(bitrate)
{

}
Session::~Session()
{

}

void Session::dohttp(std::string& m3u8url)
{
	_RespBody.str("");
	_baseurl = m3u8url.substr(0,m3u8url.find_last_of("/"));
	std::string fetchm3u8 = m3u8url + "&rate="+ _bitrate;

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	beginRequest(msg,fetchm3u8);
}

bool Session::onHeadersEnd( const HttpMessage::Ptr msg)
{
	return true;
}

bool Session::onBodyData( const char* data, size_t size)
{
	_RespBody << data;

	return true;
}

void Session::onMessageCompleted()
{

	std::string outstr;
	while(std::getline(_RespBody,outstr))
	{
		if (outstr.find("#") != outstr.npos)
			continue;

		Download* d = new Download(_Logger);
		std::string str = _baseurl + "/" + outstr + "&rate="+ _bitrate;
		//printf("str = %s\n",str.c_str());
		d->init(get_loop());
		d->dohttp(str,outstr);
	}
	_RespBody.str("");

}

void Session::onError( int error,const char* errorDescription )
{
	if (error != elpe__EOF)
	{
		printf("Session Error,errorCode[%d],Description:%s\n",error,errorDescription);
	}
	shutdown();
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
