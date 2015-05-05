#include "TestSocketServer.h"

namespace LibAsync{
	TestSocketServer::TestSocketServer(ZQ::common::Log& log, LibAsync::EventLoop& evloop)
		:m_log(log), msAcceptSockIndex(1), Socket(evloop)
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "listen socket constructor entry"));
	}

	TestSocketServer::TestSocketServer(ZQ::common::Log& log, LibAsync::EventLoop& evloop, SOCKET sock, int index)
		:m_log(log), mIndex(index), Socket(evloop, sock)
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "accept socket constructor entry, index[%d]"), mIndex);

		mSendBuf.len = 1 * 1024;
		mSendBuf.base = (char*)malloc(sizeof(char)* mSendBuf.len);

		mRecvBuf.len = 1 * 1024;
		mRecvBuf.base = (char*)malloc(sizeof(char)* mRecvBuf.len);
	}

	TestSocketServer::~TestSocketServer()
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "TestSocketServer destructor entry, index[%d]"), mIndex);
	}

	Socket::Ptr TestSocketServer::onSocketAccepted( SOCKET sock )
	{
		int index = msAcceptSockIndex++;
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "onSocketAccepted() entry, accept socket index[%d]"), index);

		return new TestSocketServer(m_log, getLoop(), sock, index);

		/*if (!accept())
		{
			m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "failed to accept"));
			return;
		}*/
	}

	void TestSocketServer::onSocketConnected()
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "onSocketConnected() entry, index[%d]"), mIndex);

	/*	if (!recv(mRecvBuf))
		{
			onSocketError(LibAsync::ERR_ERROR);
		}*/
	}

	void TestSocketServer::onSocketSent(size_t size)
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "onSocketSent() entry, send size [%d], index[%d]"), size, mIndex);
	}

	void TestSocketServer::onSocketRecved(size_t size)
	{
		m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocketServer, "onSocketRecved() entry, received size [%d], index[%d]"), size, mIndex);
	}
}
