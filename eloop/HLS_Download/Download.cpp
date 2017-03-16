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
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"downloading:%s,url:%s"),_CurrentDownloadFileName.c_str(),url.c_str());
		printf("downloading:%s,url:%s\n",_CurrentDownloadFileName.c_str(),url.c_str());
	}	
}

void Download::onHttpDataSent()
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"send suc."));
}

void Download::onHttpDataReceived( size_t size )
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"recv data size = %d."),size);
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
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"download complete:%s,size:%dByte,take:%dms,bitrate:%dkbps"),_CurrentDownloadFileName.c_str(),_totalSize,_totalTime,speed);
	printf("download complete:%s,size:%dByte,take:%dms,bitrate:%dkbps\n",_CurrentDownloadFileName.c_str(),_totalSize,_totalTime,speed);
}

void Download::onError( int error,const char* errorDescription )
{
	if (error != elpe__EOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Download Error,errorCode[%d],Description:%s"),error,errorDescription);
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

void Session::onHttpDataSent()
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"send suc."));
}

void Session::onHttpDataReceived( size_t size )
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"recv data size = %d."),size);
}

bool Session::onHeadersEnd( const HttpMessage::Ptr msg)
{
	return true;
}

bool Session::onBodyData( const char* data, size_t size)
{
	std::string body;
	body.append(data,size);
	_RespBody << body;

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
		//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"outstr:%s"),str.c_str());
		d->init(get_loop());
		d->dohttp(str,outstr);
	}
	_RespBody.str("");

}

void Session::onError( int error,const char* errorDescription )
{
	if (error != elpe__EOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session Error,errorCode[%d],Description:%s"),error,errorDescription);
		printf("Session Error,errorCode[%d],Description:%s\n",error,errorDescription);
	}
	shutdown();
}


// ---------------------------------------
// class DownloadThread
// ---------------------------------------
DownloadThread::DownloadThread(ZQ::common::Log& logger,M3u8List m3u8list,std::string& bitrate,int interval,int loopcount)
	:_Logger(logger),
	_m3u8list(m3u8list),
	_bitrate(bitrate),
	_IntervalTime(interval),
	_loopCount(loopcount)
{

}

DownloadThread::~DownloadThread()
{

}

int DownloadThread::run(void)
{
	Loop loop(false);
	M3u8List::iterator it;
	std::string m3u8;
	while(!_m3u8list.empty())
	{
		m3u8 = _m3u8list.front();
		for(int i=0;i< _loopCount;i++)
		{
			ZQ::eloop::Session* s = new ZQ::eloop::Session(_Logger,_bitrate);
			s->init(loop);
			s->dohttp(m3u8);
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(DownloadThread,"m3u8:%s"),m3u8.c_str());
			SYS::sleep(_IntervalTime);
		}
		_m3u8list.pop_front();
	}
	_m3u8list.clear();
	loop.run(Loop::Default);
	return 0;
}

} }//namespace ZQ::eloop
