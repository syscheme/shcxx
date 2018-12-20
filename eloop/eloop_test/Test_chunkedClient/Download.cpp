#include "Download.h"
#include <ostream>
#include <urlstr.h>
namespace ZQ {
	namespace eloop {
// ---------------------------------------
// class DownloadClient
// ---------------------------------------
DownloadClient::DownloadClient(ZQ::common::Log& logger,const std::string& filename)
	:HttpClient(logger),
	_fp(NULL),
	_Logger(logger),
	_count(0),
	_filename(filename)
{

}

DownloadClient::~DownloadClient()
{

}

void DownloadClient::dohttp(std::string& url)
{
	_url = url;
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->keepAlive(true);
	msg->url("*");

	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(DownloadClient,"client count %d"),m_count);


	beginRequest(msg,url);
}

bool DownloadClient::onHeadersEnd( const HttpMessage::Ptr msg)
{
	_Response = msg;
	std::ostringstream stream;
	stream<<_count;
	std::string file = _filename.substr(0,_filename.find_last_of('.'));
	std::string suffix = _filename.substr(_filename.find_last_of('.'));

	file = file + stream.str() + suffix;

	printf("filename = %s\n",file.c_str());

	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(DownloadClient,"filename = %s"),file.c_str());

	_fp = fopen(file.c_str(),"w+");
	if (_fp == NULL)
	{
		return false;
	}
	_count++;
	return true;
}

bool DownloadClient::onBodyData( const char* data, size_t size)
{
	//_RespBody.append(data,size);
//	std::string body;
//	body.append(data,size);
//	printf("onBodyData data:%s,size:%d\n",body.c_str(),size);
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(DownloadClient,"onBodyData data:%s,len = %d"),data,size);
	int ret = fwrite(data,1,size,_fp);
	if (ret != size)
	{
		closefile();
	}
	return true;
}

void DownloadClient::onMessageCompleted()
{
	printf("DownloadClient http end!\n");
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(DownloadClient,"DownloadClient http end!"));

	closefile();
//	shutdown();
}

void DownloadClient::onError(  int error,const char* errorDescription )
{
	printf("DownloadClient %s\n",errorDescription);
	closefile();
	HttpClient::onError(error,errorDescription);
}

void DownloadClient::closefile()
{
	if (_fp != NULL)
	{
		fclose(_fp);
	}	
}
} }//namespace ZQ::eloop