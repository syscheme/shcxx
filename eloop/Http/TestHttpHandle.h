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

	TestHttpHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~TestHttpHandle();

	void Response();
	void ResponseIndex();

 	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
 	virtual bool onBodyData( const char* data, size_t size);
 	virtual void onMessageCompleted();
 	virtual void onParseError( int error,const char* errorDescription );

	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );

};

// ---------------------------------------
// class DownloadeHandle
// ---------------------------------------
class DownloadeHandle : public HttpHandler
{
public:
	typedef HttpApplication<DownloadeHandle> DownloadeHandleFactory;

	DownloadeHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~DownloadeHandle();

	void Response();
	void ResponseIndex();

	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onParseError( int error,const char* errorDescription );

	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );

private:
	void close();

private:
	FILE*				_fp;
	size_t				_dataSize;
	char*				_buf;

};


// ---------------------------------------
// class UtilHandle
// ---------------------------------------
class UtilHandle : public HttpHandler
{
public:
	typedef HttpApplication<UtilHandle> UtilHandleFactory;

	UtilHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~UtilHandle();

	void Response();
	void ResponseIndex();

	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onParseError( int error,const char* errorDescription );

	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );

};


// ---------------------------------------
// class EmptyHttpHandle
// ---------------------------------------
class EmptyHttpHandle : public HttpHandler
{
public:
	typedef HttpApplication<EmptyHttpHandle> EmptyHttpHandleFactory;

	EmptyHttpHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
	~EmptyHttpHandle();

	void Response();
	void ResponseIndex();

	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onParseError( int error,const char* errorDescription );

	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );

};


} }//namespace ZQ::eloop
#endif