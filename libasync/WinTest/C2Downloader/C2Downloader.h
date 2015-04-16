#ifndef _C2_DOWNLOADER_H
#define _C2_DOWNLOADER_H
#include <http.h>
#include <Log.h>
#include  <Pointer.h>

namespace LibAsync{
	class C2DownLoader;
	typedef ZQ::common::Pointer<C2DownLoader>   C2DownLoaderPtr;

	typedef enum _TransferStatus{
		NotConnected,
		Connected,
		SendCompleted,
		RecvCompleted,
		Completed,
		Failure
	}TransferStatus;

	typedef std::map<int64, int > BITRATE;
	typedef enum  _clientStatus{
		  CLIENT_SUCCESS    = 0,
		  CLIENT_FAILED     = 1,
		  CLIENT_RUNING     = 2
	}CLIENTSTATUS;

	typedef struct _httpData{
		  _httpData()
		  {
				avgBitrate = 0;
				preBitrate.clear();
				url = "";
				totalRevSize = 0;
				sessId = "";
		  }
		  std::string    strAddr;
		  std::string    url;
		  int               totalRevSize;
		  std::string    sessId;
		  int	             avgBitrate;
		  BITRATE      preBitrate;
		  CLIENTSTATUS    clientStat;
	}HTTPDATA;

	class ServerCallBack{
	public:
		  virtual void onDateReady(LibAsync::C2DownLoaderPtr& pt, const HTTPDATA&  data) = 0;
		  virtual  void onClientStop(LibAsync::C2DownLoaderPtr& pt, CLIENTSTATUS stat = CLIENT_SUCCESS) = 0;
	};

	class ZQ_COMMON_API C2DownLoader : public HttpClient, public Timer
	{
	public:
		C2DownLoader(ZQ::common::Log& log, ServerCallBack* mgrCB, uint timeout = 30*1000);
		virtual ~C2DownLoader();

		bool setURI(const std::string uri);
		bool setCommParam(const std::string ip, const unsigned short port);
		bool setHeader(const std::string key, const std::string value);

		bool sendRequest();
	private:
		int getAvgBitrate();
		std::string getTransferStatus();
	private:
		virtual void	onReqMsgSent( size_t size);

		// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataReceived( size_t size );

		virtual bool	onHttpMessage( const HttpMessagePtr msg);

		virtual bool	onHttpBody( const char* data, size_t size);

		virtual void	onHttpComplete();

		virtual void	onHttpError( int error );

		virtual void	onTimer();

	private:
		ZQ::common::Log&	_log;
		HttpMessagePtr		_headerMsg;
		HttpMessagePtr		_sendMsg;

		AsyncBufferS		_recvBufs;
		int64				_startTime;
		int64            _endTime;

		std::string			_uri;
		std::string			_ip;
		unsigned short		_port;
		uint				_timeout;

		//for statistical data
		int64				_lastStatisticsTime;
		int64				_nextStatisticsTime;
		int64				_recvCount;
		int64				_lastRecvCount;

		//manager data
		ServerCallBack*		_mgrCB;
		HTTPDATA			_mgrData;

		TransferStatus		_status;

		bool              brecv;
		bool              bhttpbody;
	};
}
#endif
