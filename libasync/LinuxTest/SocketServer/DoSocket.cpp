
#include "DoSocket.h"
//#include "SendThread.h"

#include <SystemUtils.h>

namespace LibAsync {
DoSocket::DoSocket(EventLoop& loop, SOCKET sock, ZQ::common::Log& log, ZQ::common::NativeThreadPool& pool)
	:_log(log), _threadPool(pool), _sendTimes(0), Socket(loop, sock)
{
	_recvBuf.len = 2 * 1024;
	_recvBuf.base = (char*)malloc(sizeof(char)* _recvBuf.len);
	_sendBuf.len = 6 * 1024;
	_sendBuf.base = (char *)malloc(sizeof(char) * _sendBuf.len);
}

DoSocket::~DoSocket()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "~DoSocket() server[%p] entry."), this);
}

void DoSocket::clear()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "clear() server[%p] entry."), this);
	if ( _recvBuf.base != NULL )
	{
		free (_recvBuf.base);
		_recvBuf.len = 0;
		_recvBuf.base = NULL;
	}
	if( _sendBuf.base != NULL )
	{
		free(_sendBuf.base);
		_sendBuf.len = 0;
		_sendBuf.base = NULL;
	}
	/*if(_fd != NULL)
	{
		fclose (_fd);
		_fd = NULL;
	}*/
	_thisPtr =NULL;	
}

void DoSocket::onSocketConnected()
{	
	DoSocketPtr pt(this);
	_thisPtr = pt;
	pt = NULL;
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketConnected() server[%p] entry, start to recv from client."), this);
	recv(_recvBuf);
}

void DoSocket::onSocketRecved(size_t size)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketRecved() server[%p] entry, start to recv from client [%d : %s]."), this, size, _recvBuf.base);
	std::string fileName;
	fileName.assign(_recvBuf.base, size);
	//_fileName = fileName;
	//_fd = fopen(fileName.c_str(), "r");
	//assert((_fd != NULL) && "open file failed.");
	int ret = getData(_sendBuf.base, _sendBuf.len);
	if (ret > 0)
	{
		_sendBuf.len = ret;
		if ( send(_sendBuf) )
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketRecved() send successful."), ret);
		else
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(DoSocket, "onSocketRecved() send failed."), ret);
			clear();
		}
	}
	else{
		_log(ZQ::common::Log::L_WARNING, CLOGFMT(DoSocket, "onSocketRecved() getData return ret [%d]."), ret);
		clear();
	}
}

void DoSocket::onSocketSent(size_t size)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketSent() server[%p] entry."), this);
	int ret = getData(_sendBuf.base, _sendBuf.len);
	if (ret > 0)
	{
		_sendBuf.len = ret;
		if ( send( _sendBuf ) )
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketSent() send successful."), ret);
		else
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(DoSocket, "onSocketSent() send failed."), ret);
			clear();
		}
	}
	else{
		_log(ZQ::common::Log::L_WARNING, CLOGFMT(DoSocket, "onSocketSent() server[%p] file [%s] getData return ret [%d]."), this, _fileName.c_str(), ret);
		clear();
	}
}

void DoSocket::onSocketError(int err)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketError() server[%p] entry [%d]."), this, err);
	clear();
}

int DoSocket::getData(char* buf, int buflen)
{
	_sendTimes ++ ;
	if( _sendTimes > 10000 )
		return 0;
	memset(buf, '\0', buflen);
	//int ret = fread(buf, sizeof(char), buflen, _fd);
	int ret = snprintf(buf, buflen, "abcdefghijklmnopqrstuvwxyz\n");
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "getData() server[%p] get data[%d]."), this, ret);
	return ret;
}

}