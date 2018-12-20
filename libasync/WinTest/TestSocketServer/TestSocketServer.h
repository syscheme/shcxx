#ifndef _TEST_SOCKET_SERVER_H
#define _TEST_SOCKET_SERVER_H

#include <eventloop.h>
#include <socket.h>

namespace LibAsync{
	class TestSocketServer : public LibAsync::Socket
	{
	public:
		TestSocketServer(ZQ::common::Log& log, LibAsync::EventLoop& evloop);
		TestSocketServer(ZQ::common::Log& log, LibAsync::EventLoop& evloop, SOCKET sock, int index);
		virtual ~TestSocketServer();

	private:
		virtual Socket::Ptr onSocketAccepted( SOCKET sock );
		virtual	void		onSocketConnected();
		virtual	void		onSocketRecved(size_t size);
		virtual void		onSocketSent(size_t size);

	private:
		ZQ::common::Log&		m_log;
		LibAsync::AsyncBuffer	mSendBuf;
		LibAsync::AsyncBuffer	mRecvBuf;
		int						nRecvCount;
		int						mIndex;
	private:
		int						msAcceptSockIndex;
	};

	typedef ZQ::common::Pointer<TestSocketServer>		TestSocketServerPtr;
}

#endif