#include "Download.h"

namespace ZQ {
	namespace eloop {


// ---------------------------------------
// class fetchm3u8
// ---------------------------------------
/*
fetchm3u8::fetchm3u8(ZQ::common::Log& logger)
:HttpClient(logger),_Logger(logger)
{

}
fetchm3u8::~fetchm3u8()
{

}

void fetchm3u8::dohttp(std::string& url)
{
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	beginRequest(msg,url);
}
}

bool fetchm3u8::onHeadersEnd( const HttpMessage::Ptr msg)
{
	printf("onHeadersEnd\n");
	return true;
}

bool fetchm3u8::onBodyData( const char* data, size_t size)
{
	printf("onBodyData\n");
	return true;
}

void fetchm3u8::onMessageCompleted()
{
	printf("onMessageCompleted\n");
}

void fetchm3u8::onParseError( int error,const char* errorDescription )
{
	printf("onParseError\n");
}

*/
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

void Download::dohttp(std::string& url)
{
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::POST);
	msg->url("*");

	//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	beginRequest(msg,url);
}


bool Download::onHeadersEnd( const HttpMessage::Ptr msg)
{
	_Response = msg;
	return true;
}

bool Download::onBodyData( const char* data, size_t size)
{
	return true;
}

void Download::onMessageCompleted()
{
	shutdown();
}

void Download::onParseError( int error,const char* errorDescription )
{
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