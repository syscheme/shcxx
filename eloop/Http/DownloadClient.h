#ifndef __DOWNLOAD_CLIENT_h__
#define __DOWNLOAD_CLIENT_h__

#include "httpClient.h"
#include <NativeThread.h>

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class DownloadClient
// ---------------------------------------
class DownloadClient:public HttpClient
{
public:
	DownloadClient(ZQ::common::Log& logger);
	~DownloadClient();
	
	void dohttp(std::string& url);
	void closefile();

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);

	virtual bool	onHttpBody( const char* data, size_t size);

	virtual void	onHttpComplete();

	virtual void	onHttpError( int error );

private:
	ZQ::common::Log&	_Logger;
	HttpMessage::Ptr	_Response;
	std::string			_RespBody;
	FILE*				_fp;
};


// ---------------------------------------
// class EmptyClient
// ---------------------------------------
class DownloadThread;
class EmptyClient:public HttpClient
{
public:
	EmptyClient(ZQ::common::Log& logger,DownloadThread* thread);
	~EmptyClient();

	void dohttp(std::string& url);

	virtual void OnConnected(ElpeError status);

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);

	virtual bool	onHttpBody( const char* data, size_t size);

	virtual void	onHttpComplete();

	virtual void	onHttpError( int error );

private:
	ZQ::common::Log&	_Logger;
	HttpMessage::Ptr	_Response;
	std::string			_RespBody;
	DownloadThread*		_Thread;
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

	void subtract();
	void add();
	int getCount();


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
