#include "TestHttpHandle.h"


// ---------------------------------------
// class TestHttpHandleFactory
// ---------------------------------------
TestHttpHandleFactory::TestHttpHandleFactory()
{

}
TestHttpHandleFactory::~TestHttpHandleFactory()
{

}

IHttpHandler::Ptr TestHttpHandleFactory::create( HttpConnection& processor )
{
	IHttpHandler::Ptr handler = new TestHttpHandle(processor);
	return handler;
}





// ---------------------------------------
// class TestHttpHandle
// ---------------------------------------
TestHttpHandle::TestHttpHandle(HttpConnection& processor):m_http(processor)
{

}

TestHttpHandle::~TestHttpHandle()
{

}

void Response()
{

}

void ResponseIndex()
{

}


bool TestHttpHandle::onHttpMessage( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::HTTP_RESPONSE);

	printf("url = %s\n",msg->url().c_str());
	int code = 200;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Server","LibAsYnc HtTp SeRVer");
	outmsg->header("Date",HttpMessage::httpdate());

	std::string body = "Welcome to tinyweb. <a href=\"index.html\">index.html</a>";

	outmsg->contentLength(body.length());

	std::string head = outmsg->toRaw();
	m_http.send(head.c_str(),head.length());

	m_http.send(body.c_str(),body.length());

	m_http.setkeepAlive(false);

	return true;
}


bool TestHttpHandle::onHttpBody( const char* data, size_t size)
{
	return true;
}


void TestHttpHandle::onHttpComplete()
{

}


void TestHttpHandle::onHttpError( int error )
{

}

void TestHttpHandle::onHttpDataSent(bool keepAlive)
{

}

void TestHttpHandle::onHttpDataReceived( size_t size )
{

}

void TestHttpHandle::onWritable()
{

}
