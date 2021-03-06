#include "Download.h"
#include <urlstr.h>
#include <TimeUtil.h>
namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class Download
// ---------------------------------------
Download::Download(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,Statistics stat,std::list<std::string> file,Download::ObjServer objserver,LoopRateMonitor& loopRate,int errorcount)
	:HttpClient(logger),
	_Logger(logger),
	_baseurl(baseurl),
	_bitrate(bitrate),
	_stat(stat),
	_totalSize(0),
	_onBodyTime(0),
	_objServer(objserver),
	_completed(false),
	_errorCount(errorcount),
	_interval(0),
	_file(file),
	_ReqTime(0),
	_newReqTime(ZQ::common::now()),
	_loopRate(loopRate)
{
}
Download::~Download()
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"The Download destructor to exit!"));
}

void Download::dohttp()
{
	_CurrentDownloadFileName = _file.front();
	std::string url = _baseurl + "/" + _CurrentDownloadFileName;
	if(_objServer == EdgeFE)
	{
		url = url + "&rate="+ _bitrate;
	}
	 
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");
	beginRequest(msg,url);
	_connTime = ZQ::common::now();
	_interval = ZQ::common::now();
	_newReqTime = ZQ::common::now() - _newReqTime;
}

void Download::OnConnected(ElpeError status)
{
	HttpClient::OnConnected(status);
	_ReqTime = ZQ::common::now();
	_startTime = ZQ::common::now();
	std::string url = _baseurl + "/" + _CurrentDownloadFileName;
	if(_objServer == EdgeFE)
		url = url + "&rate=" + _bitrate;
	_connTime = ZQ::common::now() - _connTime;
	_firstDataTime = ZQ::common::now();
	_onBodyTime = ZQ::common::now();

	_recvCount = 0;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"downloading:%s"),url.c_str());
	printf("downloading:%s\n",url.c_str());

}

void Download::onHttpDataSent()
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"send suc."));
	_ReqTime = ZQ::common::now() - _ReqTime;
}

void Download::onHttpDataReceived( size_t size )
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"recv data size = %d."),size);
	_recvCount++;
}


bool Download::onHeadersEnd( const HttpMessage::Ptr msg)
{
	if (_stat.CompletionTime != 0)
	{
		_interval = ZQ::common::now() -_stat.CompletionTime;
		_stat.allInterval += _interval;
		if (_stat.MaxInterval < _interval)
		{
			_stat.MaxInterval = _interval;
			_stat.file1 = _stat.prevFile;
			_stat.file2 = _CurrentDownloadFileName;
		}	
	}
	else
		_interval = ZQ::common::now() - _interval;
	_stat.prevFile = _CurrentDownloadFileName;
	_totalSize = 0;
	_firstDataTime = ZQ::common::now() - _firstDataTime;
	return true;
}

bool Download::onBodyData( const char* data, size_t size)
{
	_totalSize += size;
//	_onBodyTime = ZQ::common::now() - _onBodyTime;
	std::string url = _baseurl + "/" + _CurrentDownloadFileName;
//	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"%s onBodyData len[%d],took:%dms"),url.c_str(),size,_onBodyTime);
//	_onBodyTime = ZQ::common::now();
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

	if (_stat.MaxConnTime < _connTime)
	{
		_stat.MaxConnTime = _connTime;
		_stat.MaxConnFile = _CurrentDownloadFileName;
	}
	

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	int64 onceRecv = _totalSize/_recvCount;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"download complete:%s,host:%s:%d,loopRate:%lld/s,interval:%lldms,newReqTime:%lldms,ConnTakeTime:%lldms,ReqTime:%lldms,firstDataTime:%lldms,_recvCount:%lld,onceRecv:%lldByte,size:%lldByte,take:%lldms,bitrate:%lldkbps"),url.c_str(),locip,locport,_loopRate.getRate(),_interval,_newReqTime,_connTime,_ReqTime,_firstDataTime,_recvCount,onceRecv,_totalSize,totalTime,speed);
	printf("download complete:%s,host:%s:%d,loopRate:%lld/s,interval:%lldms,newReqTime:%lldms,ConnTakeTime:%lldms,firstDataTime:%lldms,_recvCount:%lld,onceRecv:%lldByte,size:%lldByte,take:%lldms,bitrate:%lldkbps\n",url.c_str(),locip,locport,_loopRate.getRate(),_interval,_newReqTime,_connTime,_firstDataTime,_recvCount,onceRecv,_totalSize,totalTime,speed);

// 	char peerip[17] = { 0 };
// 	int  peerport = 0;
// 	getpeerIpPort(peerip,peerport);
	_file.pop_front();
	if (!_file.empty())
	{
		Download* d = new Download(_Logger,_baseurl,_bitrate,_stat,_file,_objServer,_loopRate);
		//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"outstr:%s"),str.c_str());
		d->init(get_loop());
		d->dohttp();
	}
	else
	{
		int64 tm = ZQ::common::now() - _stat.allStartTime;
		int64 sp = _stat.allSize * 8/tm;
		int64 Average = _stat.allInterval /(_stat.fileTotal - 1);
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"%d files downloaded to complete,directory:%s,size:%lldByte,take:%lldms,bitrate:%lldkbps,Average interval: %lld ms,The maximum time between %s and %s is : %lld ms,MaxConnFile:%s,MaxConnTime:%lldms,loopAverageRate:%lld/s"),
			_stat.fileTotal,_baseurl.c_str(),_stat.allSize,tm,sp,Average,_stat.file1.c_str(),_stat.file2.c_str(),_stat.MaxInterval,_stat.MaxConnFile.c_str(),_stat.MaxConnTime,_loopRate.getAverageRate());
		_loopRate.stopAt();
		_loopRate.closeAt();
	}
}

void Download::onError( int error,const char* errorDescription )
{
	if (error == 0)
	{
		_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(onError,"elpeSuccess!"));
		return;
	}

//	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Download Error,errorCode[%d],Description:%s,url:%s,file:%s"),error,errorDescription,_baseurl.c_str(),_CurrentDownloadFileName.c_str());


	if (!_completed)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"_completed is false,url:%s,file:%s"),_baseurl.c_str(),_CurrentDownloadFileName.c_str());
/*		if (_errorCount >= 5)
		{
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Continuous request failed,url:%s,file:%s"),_baseurl.c_str(),_CurrentDownloadFileName.c_str());
			shutdown();
			return;
		}*/
		if (!_file.empty())
		{
			_errorCount++;
			Download* d = new Download(_Logger,_baseurl,_bitrate,_stat,_file,_objServer,_loopRate,_errorCount);
			//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"outstr:%s"),str.c_str());
			d->init(get_loop());
			d->dohttp();
		}
		else
		{
			int64 tm = ZQ::common::now() - _stat.allStartTime;
			int64 sp = _stat.allSize * 8/tm;
			int64 Average = _stat.allInterval /(_stat.fileTotal - 1);
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"%lld files downloaded to complete,directory:%s,size:%lldByte,take:%lldms,bitrate:%lldkbps,Average interval: %lld ms,The maximum time between %s and %s is : %lld ms"),
				_stat.fileTotal,_baseurl.c_str(),_stat.allSize,tm,sp,Average,_stat.file1.c_str(),_stat.file2.c_str(),_stat.MaxInterval);

			//		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Average interval: %d ms,Maximum interval: %d ms "),);
		}
	}

	if (error != elpuEOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Download,"Download Error,errorCode[%d],Description:%s"),error,errorDescription);
		printf("Download Error,errorCode[%d],Description:%s\n",error,errorDescription);
	}
	shutdown();
}

//-------------------------------------
// clasee Transmission
//--------------------------------------
Transmission::Transmission(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,Download::Statistics stat,std::list<std::string> file,int64 limit,Download::ObjServer objserver)
:_startTime(0),
_Logger(logger),
_bitrate(bitrate),
_baseurl(baseurl),
_stat(stat),
_file(file),
_limit(limit),
_objserver(objserver),
_errorCount(0),
_completed(false)
{

}

Transmission::~Transmission()
{
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Transmission,"The Transmission destructor to exit!"));
}

void Transmission::HeadersEnd(std::string currentFile)
{
	if (_stat.CompletionTime != 0)
	{
		int64 interval = ZQ::common::now() -_stat.CompletionTime;
		_stat.allInterval += interval;
		if (_stat.MaxInterval < interval)
		{
			_stat.MaxInterval = interval;
			_stat.file1 = _stat.prevFile;
			_stat.file2 = currentFile;
		}	
	}
	_stat.prevFile = currentFile;
}

void Transmission::MessageCompleted(int64 totalSize)
{
	_stat.allSize += totalSize;
	_stat.CompletionTime = ZQ::common::now();
	_completed = true;
	_errorCount = 0;
	_file.pop_front();

	//reset timer;
	_total += totalSize;
	int64 ideal = _total*8*1000 / _limit;			//ms
	int64 realTime = ZQ::common::now() - _startTime;
	int64 sl = ideal - realTime;
	if (sl > 0)
		start(sl,-1);//set_repeat(sl);
	else
		start(0,-1);//set_repeat(0);
}

void Transmission::DownloadError(const std::string& currentFile)
{
	if (!_completed)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Transmission,"_completed is false,url:%s,file:%s"),_baseurl.c_str(),currentFile.c_str());
		if (_errorCount >= 5)
		{
			close();
			return;
		}
		_errorCount++;
		start(0,-1);
	}
}

void Transmission::OnClose()
{
	delete this;
}

void Transmission::OnTimer()
{
	if (_startTime == 0)
	{
		_startTime = ZQ::common::now();
	}
	if (!_file.empty())
	{
		_completed = false;
		ControlDownload* d = new ControlDownload(_Logger,*this);
		d->init(get_loop());
		d->dohttp(_file.front());
		//set_repeat(-1);
		//stop();
	}
	else
	{
		int64 tm = ZQ::common::now() - _stat.allStartTime;
		int64 sp = _stat.allSize * 8/tm;
		int64 Average = _stat.allInterval /(_stat.fileTotal - 1);
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Transmission,"%lld files downloaded to complete,directory:%s,size:%lldByte,take:%lldms,bitrate:%lldkbps,Average interval: %lld ms,The maximum time between %s and %s is : %lld ms"),_stat.fileTotal,_baseurl.c_str(),_stat.allSize,tm,sp,Average,_stat.file1.c_str(),_stat.file2.c_str(),_stat.MaxInterval);
		close();
	}
}

const std::string& Transmission::getBitrate() const
{
	return _bitrate;
}
const std::string& Transmission::getBaseurl() const
{
	return _baseurl;
}
const Download::ObjServer& Transmission::getObjServer() const
{
	return _objserver;
}

//-------------------------------------
// clasee ControlDownload
//-------------------------------------
ControlDownload::ControlDownload(ZQ::common::Log& logger,Transmission& trans)
	:HttpClient(logger),
	_Logger(logger),
	_trans(trans)
{

}

ControlDownload::~ControlDownload()
{

}

void ControlDownload::dohttp(const std::string& filename)
{
	_filename = filename;
	std::string url = _trans.getBaseurl() + "/" + filename;
	if (_trans.getObjServer() == Download::EdgeFE)
	{
		url = url + "&rate="+ _trans.getBitrate();
	}

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	beginRequest(msg,url);
}

void ControlDownload::OnConnected(ElpeError status)
{
	HttpClient::OnConnected(status);
	_startTime = ZQ::common::now();
	std::string url = _trans.getBaseurl() + "/" + _filename;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ControlDownload,"downloading:%s"),url.c_str());
	printf("downloading:%s\n",url.c_str());
}

void ControlDownload::onHttpDataSent()
{

}

void ControlDownload::onHttpDataReceived( size_t size )
{

}

bool ControlDownload::onHeadersEnd( const HttpMessage::Ptr msg)
{
	_trans.HeadersEnd(_filename);
	_totalSize = 0;
	return true;
}

bool ControlDownload::onBodyData( const char* data, size_t size)
{
	_totalSize += size;
	return true;
}

void ControlDownload::onMessageCompleted()
{
	_trans.MessageCompleted(_totalSize);

	int64 totalTime = ZQ::common::now() - _startTime; 

	int64 speed = _totalSize * 8/totalTime;
	std::string url = _trans.getBaseurl() + "/" + _filename;
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ControlDownload,"download complete:%s,size:%lldByte,take:%lldms,bitrate:%lldkbps"),url.c_str(),_totalSize,totalTime,speed);
	printf("download complete:%s,size:%lldByte,take:%lldms,bitrate:%lldkbps\n",url.c_str(),_totalSize,totalTime,speed);

}

void ControlDownload::onError( int error,const char* errorDescription )
{
	if (error == 0)
	{
		_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(onError,"elpeSuccess!"));
		return;
	}

	if (error != elpuEOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ControlDownload,"Download Error,errorCode[%d],Description:%s"),error,errorDescription);
		printf("Download Error,errorCode[%d],Description:%s\n",error,errorDescription);
		_trans.DownloadError(_filename);
	}
	shutdown();
}

// ---------------------------------------
// class Session
// ---------------------------------------
Session::Session(ZQ::common::Log& logger,std::string bitrate,int64 limit,Download::ObjServer objserver,LoopRateMonitor& loopRate)
	:HttpClient(logger),
	_Logger(logger),
	_bitrate(bitrate),
	_limit(limit),
	_objServer(objserver),
	_loopRate(loopRate)
{

}
Session::~Session()
{
	//_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"The Session destructor to exit!"));
}

void Session::dohttp(std::string& m3u8url)
{
	_RespBody.str("");
	_baseurl = m3u8url.substr(0,m3u8url.find_last_of("/"));	
	std::string fetchm3u8 = m3u8url;
	if (_objServer == Download::EdgeFE)
	{
		fetchm3u8 = fetchm3u8 + "&rate="+ _bitrate; 
	}
	
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_REQUEST);
	msg->method(HttpMessage::GET);
	msg->url("*");

	printf("downloading:%s\n",fetchm3u8.c_str());
	beginRequest(msg,fetchm3u8);
}

void Session::OnConnected(ElpeError status)
{
//	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session OnConnected,baseurl:%s"),_baseurl.c_str());
	HttpClient::OnConnected(status);
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session start,baseurl:%s"),_baseurl.c_str());
}

void Session::onHttpDataSent()
{
//	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"send suc."));
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
		if (_limit > 0)
		{
			Transmission * t = new Transmission(_Logger,_baseurl,_bitrate,stat,file,_limit,_objServer);
			t->init(get_loop());
			t->start(0,-1);
		}
		else
		{
			Download* d = new Download(_Logger,_baseurl,_bitrate,stat,file,_objServer,_loopRate);
			d->init(get_loop());
			d->dohttp();
		}
	}
}

void Session::onError( int error,const char* errorDescription )
{
	if (error != elpuEOF)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(Session,"Session Error,baseurl:%s,errorCode[%d],Description:%s"),_baseurl.c_str(),error,errorDescription);
		printf("Session Error,errorCode[%d],Description:%s\n",error,errorDescription);
	}
	shutdown();
}

// ---------------------------------------
// class DownloadThread
// ---------------------------------------
DownloadThread::DownloadThread(ZQ::common::Log& logger,M3u8List m3u8list,std::string& bitrate,int interval,int loopcount,int64 limit,Download::ObjServer objserver)
	:_Logger(logger),
	_m3u8list(m3u8list),
	_bitrate(bitrate),
	_IntervalTime(interval),
	_loopCount(loopcount),
	_limit(limit),
	_objServer(objserver)
{

}

DownloadThread::~DownloadThread()
{

}

int DownloadThread::run(void)
{
	Loop loop(false);
	_loopRateMonitor.init(loop);
	_loopRateMonitor.startAt();
	M3u8List::iterator it;
	std::string m3u8;
	while(!_m3u8list.empty())
	{
		m3u8 = _m3u8list.front();
		for(int i=0;i< _loopCount;i++)
		{
			ZQ::eloop::Session* s = new ZQ::eloop::Session(_Logger,_bitrate,_limit,_objServer,_loopRateMonitor);
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
