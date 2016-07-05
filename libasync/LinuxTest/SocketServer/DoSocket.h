#ifndef _DOSOCKET_H
#define _DOSOCKET_H

#include <socket.h>

#include "ReadFile.h"

namespace LibAsync {
class DoSocket;
typedef ZQ::common::Pointer<DoSocket> DoSocketPtr;

class DoSocket : public Socket, public Timer{
public:
	DoSocket(EventLoop& loop, SOCKET sock, ZQ::common::Log& log);
	~DoSocket();

	virtual	void	onSocketConnected();
	bool doSend(AsyncBuffer sendBuf)
	{
		return send(sendBuf);
	}
	void clear();
protected:
	virtual	void	onSocketRecved(size_t size);
	virtual void	onSocketSent(size_t size); 
	virtual	void	onSocketError(int err);
	virtual void    onWritable();
	virtual void    onTimer();
//	int getData(char* buf, int buflen);

private:
	ZQ::common::Log&   				_log;
	AsyncBuffer           				_recvBuf;
	AsyncBuffer                     _sendBuf;
	int                             _bufRemain;
	int                             _currSendSize;
	int64                           _totalSendSize;
//	ZQ::common::NativeThreadPool& 	_threadPool;
	DoSocketPtr						_thisPtr;
	std::string                     _fileName;
	ReadFile::Ptr                   _readPtr;

//	FILE*                         _fd;
//	int                            _sendTimes;
//	SendThread*						_sendTh;
};

}

#endif
