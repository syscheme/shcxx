#include "C2Downloader.h"
#include <TimeUtil.h>

#define  IntervalTime	(1 * 1)	//1ms
namespace LibAsync{

	C2DownLoader::C2DownLoader(ZQ::common::Log& log, ServerCallBack* mgrCB, uint timeout)
		:_log(log), 
		_uri(""), 
		_timeout(timeout),
		_lastStatisticsTime(0), 
		_nextStatisticsTime(0), 
		_lastRecvCount(0),
		_recvCount(0),
		_mgrCB(mgrCB),
		brecv(false),
		bhttpbody(false),
		_status(LibAsync::NotConnected),
		HttpClient(),
		Timer(getLoop())
	{
		for (int i=0; i<8; i++)
		{
			AsyncBuffer buf;
			buf.len = 8* 1024;
			buf.base = (char*)malloc(sizeof(char)* buf.len);
			_recvBufs.push_back(buf);
		}
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] entey constructor, and allocate 8 bufs which size is 8*1024 byte"), this);
	   _sendMsg = new  HttpMessage(HTTP_REQUEST);
	}

	C2DownLoader::~C2DownLoader()
	{
		  cancel();
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] entey destructor"), this);
	}

	bool C2DownLoader::setHeader(const std::string key, const std::string value)
	{
		_sendMsg->header(key, value);
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] set header [%s = %s]"), this, key.c_str(), value.c_str());
		return true;
	}

	bool C2DownLoader::setURI(const std::string uri)
	{
		if (uri.empty())
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] set uri failure, url is empty"), this);
			return false;
		}
		_uri = uri;
		return true;
	}

	bool C2DownLoader::setCommParam(const std::string ip, const unsigned short port)
	{
		if (ip.empty())
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] set communicate parameters failure, ip is empty"), this);
			return false;
		}

		if (port == 0)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] set communicate parameters failure, port is 0"), this);
			return false;
		}

		_ip		= ip;
		_port	= port;
		return true;
	}

	bool C2DownLoader::sendRequest()
	{
		  int64 startSend = ZQ::common::TimeUtil::now();
		_recvCount = 0;
		_mgrData.avgBitrate = 0;
		_mgrData.preBitrate.clear();
		_mgrData.totalRevSize = 0;
		_mgrData.url = "";
		_mgrData.sessId = "";
		_mgrData.clientStat = LibAsync::CLIENT_FAILED;
		char buf[128];
		memset(buf, '\0', 128);
		snprintf(buf, sizeof(buf), "%p", this);
		_mgrData.strAddr = buf;

		if (_uri.empty())
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] send download request failure, url is empty"), this);
			C2DownLoaderPtr pt(this);
			cancel();
			_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
			return false;
		}

		if (_ip.empty())
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] send download request failure, ip is empty"), this);
			C2DownLoaderPtr pt(this);
			cancel();
			_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
			return false;
		}

		if (_port == 0)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ]  send download request failure, port is 0"), this);
			C2DownLoaderPtr pt(this);
			cancel();
			_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
			return false;
		}

		_sendMsg->method(HTTP_GET);
		_sendMsg->url(_uri);
		_sendMsg->keepAlive(true);
		//_sendMsg->chunked(true);

		update(_timeout);
		if (!beginRequest(_sendMsg, _ip, _port))
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "[ client: %p ] send request failure"), this);
			cancel();	//remove timer from iocp
			C2DownLoaderPtr pt(this);
			_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
			return false;
		}

		_mgrData.url = _uri;
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] send request success using time[%d]."), this, (int)(ZQ::common::TimeUtil::now() - startSend));
		return true;
	}

	bool C2DownLoader::onHttpBody(const char* data, size_t size)
	{
		  if ( !bhttpbody )
		  {
				std::string ip = "";
				unsigned short Port = 0;
				Socket::getLocalAddress(ip, Port);
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] onHttpBody() entry with client[%s:%d] status[%s]."), this, ip.c_str(), Port, getTransferStatus().c_str());
				bhttpbody = true;
		  }
		_recvCount += size;
		_mgrData.totalRevSize = _recvCount;
		
		if (_headerMsg->chunked())
		{
			if ((_recvCount / 1024) < 1024)
			{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] receive data size [ %5ldB ], received total data size [ %5ldKB ]"), this, size, _recvCount/1024);
			}
			else{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] receive data size [ %5ldB ], received total data size [ %.03fMB ]"), this, size, (float)_recvCount/(1024 * 1024));
			}
		}
		else{
			int64 totalLen = _headerMsg->contentLength();
			if ((_recvCount / 1024) < 1024)
			{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] Received data size [ %5ldKB / %.03fMB ]"), this, _recvCount/1024, (float)totalLen/(1024 * 1024));
			}
			else{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] Received data size [ %.03fMB / %.03fMB ]"), this, (float)_recvCount/(1024 * 1024), (float)totalLen/(1024 * 1024));
			}
		}

		update(_timeout);
		return true;
	}

	void C2DownLoader::onHttpComplete()
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] onHttpComplete() entry"), this);

		_status = LibAsync::Completed;
		_endTime = ZQ::common::now();
	}

	bool C2DownLoader::onHttpMessage(const HttpMessagePtr msg)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] onHttpMessage() entry status[%s]."), this, getTransferStatus().c_str());
		_headerMsg = msg;
		_mgrData.sessId = _headerMsg->header("X-SessionId");
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] Got session id [%s]"), this, _mgrData.sessId.c_str());

		return true;
	}

	void C2DownLoader::onHttpDataReceived( size_t size )
	{
		 if ( !brecv )
		 {
			   std::string ip = "";
			   unsigned short Port = 0;
			   Socket::getLocalAddress(ip, Port);
			   _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] onHttpDataReceived() entry get size [%d]  with client[%s:%d] status[%s]."), this, size, ip.c_str(), Port, getTransferStatus().c_str());
			   brecv = true;
		 }
		int64 timestamp = ZQ::common::TimeUtil::now();
		if (timestamp >= _nextStatisticsTime)
		{
			//calculate instantaneous bitrate
			int bitrate = (_recvCount - _lastRecvCount) / ((float)(timestamp - _lastStatisticsTime)/1000);
			int64 samplingTime = (timestamp + _lastStatisticsTime)/2;
			_mgrData.preBitrate.insert(std::make_pair(samplingTime, bitrate * 8));

			_lastStatisticsTime = timestamp;
			_nextStatisticsTime = _lastStatisticsTime + IntervalTime;
			_lastRecvCount = _recvCount;
		}

		if(LibAsync::Completed == _status)
		{
			//calculate average bitrate
			int64 timestamp = ZQ::common::TimeUtil::now();
			int64 useTime = timestamp - _startTime;
			//int avgBitrate = (_recvCount * 8)/((float)(timestamp - _startTime)/1000);
			_mgrData.avgBitrate = getAvgBitrate();//avgBitrate;
			_mgrData.clientStat = CLIENT_SUCCESS;
			std::string ip = "";
			unsigned short Port = 0;
			Socket::getLocalAddress(ip, Port);

			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] Got all data with client[%s:%d] status[%s]."), this, ip.c_str(), Port, getTransferStatus().c_str());

			cancel();	//remove timer from iocp
			C2DownLoaderPtr pt(this);
			_mgrCB->onDateReady(pt, _mgrData);
			_mgrCB->onClientStop(pt, LibAsync::CLIENT_SUCCESS);
		}
		else{
			update(_timeout);
			recvRespBody(_recvBufs);
		}
	}

	void C2DownLoader::onReqMsgSent(size_t size)
	{
		  std::string ip = "";
		  unsigned short Port = 0;
		  Socket::getLocalAddress(ip, Port);
		  _status	= LibAsync::SendCompleted;
		  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(C2DownLoader, "[ client: %p ] onReqMsgSent [size:%d] with client[%s:%d] status[%s]."), this,  size, ip.c_str(), Port, getTransferStatus().c_str());

		_startTime			= ZQ::common::TimeUtil::now();
		_lastStatisticsTime = _startTime;
		_nextStatisticsTime = _lastStatisticsTime + IntervalTime;
		update(_timeout);
		_endTime         = _startTime;
		getResponse();
	}

	void C2DownLoader::onTimer()
	{
		std::string ip = "";
		unsigned short Port = 0;
		Socket::getLocalAddress(ip, Port);
		// _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onTimer() entry, [ client : %p with client [%s:%d] get response timeout"), this, ip.c_str(), Port);
		if (_status >= LibAsync::Connected)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onTimer() entry, [ client : %p ]  with client [%s:%d] get response timeout status[%s]."), this, ip.c_str(), Port, getTransferStatus().c_str());
		}
		else{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onTimer() entry, [ client : %p ] with client [%s:%d] connect or send request timeout status[%s]."), this, ip.c_str(), Port, getTransferStatus().c_str());
		}
		if (LibAsync::Failure == _status)
		{
			  _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onTimer() [ client : %p ] [session : %s] status[%s] onClientStop() has already been called, so return immediately ."), this,  _mgrData.sessId.c_str() , getTransferStatus().c_str());
			  return;
		}
		_status = LibAsync::Failure;
		C2DownLoaderPtr pt(this);
		_mgrData.avgBitrate = getAvgBitrate();
		_mgrData.clientStat = LibAsync::CLIENT_FAILED;
		_mgrCB->onDateReady(pt, _mgrData);
	    cancel();
		_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
	}

	void C2DownLoader::onHttpError( int error )
	{
		if (LibAsync::Failure == _status)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onHttpError() entry, [ client : %p ] [error : %d] [session : %s] status[%s] onClientStop() has already been called, so return immediately ."), this, error, _mgrData.sessId.c_str() , getTransferStatus().c_str());
			return;
		}
		_status = LibAsync::Failure;
		  std::string ip = "";
		  unsigned short Port = 0;
		  Socket::getLocalAddress(ip, Port);
		  _log(ZQ::common::Log::L_ERROR, CLOGFMT(C2DownLoader, "onHttpError() entry, [ client : %p ] [error : %d] [session : %s] with client[%s:%d] status[%s] "), this, error, _mgrData.sessId.c_str(), ip.c_str(), Port , getTransferStatus().c_str());
		cancel();	//remove timer from iocp
		C2DownLoaderPtr pt(this);
		_mgrData.avgBitrate = getAvgBitrate();
		_mgrCB->onDateReady(pt, _mgrData);
		_mgrCB->onClientStop(pt, LibAsync::CLIENT_FAILED);
	}

	int C2DownLoader::getAvgBitrate()
	{
		if (_endTime <= _startTime )
		{
			  int64 timestamp = ZQ::common::TimeUtil::now();
			  _startTime = timestamp;
		}
		int avgBitrate = (_recvCount * 8)/((float)(_endTime - _startTime)/1000);

		return avgBitrate;
	}

	std::string C2DownLoader::getTransferStatus()
	{
		  if (LibAsync::NotConnected == _status )
		  {
				return "NotConnected";
		  }
		  else  if (LibAsync::Connected == _status )
		  {
				return "Connected";
		  }
		  else  if (LibAsync::SendCompleted == _status )
		  {
				return "SendCompleted";
		  }
		  else  if (LibAsync::RecvCompleted == _status )
		  {
				  return "RecvCompleted";
		  }
		  else  if (LibAsync::Completed == _status )
		  {
				  return "Completed";
		  }
		  else  if (LibAsync::Failure == _status )
		  {
				return "Failure";
		  }
		  else 
		  {
				return "Unknown";
		  }
	}

}
