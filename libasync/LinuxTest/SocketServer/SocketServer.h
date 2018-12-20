#ifndef _SOCKETSERVER_H
#define _SOCKETSERVER_H

#include <socket.h>
#include <eventloop.h>
#include <Log.h>
#include <Locks.h>
#include <NativeThreadPool.h>

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

class SocketServer;
typedef ZQ::common::Pointer<SocketServer> SocketServerPtr;

class SocketServer : public Socket{
public:
	SocketServer(EventLoop& loop, const std::string ip, const unsigned int port, ZQ::common::Log& log);
	~SocketServer();
	bool startServer(int backlog);	
	bool getError()
	{
		return _error;
	}
	static LoopCenter _lopCenter;
protected:
	virtual Socket::Ptr	onSocketAccepted( SOCKET sock );
	virtual	void	onSocketError(int err); 
private:
	std::string           _serverIP;
	unsigned  int         _port;
	ZQ::common::Log&       _log;

	//ZQ::common::NativeThreadPool* _pThreadPool;
	int                           _connectionNum;
	bool                         _error;


};

}






#endif