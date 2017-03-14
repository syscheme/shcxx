#include "Download.h"
#include <urlstr.h>
#include <TimeUtil.h>
namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class Session
// ---------------------------------------
Session::Session(ZQ::common::Log& logger)
:HttpClient(logger),_Logger(logger)
{

}
Session::~Session()
{

}

void Session::fetchm3u8(std::string& m3u8url)
{
	_fetchm3u8Completed = false;
	ZQ::common::URLStr urlstr(m3u8url.c_str());

	//change uri, host in msg
	std::string m3u8Path = urlstr.getPathAndParam();
	_basePath = m3u8Path.substr(0,m3u8Path.find_last_of("/"));


	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::POST);
	msg->url("*");

	//	printf("url = %s\n",url.c_str());
	//beginRequest(msg,"http://192.168.81.28:9978/empty");
	beginRequest(msg,m3u8url);
}

int Session::DownloadCurrentFile()
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
}


bool Session::onHeadersEnd( const HttpMessage::Ptr msg)
{
	//_Response = msg;

	return true;
}

bool Session::onBodyData( const char* data, size_t size)
{
	if (!_fetchm3u8Completed)
	{
		//_RespBody.append(data,size);
		_RespBody << data;
	}
	else
	{
		_totalSize += size;
	}

	return true;
}

void Session::onMessageCompleted()
{
	if (!_fetchm3u8Completed)
	{
		_fetchm3u8Completed = true;
		std::string outstr;
		while(std::getline(_RespBody,outstr))
		{
			if (outstr.find("#") != outstr.npos)
				continue;
			_file.push_back(outstr);
		}
		_RespBody.str("");
	}

	if (!_CurrentDownloadFileName.empty())
	{
		_totalTime = ZQ::common::now() - _startTime;
		double speed = _totalSize * 8/_totalSize;
		printf("download complete:%s,size:%dByte,bitrate:%dkbps\n",_CurrentDownloadFileName.c_str(),_totalSize,speed);
		_CurrentDownloadFileName.clear();
	}

	if (!_file.empty())
	{
		_CurrentDownloadFileName = _file.front();
		_file.pop_front();
		DownloadCurrentFile();
		_totalSize = 0;
	}
	
}

void Session::onError( int error,const char* errorDescription )
{
	printf("onError\n");

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