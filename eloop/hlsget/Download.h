#ifndef __DOWNLOAD_h__
#define __DOWNLOAD_h__

#include "../Http/LibHttp/HttpClient.h"
#include "TimeUtil.h"
#include <NativeThread.h>
#include <list>
#include <string>

namespace ZQ {
	namespace eloop {


//--------------------------------------------------
//class LoopRateMonitor
//--------------------------------------------------
class LoopRateMonitor:public ZQ::eloop::Idle
{
public:
	LoopRateMonitor()
		:_time(ZQ::common::now()),
		_startTime(ZQ::common::now()),
		_total(0),
		_count(0),
		_rate(0)
	{

	}
	~LoopRateMonitor() {}
	virtual void OnIdle()
	{
		_total++;
		if ((ZQ::common::now() - _time) >= 1000)
		{
			_time = ZQ::common::now();
			_rate = _count;
			_count = 0;
		}
		else
			_count++;
	}
	int64 getRate()
	{
//		 _rate = _count*1000/(ZQ::common::now()-_time);
		return _rate;
	}

	int64 getAverageRate()
	{
		return (_total*1000/(ZQ::common::now() - _startTime));
	}
private:
	int64	_startTime;
	int64   _total;
	int64	_time;
	int64	_count;
	int64	_rate;
};

// ---------------------------------------
// class Download
// ---------------------------------------

class Download:public HttpClient
{
public:
	typedef struct statistics
	{
		statistics() {
			CompletionTime = 0;
			allStartTime = 0;
			allSize = 0;
			allInterval = 0;
			MaxInterval = 0;
			fileTotal = 0;
			prevFile = "";
			file1 = "";
			file2 = "";
		}
		int64				CompletionTime;
		int64				allStartTime;
		int64				allSize;
		int64				allInterval;
		int64				MaxInterval;
		int64				fileTotal;
		std::string			prevFile;
		std::string			file1;
		std::string			file2;
	}Statistics;

	typedef enum{
		EdgeFE	= 0,
		Aqua	= 1,
		Def		= EdgeFE
	}ObjServer;
public:
	Download(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,Statistics stat,std::list<std::string> file,Download::ObjServer objserver,LoopRateMonitor& loopRate,int errorcount = 0);
	~Download();
	void dohttp();
	virtual void OnConnected(ElpeError status);

	virtual void	onHttpDataSent();

	virtual void	onHttpDataReceived( size_t size );

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	int64				_startTime;			//ms
	int64				_connTime;
	int64				_interval;
	int64				_recvCount;
	int64				_firstDataTime;
	int64				_onBodyTime;
	int64				_totalSize;
	Statistics			_stat;
	std::string	_CurrentDownloadFileName;
	std::string			_baseurl;
	std::string			_bitrate;
	std::list<std::string>  _file;
	Download::ObjServer _objServer;
	bool				_completed;
	int					_errorCount;
	LoopRateMonitor&	_loopRate;
};

//--------------------------------------
// clasee Transmission
//--------------------------------------
class Transmission:public Timer
{
public:
	Transmission(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,Download::Statistics stat,std::list<std::string> file,int64 limit,Download::ObjServer objserver);
	~Transmission();

	virtual void OnTimer();
	virtual void OnClose();

	void HeadersEnd(std::string currentFile);
	void MessageCompleted(int64 totalSize);
	void DownloadError(const std::string& currentFile);

	const std::string& getBitrate() const;
	const std::string& getBaseurl() const;
	const Download::ObjServer& getObjServer() const;


private:
	ZQ::common::Log&		_Logger;
	std::list<std::string>  _file;
	int64					_total;
	std::string				_bitrate;
	std::string				_baseurl;
	Download::Statistics	_stat;
	int64					_startTime;			//ms
	bool					_completed;
	int64					_limit;
	Download::ObjServer		_objserver;
	int						_errorCount;
};

//--------------------------------------
// clasee ControlDownload
//--------------------------------------
class ControlDownload:public HttpClient
{
public:
	ControlDownload(ZQ::common::Log& logger,Transmission& trans);
	~ControlDownload();

	void dohttp(const std::string& filename);
	virtual void OnConnected(ElpeError status);

	virtual void	onHttpDataSent();

	virtual void	onHttpDataReceived( size_t size );

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&		_Logger;
	Transmission&			_trans;
	int64					_startTime;			//ms
	int64					_totalSize;
	std::string				_filename;
};

// ---------------------------------------
// class Session
// ---------------------------------------
class Session:public HttpClient
{
public:
	Session(ZQ::common::Log& logger,std::string bitrate,int64 limit,Download::ObjServer objserver,LoopRateMonitor& loopRate);
	~Session();

	void dohttp(std::string& m3u8url);
	virtual void OnConnected(ElpeError status);

	virtual void	onHttpDataSent();

	virtual void	onHttpDataReceived( size_t size );

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	std::stringstream	_RespBody;
	std::string			_baseurl;
	std::string			_m3u8file;
	std::string			_bitrate;
	int64				_limit;
	Download::ObjServer	_objServer;
	LoopRateMonitor&	_loopRate;
};

// ---------------------------------------
// class DownloadThread
// ---------------------------------------
class DownloadThread:public ZQ::common::NativeThread
{
public:
	typedef std::list<std::string> M3u8List;
public:
	DownloadThread(ZQ::common::Log& logger,M3u8List m3u8list,std::string& bitrate,int interval,int loopcount,int64 limit,Download::ObjServer objserver);
	~DownloadThread();

	virtual int run(void);


private:
	ZQ::common::Log&	_Logger;
	int					_IntervalTime;
	int					_loopCount;
	M3u8List			_m3u8list;
	std::string&		_bitrate;
	int64				_limit;
	Download::ObjServer	_objServer;
	LoopRateMonitor		_loopRateMonitor;
};

} }//namespace ZQ::eloop

#endif
