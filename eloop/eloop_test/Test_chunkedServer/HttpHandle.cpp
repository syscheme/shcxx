#include "HttpHandle.h"
#include <fstream>
#include <TimeUtil.h>


namespace ZQ {
	namespace eloop {


// --------------------------------------------------
// class DownloadeHandle
// --------------------------------------------------
DownloadeHandle::DownloadeHandle(HttpPassiveConn& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps)
:HttpHandler(conn,logger,dirProps,appProps),
_buf(NULL),
_fp(NULL),
_isClose(true),
_dataSize(1024*1024)
{
	_buf = (char*)malloc(_dataSize);

	Properties::const_iterator it = appProps.find("workdir");
	if( it != appProps.end()){
		_workdir = it->second;
/*		if(_workdir[_workdir.length()] != '/'){
			_workdir += "/";
		}*/
	}
}

DownloadeHandle::~DownloadeHandle()
{
	close();
	free(_buf);
}

bool DownloadeHandle::onHeadersEnd( const HttpMessage::Ptr msg)
{
	_Logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"http header data:[%s]"),msg->toRaw().c_str());
	if (msg != NULL)
	{
		_msg = msg;
		return true;
	}
	else
		return false;
}


bool DownloadeHandle::onBodyData( const char* data, size_t size)
{
	return true;
}


void DownloadeHandle::onMessageCompleted()
{
	HttpMessage::Ptr outmsg = new HttpMessage(HttpMessage::MSG_RESPONSE);

	int code = 200;
	outmsg->code(code);
	outmsg->status(HttpMessage::code2status(code));
	outmsg->keepAlive(true);
	outmsg->header("Server","LibAsYnc HtTp SeRVer");
	outmsg->header("Date",HttpMessage::httpdate());

	std::string url= _msg->url();
	std::string filename = url.substr(url.find_last_of('/')+1);
	printf("url = %s\n",url.c_str());

	//filename = "D:\\vedio\\" + filename;
	filename = _workdir + filename;
/*
	std::ifstream in(filename.c_str());   
	in.seekg(0,std::ios::end);   
	long size = in.tellg();   
	in.close();
	printf("content length = %ld\n",size);
	outmsg->contentLength(size);*/

	// 	printf("Suspended for 10 seconds!\n");
	// 	SYS::sleep(10000);
	// 	printf("Suspend the end!\n");

	outmsg->chunked(true);

	int64 start = ZQ::common::now();
	_fp = fopen(filename.c_str(),"rb");
	if (_fp == NULL)
		return;

	_isClose = false;
//	std::string head = outmsg->toRaw();
//	_conn.write(head.c_str(),head.length());
//	_conn.onRespHeader();
	_conn.beginSend(outmsg);
}


void DownloadeHandle::onError( int error,const char* errorDescription )
{

}

void DownloadeHandle::onHttpDataSent(size_t size)
{
	if (_isClose)
		return;
	
	memset(_buf,0,_dataSize);
	std::ostringstream strbuf;
	char buf[1024];
//	int ret = fread(_buf,1,_dataSize,_fp);
	int ret = fread(buf,1,1000,_fp);
	if (ret > 0)
	{
		//printf("send data:%s, size = %d\n",_buf,ret);
		char len[32];
		sprintf( len, "%x\r\n",ret);
		strbuf << len;
		strbuf << _buf << "\r\n";
		std::string outstr = strbuf.str();
		//printf("send data:%s\n,len = %d\n",outstr.c_str(),outstr.size());
		//_conn.write(outstr.c_str(),outstr.size());

		_conn.SendBody(buf,ret);
	}else
	{
		close();
		printf("close\n");
		strbuf << 0 << "\r\n\r\n";
		std::string outstr = strbuf.str();
		//_conn.write(outstr.c_str(),outstr.size());
		//_conn.onRespComplete();
		_conn.endSend();
	}

}

void DownloadeHandle::onHttpDataReceived( size_t size )
{

}

void DownloadeHandle::close()
{
	fclose(_fp);
	_isClose = true;
}

} }//namespace ZQ::eloop