
#include "DoSocket.h"
//#include "SendThread.h"
#include <TimeUtil.h>
#include <SystemUtils.h>

namespace LibAsync {
DoSocket::DoSocket(EventLoop& loop, SOCKET sock, ZQ::common::Log& log)
	:_log(log), _bufRemain(0), _currSendSize(0), _totalSendSize(0),Socket(loop, sock),Timer(loop)
{
	_recvBuf.len = 16 * 1024;
	_recvBuf.base = (char*)malloc(sizeof(char)* _recvBuf.len);
	if( NULL == _recvBuf.base)
		assert(false);

	_sendBuf.len = 401 * 1024;
	_sendBuf.base = (char *)malloc(sizeof(char) * _sendBuf.len);
	if( NULL == _sendBuf.base )
		assert(false);
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
	if(fileName.empty())
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(DoSocket, "onSocketRecved() server[%p] failed to get fileName from client."), this);
		clear();
		return;
	}
	_fileName = fileName;
	_readPtr = new ReadFile(_log, fileName, getLoop());
	bool initret = false;
	if( _readPtr != NULL)
		initret = _readPtr->initReadFile();
	if( !initret )
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(DoSocket, "onSocketRecved() server[%p] failed to init readFile with file[%s]."), this, fileName.c_str());
		clear();
		return;
	}
	// registerWrite();
	onWritable();
}

uint64 current_time()
{
	struct timeval va;
	gettimeofday(&va, NULL);
	return (uint64)va.tv_sec * 1000 * 1000 + va.tv_usec;
}

void DoSocket::onWritable()
{
	bool loop = true;
	while(loop)
	{
		int len =0;
		if(_bufRemain <= 0)
		{
			//_sendBuf.len = sizeof(_sendBuf.base);
			memset(_sendBuf.base, '\0', _sendBuf.len);
			len = _sendBuf.len;
			if( !_readPtr->getBuffer(_sendBuf.base, len))
			{
				//assert(false);
				clear();
				return;
			}
			_currSendSize = 0;
			_bufRemain = len;
		}

		if( _bufRemain <= 0)
		{
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(DoSocket, "onWritable() server[%p] with file[%s] got bufRemain[%d]."), this, _fileName.c_str(), _bufRemain);
			if(_bufRemain == -1)
			{
				//暂时没有数据可发，请等待
				updateTimer(20);
				return;
			}
			else
			{
				assert(false);
				clear();
				return;
			}
		}
		else
		{
			AsyncBuffer aBuf;
			aBuf.base = _sendBuf.base + _currSendSize;
			aBuf.len = _bufRemain;
			uint64 star = current_time();
			int ret = sendDirect(aBuf);
			int time = (int)(current_time() - star);
			if(ret > 0)
			{
				_currSendSize += ret;
				_totalSendSize += ret;
				_bufRemain -= ret;
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onWritable() server[%p] send file[%s] size[%d/%ld] using[%d]us."), this, _fileName.c_str(), ret, _totalSendSize, time);
			}
			else if (ret == ERR_EAGAIN)
			{
				_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onWritable() server[%p] send file[%s] size[%d/%ld] recv eagain using[%d]us."), this, _fileName.c_str(), ret,  _totalSendSize, time);
				registerWrite();
				loop = false;
				return;
			}
			else 
			{
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(DoSocket, "onWritable() server[%p] send file[%s] failed with offset[%d/%ld] using[%d]us."), this, _fileName.c_str(), ret, _totalSendSize, time);
				assert(false);
				return;
			}
				//updateTimer(5);
				//return;
		}
	}	
}

void DoSocket::onTimer()
{
	cancelTimer();	
//	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onTimer() server[%p] entry."), this);
	onWritable();
}


void DoSocket::onSocketSent(size_t size)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketSent() server[%p] entry."), this);
	//int ret = getData(_sendBuf.base, _sendBuf.len);
	/*if (ret > 0)
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
	}*/
}

void DoSocket::onSocketError(int err)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(DoSocket, "onSocketError() server[%p] entry [%d]."), this, err);
	clear();
}
/*
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
*/
}