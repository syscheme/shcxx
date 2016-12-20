#include "Test_httpServer.h"


// ---------------------------------------
// class TestHttpHandleFactory
// ---------------------------------------
TestHttpHandleFactory::TestHttpHandleFactory()
{

}
TestHttpHandleFactory::~TestHttpHandleFactory()
{

}

IHttpHandler::Ptr TestHttpHandleFactory::create( HttpProcessor* processor )
{
	IHttpHandler::Ptr handler = new TestHttpHandle(processor);
	return handler;
}





// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
TestHttpHandle::TestHttpHandle(HttpProcessor* processor):m_http(processor)
{

}

TestHttpHandle::~TestHttpHandle()
{

}


bool TestHttpHandle::onHttpMessage( const HttpMessage::Ptr msg)
{
	return NULL;
}


bool TestHttpHandle::onHttpBody( const char* data, size_t size)
{
	return false;
}


void TestHttpHandle::onHttpComplete()
{

}


void TestHttpHandle::onHttpError( int error )
{

}

void TestHttpHandle::onHttpDataSent( size_t size)
{

}

void TestHttpHandle::onHttpDataReceived( size_t size )
{

}

void TestHttpHandle::onWritable()
{

}
