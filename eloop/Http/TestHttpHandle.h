#ifndef __TEST_HTTP_SERVER__h__
#define __TEST_HTTP_SERVER__h__

#include "httpServer.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
class TestHttpHandle : public IHttpHandler
{
public:
	typedef HttpApplication<TestHttpHandle> TestHttpHandleFactory;

	TestHttpHandle(HttpConnection& conn);
	~TestHttpHandle();

	void Response();
	void ResponseIndex();

 	virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
 	virtual bool onBodyData( const char* data, size_t size);
 	virtual void onMessageCompleted();
 	virtual void onParseError( int error );

	virtual void	onHttpDataSent(bool keepAlive);

	virtual void	onHttpDataReceived( size_t size );

	virtual void 	onWritable();

private:
	HttpConnection& _conn;
};

} }//namespace ZQ::eloop
#endif