#ifndef __DOWNLOAD_h__
#define __DOWNLOAD_h__

#include "HttpClient.h"
#include <NativeThread.h>
#include <vector>
#include <string>

namespace ZQ {
	namespace eloop {

/*
// ---------------------------------------
// class fetchm3u8
// ---------------------------------------
class fetchm3u8:public HttpClient
{
public:
	fetchm3u8(ZQ::common::Log& logger);
	~fetchm3u8();

	void dohttp(std::string& url);

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onParseError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	HttpMessage::Ptr	_Response;
	std::string			_RespBody;
	std::vector<std::string> _m3u8file;
};
*/
// ---------------------------------------
// class DownloadClient
// ---------------------------------------
class Download:public HttpClient
{
public:
	Download(ZQ::common::Log& logger);
	~Download();

	void dohttp(std::string& url);

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);

	virtual bool	onBodyData( const char* data, size_t size);

	virtual void	onMessageCompleted();

	virtual void	onParseError( int error,const char* errorDescription );

private:
	ZQ::common::Log&	_Logger;
	HttpMessage::Ptr	_Response;
	std::string			_RespBody;
};


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