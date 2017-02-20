#include "TestHttpHandle.h"
#include <fstream>
#include <TimeUtil.h>

extern char gchar64_1024[64*1024];
namespace ZQ {
namespace eloop {
	/*
// ---------------------------------------
// class TestHttpHandleFactory
// ---------------------------------------
TestHttpHandleFactory::TestHttpHandleFactory()
{

}
TestHttpHandleFactory::~TestHttpHandleFactory()
{

}

HttpHandler::Ptr TestHttpHandleFactory::create( HttpConnection& processor )
{
	HttpHandler::Ptr handler = new TestHttpHandle(processor);
	return handler;
}*/


char* gchar1024 = "12345678910*12345678910*12345678910*12345678910*12345678910123456789101234567891012345678910123456789101234567891012345678910123456789101234567666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666689101234567891012345689 \
				   10123456789101234456789101234567891012345678910123456789101234567891012345678910123456789101123456789101234567891012345678910123456789101236666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666891012345678910123456456\
				   78910123456789101234567891013456789101234567891012345678910123452267891012345678910123456789101234567891012345678910123456789101266666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666668910123456789101234563456\
				   7891012123456789asdf5678910123456782345678910123456789101234567891012910123ladjflffffffffffffffffffffffffffffffffffffjdkfladsfjkad1234567890qwertyu";



// --------------------------------------------------
// class TestHttpHandle
// --------------------------------------------------
TestHttpHandle::TestHttpHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
		:HttpHandler(conn,logger,dirProps, appProps)
{

}

TestHttpHandle::~TestHttpHandle()
{

}

void TestHttpHandle::Response()
{

}

void TestHttpHandle::ResponseIndex()
{

}

bool TestHttpHandle::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);

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
	_conn.write(head.c_str(),head.length());

	_conn.write(body.c_str(),body.length());

	//_conn.setkeepAlive(false);

	return true;
}


bool TestHttpHandle::onBodyData( const char* data, size_t size)
{
	return true;
}


void TestHttpHandle::onMessageCompleted()
{

}


void TestHttpHandle::onParseError( int error,const char* errorDescription )
{

}

void TestHttpHandle::onHttpDataSent()
{

}

void TestHttpHandle::onHttpDataReceived( size_t size )
{

}

// --------------------------------------------------
// class TestHttpHandle
// --------------------------------------------------
DownloadeHandle::DownloadeHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
		:HttpHandler(conn,logger,dirProps,appProps),
		_buf(NULL),
		_fp(NULL),
		_dataSize(1024*1024)
{
	_buf = (char*)malloc(_dataSize);
}

DownloadeHandle::~DownloadeHandle()
{
	printf("The normal exit\n");
	close();
	free(_buf);
}

void DownloadeHandle::Response()
{

}

void DownloadeHandle::ResponseIndex()
{

}


bool DownloadeHandle::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);

	//printf("url = %s\n",msg->url().c_str());
	int code = 200;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Server","LibAsYnc HtTp SeRVer");
	outmsg->header("Date",HttpMessage::httpdate());

	std::string url= msg->url();
	std::string filename = url.substr(url.find_last_of('/')+1);

	filename = "D:\\vedio\\" + filename;
	//filename = "/home/zhixiang.zhu/temp/" + filename;

	std::ifstream in(filename.c_str());   
	in.seekg(0,std::ios::end);   
	long size = in.tellg();   
	in.close();
	printf("content length = %ld\n",size);

	outmsg->contentLength(size);

// 	printf("Suspended for 10 seconds!\n");
// 	SYS::sleep(10000);
// 	printf("Suspend the end!\n");

	std::string head = outmsg->toRaw();
	_conn.write(head.c_str(),head.length());


	_fp = fopen(filename.c_str(),"rb");

	if (_fp == NULL)
	{
		return false;
	}

	return true;
}


bool DownloadeHandle::onBodyData( const char* data, size_t size)
{
	return true;
}


void DownloadeHandle::onMessageCompleted()
{

}


void DownloadeHandle::onParseError( int error,const char* errorDescription )
{

}

void DownloadeHandle::onHttpDataSent()
{
	memset(_buf,0,_dataSize);
	int ret = fread(_buf,1,_dataSize,_fp);
	if (ret > 0)
	{
		//printf("send data size = %d\n",ret);
		_conn.write(_buf,ret);
	}else
	{
		close();
	}
}

void DownloadeHandle::onHttpDataReceived( size_t size )
{

}

void DownloadeHandle::close()
{
	fclose(_fp);
}

// --------------------------------------------------
// class UtilHandle
// --------------------------------------------------
UtilHandle::UtilHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
		:HttpHandler(conn,logger,dirProps, appProps)
{

}

UtilHandle::~UtilHandle()
{

}

void UtilHandle::Response()
{

}

void UtilHandle::ResponseIndex()
{

}

bool UtilHandle::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);

	printf("url = %s\n",msg->url().c_str());
	int code = 200;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Server","LibAsYnc HtTp SeRVer");
	outmsg->header("Date",HttpMessage::httpdate());

	std::string body = "cpu=[1,2,3,4,5,6,7,8,9]";

	outmsg->contentLength(body.length());

	std::string head = outmsg->toRaw();
	_conn.write(head.c_str(),head.length());

	_conn.write(body.c_str(),body.length());

	//_conn.setkeepAlive(false);

	return true;
}

bool UtilHandle::onBodyData( const char* data, size_t size)
{
	return true;
}

void UtilHandle::onMessageCompleted()
{

}

void UtilHandle::onParseError( int error,const char* errorDescription )
{

}

void UtilHandle::onHttpDataSent()
{

}

void UtilHandle::onHttpDataReceived( size_t size )
{

}


// --------------------------------------------------
// class EmptyHttpHandle
// --------------------------------------------------
EmptyHttpHandle::EmptyHttpHandle(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
	:HttpHandler(conn,logger,dirProps, appProps),
	_CurrentTime(ZQ::common::now()),
	_callCount(0)
{

}

EmptyHttpHandle::~EmptyHttpHandle()
{

}

void EmptyHttpHandle::Response()
{

}

void EmptyHttpHandle::ResponseIndex()
{

}

bool EmptyHttpHandle::onHeadersEnd( const HttpMessage::Ptr msg)
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);

	//printf("url = %s\n",msg->url().c_str());
	int code = 200;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Server","Http Download Server");
	outmsg->header("Date",HttpMessage::httpdate());

	//std::string body = "Welcome to tinyweb. <a href=\"index.html\">index.html</a>";

	//printf("body size = %d\n",body.size());

	//outmsg->contentLength(100*body.length());

	std::string head = outmsg->toRaw();
	_conn.write(head.c_str(),head.length());

	return true;
}

bool EmptyHttpHandle::onBodyData( const char* data, size_t size)
{
	return true;
}

void EmptyHttpHandle::onMessageCompleted()
{

}

void EmptyHttpHandle::onParseError( int error,const char* errorDescription )
{

}

void EmptyHttpHandle::onHttpDataSent()
{
	//std::string body = gchar1024;

	std::string body = gchar64_1024;
	
	
	if (ZQ::common::now() - _CurrentTime >= 100000)
	{
		printf("call count = %d\n",_callCount);
		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(EmptyHttpHandle,"call count = %d"),_callCount);
		_CurrentTime = ZQ::common::now();
		_callCount = 0;
	}
	_callCount++;
	printf("send body size = %d\n",body.length());
	_conn.write(body.c_str(),body.length());

}

void EmptyHttpHandle::onHttpDataReceived( size_t size )
{

}

} }//namespace ZQ::eloop
