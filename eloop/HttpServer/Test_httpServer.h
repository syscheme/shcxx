#ifndef __TEST_HTTP_SERVER__h__
#define __TEST_HTTP_SERVER__h__

#include "httpServer.h"



// ---------------------------------------
// class TestHttpHandleFactory
// ---------------------------------------
class TestHttpHandleFactory:public IHttpHandlerFactory
{
public:
	TestHttpHandleFactory();
	~TestHttpHandleFactory();

	virtual IHttpHandler::Ptr create( HttpProcessor* processor );

};



// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
class TestHttpHandle : public IHttpHandler
{
public:
	TestHttpHandle(HttpProcessor* processor);
	~TestHttpHandle();


	void Response();
	void ResponseIndex();

 	virtual bool onHttpMessage( const HttpMessage::Ptr msg);
 	virtual bool onHttpBody( const char* data, size_t size);
 	virtual void onHttpComplete();
 	virtual void onHttpError( int error );

	virtual void	onHttpDataSent(bool keepAlive);

	virtual void	onHttpDataReceived( size_t size );

	virtual void 	onWritable();

private:
	HttpProcessor* m_http;
};










#endif