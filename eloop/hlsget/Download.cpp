#include "Download.h"
#include <urlstr.h>
#include <TimeUtil.h>
namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class Download
// ---------------------------------------
Download::Download(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,Statistics stat,std::list<std::string> file)
	:HttpClient(logger),
	_Logger(logger),
	_baseurl(baseurl),
	_bitrate(bitrate),
	_stat(stat),
	_totalSize(0),
	_completed(false),
	_file(file)
{
}
Download::~Download()
{
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"The Download destructor to exit!"));
}

void Download::dohttp()
{
	_CurrentDownloadFileName = _file.front();
	std::string url = _baseurl + "/" + _CurrentDownloadFileName + "&rate="+ _bitrate;
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	beginRequest(msg,url);	
}

void Download::OnConnected(ElpeError status)
{
	HttpClient::OnConnected(status);
	_startTime = ZQ::common::now();
	std::string url = _baseurl + "/" + _CurrentDownloadFileName;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"downloading:%s"),url.c_str());
	printf("downloading:%s\n",url.c_str());

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
	if (_stat.CompletionTime != 0)
	{
		int64 interval = ZQ::common::now() -_stat.CompletionTime;
		_stat.allInterval += interval;
		if (_stat.MaxInterval < interval)
		{
			_stat.MaxInterval = interval;
			_stat.file1 = _stat.prevFile;
			_stat.file2 = _CurrentDownloadFileName;
		}	
	}
	_stat.prevFile = _CurrentDownloadFileName;
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
	_completed = true;
	_stat.allSize += _totalSize;
	int64 totalTime = ZQ::common::now() - _startTime;

	_stat.CompletionTime = ZQ::common::now();	 
	
	int64 speed = _totalSize * 8/totalTime;
	std::string url = _baseurl + "/" + _CurrentDownloadFileName;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"download complete:%s,size:%dByte,take:%dms,bitrate:%dkbps"),url.c_str(),_totalSize,totalTime,speed);
	printf("download complete:%s,size:%dByte,take:%dms,bitrate:%dkbps\n",url.c_str(),_totalSize,totalTime,speed);
	_file.pop_front();
	if (!_file.empty())
	{
		Download* d = new Download(_Logger,_baseurl,_bitrate,_stat,_file);
		//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"outstr:%s"),str.c_str());
		d->init(get_loop());
		d->dohttp();
	}
	else
	{
		int64 tm = ZQ::common::now() - _stat.allStartTime;
		int64 sp = _stat.allSize * 8/tm;
		int64 Average = _stat.allInterval /(_stat.fileTotal - 1);
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"%d files downloaded to complete,directory:%s,size:%dByte,take:%dms,bitrate:%dkbps,Average interval: %d ms,The maximum time between %s and %s is : %d ms"),_stat.fileTotal,_baseurl.c_str(),_stat.allSize,tm,sp,Average,_stat.file1.c_str(),_stat.file2.c_str(),_stat.MaxInterval);

		
//		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Average interval: %d ms,Maximum interval: %d ms "),);
	}
}

void Download::onError( int error,const char* errorDescription )
{
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Download Error,errorCode[%d],Description:%s,url:%s,file:%s"),error,errorDescription,_baseurl.c_str(),_CurrentDownloadFileName.c_str());
	
	if (!_completed)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"_completed is false,url:%s,file:%s"),_baseurl.c_str(),_CurrentDownloadFileName.c_str());
		if (!_file.empty())
		{
			Download* d = new Download(_Logger,_baseurl,_bitrate,_stat,_file);
			//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"outstr:%s"),str.c_str());
			d->init(get_loop());
			d->dohttp();
		}
		else
		{
			int64 tm = ZQ::common::now() - _stat.allStartTime;
			int64 sp = _stat.allSize * 8/tm;
			int64 Average = _stat.allInterval /(_stat.fileTotal - 1);
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"%d files downloaded to complete,directory:%s,size:%dByte,take:%dms,bitrate:%dkbps,Average interval: %d ms,The maximum time between %s and %s is : %d ms"),_stat.fileTotal,_baseurl.c_str(),_stat.allSize,tm,sp,Average,_stat.file1.c_str(),_stat.file2.c_str(),_stat.MaxInterval);


			//		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Average interval: %d ms,Maximum interval: %d ms "),);
		}
	}
	
	if (error != elpe__EOF)
	{
		//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Download Error,errorCode[%d],Description:%s"),error,errorDescription);
		//printf("Download Error,errorCode[%d],Description:%s\n",error,errorDescription);
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
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"The Session destructor to exit!"));
}

void Session::dohttp(std::string& m3u8url)
{
	_RespBody.str("");
	_baseurl = m3u8url.substr(0,m3u8url.find_last_of("/"));
	std::string fetchm3u8 = m3u8url + "&rate="+ _bitrate;

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	printf("downloading:%s\n",_baseurl.c_str());
	beginRequest(msg,fetchm3u8);
}

void Session::OnConnected(ElpeError status)
{
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session OnConnected,baseurl:%s"),_baseurl.c_str());
	HttpClient::OnConnected(status);
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session start,baseurl:%s"),_baseurl.c_str());
}

void Session::onHttpDataSent()
{
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"send suc."));
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
	std::list<std::string> file;
	std::string outstr;
	while(std::getline(_RespBody,outstr))
	{
		if (outstr.find("#") != outstr.npos)
			continue;
		file.push_back(outstr);
	}
	_RespBody.str("");

	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session onMessageCompleted,baseurl:%s,The total number of files is %d!"),_baseurl.c_str(),file.size());

	printf("download complete:%s\n",_baseurl.c_str());

	if (!file.empty())
	{
		Download::Statistics stat;
		stat.allStartTime = ZQ::common::now();
		stat.fileTotal = file.size();
		Download* d = new Download(_Logger,_baseurl,_bitrate,stat,file);
		d->init(get_loop());
		d->dohttp();
	}
}

void Session::onError( int error,const char* errorDescription )
{
//	if (error != elpe__EOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session Error,baseurl:%s,errorCode[%d],Description:%s"),_baseurl.c_str(),error,errorDescription);
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
