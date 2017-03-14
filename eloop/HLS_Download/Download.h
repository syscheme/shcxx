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
	Download(ZQ::common::Log& logger);
	~Download();
	void dohttp(std::string& url,std::string filenaem);
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
};

// ---------------------------------------
// class fetchm3u8
// ---------------------------------------
class fetchm3u8:public HttpClient
{
public:
	fetchm3u8(ZQ::common::Log& logger);
	~fetchm3u8();

	void getm3u8(std::string& m3u8url);
//	int  DownloadCurrentFile();

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
//	bool				_fetchm3u8Completed;
	HttpMessage::Ptr	_Response;
	std::stringstream	_RespBody;
//	std::list<std::string> _file;
	std::string			_baseurl;
};

// ---------------------------------------
// class Session
// ---------------------------------------
/*
class Session
{
	Session();
	~Session();
};
*/

// ---------------------------------------
// class DownloadThread
// ---------------------------------------
class DownloadThread:public ZQ::common::NativeThread
{
public:
	DownloadThread(std::string& url,ZQ::common::Log& logger,int count,int interval,int session);
	~DownloadThread();

	virtual int run(void);


private:
	ZQ::common::Log&	_Logger;
	int					_Count;
	int					_ConnectCount;
	ZQ::common::Mutex	_LockerCount;
	int					_IntervalTime;
	int					_SessionCount;
	std::string&		_url;
};

} }//namespace ZQ::eloop

#endif
