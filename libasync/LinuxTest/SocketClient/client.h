#ifndef _CLIENT_H
#define _CLIENT_H

#include <socket.h>
#include <eventloop.h>
#include <Log.h>
#include <SystemUtils.h>

#include "writeThread.h"

namespace LibAsync {

class LoopCenter {
public:
	LoopCenter();
	virtual ~LoopCenter();
	bool	startCenter( size_t count);
	void	stopCenter();
		
	///从Center里面获取一个EventLoop，当前的实现版本是roundrobin
	virtual EventLoop&	getLoop();

private:
	typedef std::vector<EventLoop*>	LOOPS;
	size_t				mIdxLoop;
	LOOPS				mLoops;
	ZQ::common::Mutex	mLocker;
};

class SockClient;
typedef ZQ::common::Pointer<SockClient>		SockClientPtr;

class SockClient : public Socket//, public ZQ::common::NativeThread
{
public:
	SockClient(ZQ::common::Log& log, const std::string& ip, unsigned short port, EventLoop& loop, const std::string& name, writeThread::Ptr ptr);
	~SockClient();

	void doConnect();
	
	void addRecvBuf( AsyncBuffer buf );
	void addSendBuf( AsyncBuffer buf );
	bool sendBuf();
	bool recvBuf(); 
	bool status();
	static LoopCenter _lopCenter;
protected:
		virtual	void	onSocketConnected();
		virtual	void	onSocketError(int err);
		virtual	void	onSocketRecved(size_t size);
		virtual void	onSocketSent(size_t size);

private:
	ZQ::common::Log&	_log;
	std::string         _ip;
	unsigned short      _port;

	AsyncBufferS       _sendBufs;
	AsyncBufferS       _recvBufs;

	int                _recvBytes;
/*
	bool 		        _connected;
	bool                _sent;
	bool                _recved;
*/
	std::string         _fileName;
	//SockClientPtr       _thisPtr;
    int64                _startRecv;
	int64                _endRecv;
	writeThread::Ptr      _writePtr;
	bool                 _recvOK;
};


}
#endif
