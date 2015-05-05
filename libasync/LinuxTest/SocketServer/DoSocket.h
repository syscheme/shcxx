#ifndef _DOSOCKET_H
#define _DOSOCKET_H

#include <socket.h>
#include <NativeThreadPool.h>

namespace LibAsync {
//class SendThread;
class DoSocket;
typedef ZQ::common::Pointer<DoSocket> DoSocketPtr;

class DoSocket : public Socket{
public:
	DoSocket(EventLoop& loop, SOCKET sock, ZQ::common::Log& log, ZQ::common::NativeThreadPool& pool);
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

	int getData(char* buf, int buflen);

private:
	ZQ::common::Log&   				_log;
	AsyncBuffer           				_recvBuf;
	AsyncBuffer                     _sendBuf;
	ZQ::common::NativeThreadPool& 	_threadPool;
	DoSocketPtr						_thisPtr;
	std::string                     _fileName;

	FILE*                         _fd;
	int                            _sendTimes;
	//SendThread*						_sendTh;
};

}

#endif