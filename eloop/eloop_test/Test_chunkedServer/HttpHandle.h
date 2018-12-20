#ifndef __TEST_HTTP_SERVER__h__
#define __TEST_HTTP_SERVER__h__

#include "HttpServer.h"

namespace ZQ {
	namespace eloop {

// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
class TestHttpHandle : public HttpHandler
{
public:
	typedef HttpApplication<TestHttpHandle> TestHttpHandleFactory;

	TestHttpHandle(HttpPassiveConn& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~TestHttpHandle();

	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onError( int error,const char* errorDescription );

	virtual void	onHttpDataSent(size_t size);
	virtual void	onHttpDataReceived( size_t size );

};

// ---------------------------------------
// class DownloadeHandle
// ---------------------------------------
class DownloadeHandle : public HttpHandler
{
public:
	typedef HttpApplication<DownloadeHandle> DownloadeHandleFactory;

	DownloadeHandle(HttpPassiveConn& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~DownloadeHandle();

	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onError( int error,const char* errorDescription );

	virtual void	onHttpDataSent(size_t size);
	virtual void	onHttpDataReceived( size_t size );

private:
	void close();

private:
	FILE*				_fp;
	size_t				_dataSize;
	char*				_buf;
	std::string			_workdir;
	HttpMessage::Ptr	_msg;
	bool				_isClose;

};

} }//namespace ZQ::eloop
#endif