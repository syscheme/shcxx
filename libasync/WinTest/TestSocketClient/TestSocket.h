#ifndef _TEST_SOCKET_H
#define _TEST_SOCKET_H

#include <eventloop.h>
#include <socket.h>

class TestSocket : public LibAsync::Socket
{
public:
	TestSocket(ZQ::common::Log& log, LibAsync::EventLoop& evloop);
	virtual ~TestSocket();

private:
	virtual	void	onSocketConnected();

	virtual	void	onSocketRecved(size_t size);

	virtual void	onSocketSent(size_t size);
private:
	ZQ::common::Log&		m_log;
	LibAsync::AsyncBuffer	mSendBuf;
	LibAsync::AsyncBuffer	mRecvBuf;
	int						nRecvCount;
};

typedef ZQ::common::Pointer<TestSocket>		TestSocketPtr;

#endif