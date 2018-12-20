#include "SendThread.h"
#include <stdio.h>

namespace LibAsync {

SendThread::SendThread(DoSocketPtr pt, const char* buffer, int buflen, ZQ::common::NativeThreadPool& pool, ZQ::common::Log& log)
		: _log(log), _ptr(pt), ZQ::common::ThreadRequest(pool)
{
	_buffer.assign(buffer, buflen);
	_run = true;
	_fd = fopen(_buffer.c_str(), "r");
	assert((_fd != NULL) && "open file failed.");
		
}

SendThread::~SendThread()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "~SendThread() entry."));
	if(_fd != NULL)
	{
		fclose (_fd);
		_fd = NULL;
	}
	_ptr = NULL;
}

int SendThread::run()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "run() entry."));
	if (_ptr == NULL )
		return false;
	char* data = (char*) malloc(sizeof(char) * 2048);
	int pos = 0, dataLen = 0;
	bool first = false;
	while( _run )
	{
		if (first)
		{
			SYS::SingleObject::STATE sigState = _wakeup.wait(10000);
			if (sigState != SYS::SingleObject::SIGNALED ||  !_run)
				continue;
		}
		first = true;
		dataLen = getData(pos, data, 2048);
		if ( dataLen <= 0)
		{
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "run() get zero data , so set _run to false."));
			_run = false;
			continue;
		}
		AsyncBuffer sendBuf;
		sendBuf.base = data;
		sendBuf.len = dataLen;
		//SYS::sleep(20);
		if (_ptr->doSend(sendBuf))
		{
			pos += dataLen;
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "run() send data[%d]/[%d] to client."), dataLen, pos);
		}
	}
	_ptr->clear();
	delete data;
	data = NULL;
	return true;
}
int SendThread::getData(int pos, char* buf, int buflen)
{
	memset(buf, '\0', buflen);
	int ret = fread(buf, sizeof(char), buflen, _fd);
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "getData() get data[%d]."), ret);
	return ret;
}

void SendThread::final(int retcode /* =0 */, bool bCancelled /* =false */)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "final() delete the socket."));
	
	delete this;
}


}