#ifndef __DOWNLOAD_h__
#define __DOWNLOAD_h__

#include "../Http/LibHttp/HttpClient.h"
#include <NativeThread.h>
#include <list>
#include <string>

namespace ZQ {
	namespace eloop {


// ---------------------------------------
// class Download
// ---------------------------------------

class Download:public HttpClient
{
public:
	Download(ZQ::common::Log& logger,std::string baseurl,std::string bitrate,std::list<std::string> file);
	~Download();
	void dohttp();

	virtual void	onHttpDataSent();

	virtual void	onHttpDataReceived( size_t size );

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	int64				_startTime;
	int64				_totalTime;			//ms
	int64				_totalSize;
	std::string	_CurrentDownloadFileName;
	std::string			_baseurl;
	std::string			_bitrate;
	std::list<std::string>  _file;

};

// ---------------------------------------
// class Session
// ---------------------------------------
class Session:public HttpClient
{
public:
	Session(ZQ::common::Log& logger,std::string bitrate);
	~Session();

	void dohttp(std::string& m3u8url);

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
};

// ---------------------------------------
// class DownloadThread
// ---------------------------------------
class DownloadThread:public ZQ::common::NativeThread
{
public:
	typedef std::list<std::string> M3u8List;
public:
	DownloadThread(ZQ::common::Log& logger,M3u8List m3u8list,std::string& bitrate,int interval,int loopcount);
	~DownloadThread();

	virtual int run(void);


private:
	ZQ::common::Log&	_Logger;
	int					_IntervalTime;
	int					_loopCount;
	M3u8List			_m3u8list;
	std::string&		_bitrate;
};

} }//namespace ZQ::eloop

#endif
