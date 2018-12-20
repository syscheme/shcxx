#include "TestSocket.h"
#include <string.h>

TestSocket::TestSocket(ZQ::common::Log& log, LibAsync::EventLoop& evloop)
:nRecvCount(0), m_log(log), LibAsync::Socket(evloop)
{
	m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocket, "constructor entry"));
	
	mSendBuf.len = 1 * 1024;
	mSendBuf.base = (char*)malloc(sizeof(char)* mSendBuf.len);

	mRecvBuf.len = 1 * 1024;
	mRecvBuf.base = (char*)malloc(sizeof(char)* mRecvBuf.len);
}

TestSocket::~TestSocket()
{
	m_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TestSocket, "destructor entry"));
}

void TestSocket::onSocketConnected()
{
	std::string sendStr = "timeout=10.75;codec=gb2312";
	strcpy(mSendBuf.base, sendStr.c_str());

	mSendBuf.len  = sendStr.size();

	bool bRet = send(mSendBuf);
	if (!bRet)
	{
		onSocketError(LibAsync::ERR_ERROR);
	}
}

void TestSocket::onSocketRecved(size_t size)
{
	std::string recvStr(mRecvBuf.base, size);
	printf("%s\n", recvStr.c_str());

	nRecvCount += recvStr.size();
	printf("Receive Bytes: %d\n", nRecvCount);

	if (!recv(mRecvBuf))
	{
		onSocketError(LibAsync::ERR_ERROR);
	}
}

void TestSocket::onSocketSent(size_t size)
{
	std::string sendStr(mSendBuf.base, size);
	printf("Send Data: %s\n", sendStr.c_str());

	if (!recv(mRecvBuf))
	{
		onSocketError(LibAsync::ERR_ERROR);
	}
}


