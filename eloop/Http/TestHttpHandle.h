#ifndef __TEST_HTTP_SERVER__h__
#define __TEST_HTTP_SERVER__h__

#include "httpServer.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
class TestHttpHandle : public HttpHandler
{
public:
	typedef HttpApplication<TestHttpHandle> App;

	TestHttpHandle(HttpConnection& conn, const Properties& dirProps, const Properties& appProps)
		: HttpHandler(conn, dirProps, appProps) {}

	~TestHttpHandle() {}

	void Response();
	void ResponseIndex();

 	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
 	virtual bool onBodyData( const char* data, size_t size);
 	virtual void onMessageCompleted();
 	virtual void onParseError( int error );

	virtual void	onHttpDataSent(bool keepAlive);
	virtual void	onHttpDataReceived( size_t size );
	virtual void 	onWritable();
};

} }//namespace ZQ::eloop
#endif