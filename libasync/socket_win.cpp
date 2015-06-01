#include "socket.h"
#include <assert.h>

namespace LibAsync{
	LPFN_ACCEPTEX  LibAsync::Socket::mlpfnAcceptEx  = NULL;
	LPFN_CONNECTEX LibAsync::Socket::mlpfnConnectEx = NULL;

	bool Socket::open( int family, int type, int protocol )
	{
		mSocket = ::WSASocket(family, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (mSocket == INVALID_SOCKET)
		{
			return false;
		}

		mLoop.addSocket(this);
		return true;	
	}

	bool Socket::isOpened() const
	{
		return (mSocket != INVALID_SOCKET);
	}

    /*bool Socket::setSendBufSize( int size )
    {
        return setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size)) == 0;
    }*/

	bool Socket::connect( const std::string& ip, unsigned short port )
	{
		SocketAddrHelper helper;
		if(!helper.parse(ip,port))
			return false;
		const struct addrinfo* info = helper.info();
		assert(info != NULL && "should never be false");
		if(!isOpened()) {			
			if(!open(info->ai_family, info->ai_socktype, info->ai_protocol))
				return false;			
		}
		
		//for ConnectEX bind local address
		std::string  localIp;
		//unsigned short localPort;
		struct sockaddr addr;
		int addrSize = sizeof(addr);
		if(getsockname(mSocket,(struct sockaddr*)&addr, &addrSize) != 0)
		{
			int errCode = WSAGetLastError();
			if (WSAEINVAL == errCode)	//not yet to bind
			{
				sockaddr_in localAddr;
				ZeroMemory(&localAddr, sizeof(sockaddr_in));
				localAddr.sin_family = AF_INET;
				localAddr.sin_addr.s_addr = INADDR_ANY;
				localAddr.sin_port = 0;
				::bind(mSocket, (sockaddr*)&localAddr, sizeof(sockaddr_in));
			}
		}

		IOCP_OVERLAPPED* pIO = new IOCP_OVERLAPPED;
		memset(&pIO->overlapped, 0, sizeof(pIO->overlapped));
		pIO->buf = NULL;
		pIO->opType = LibAsync::OP_CONNECT;
        pIO->sockRef = this;

		DWORD dwSendBytes = 0;
		bool bRet = mlpfnConnectEx(mSocket, info->ai_addr, sizeof(addrinfo), NULL, 0, &dwSendBytes, &pIO->overlapped);
		if (!bRet)
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				return false;
			}
		}
		return true;
	}

	bool Socket::getAcceptEx()
	{
		//for AcceptEx
		GUID	guidAcceptEx = WSAID_ACCEPTEX;
		DWORD	dwBytes = 0;
		SOCKET sock = ::WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		int ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &mlpfnAcceptEx, sizeof(mlpfnAcceptEx), &dwBytes, NULL, NULL);
		if (SOCKET_ERROR == ret)
		{
			//int err = WSAGetLastError();
			return false;
		}
		return true;
	}

	bool Socket::getConnectEx()
	{
		//get function point of ConnectEX
		GUID	guidConnectEx = WSAID_CONNECTEX;
		DWORD	dwBytes = 0;
		SOCKET sock = ::WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		int ret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx), &mlpfnConnectEx, sizeof(mlpfnConnectEx), &dwBytes, NULL, NULL);
		if (SOCKET_ERROR == ret)
		{
			return false;
		}
		return true;
	}

	bool Socket::innerAccept()
	{
		IOCP_OVERLAPPED* pIO = new IOCP_OVERLAPPED;
		memset(&pIO->overlapped, 0, sizeof(pIO->overlapped));
		pIO->buf = new WSABUF;
		pIO->buf->len = sizeof(struct addrinfo) * 2;
		pIO->buf->buf = new char[pIO->buf->len];
		pIO->opType = LibAsync::OP_ACCEPT;
        pIO->sockRef = this;

		DWORD dwRecvBytes = 0;
		DWORD dwReceiveDataLength = 0; //do not receive when accept 

		mAcceptSocket = ::WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mAcceptSocket == INVALID_SOCKET)
		{
			int errCode = WSAGetLastError();
			return false;
		}

		bool bRet = mlpfnAcceptEx(mSocket, mAcceptSocket, pIO->buf->buf, 0, sizeof(addrinfo), sizeof(addrinfo), &dwRecvBytes, &pIO->overlapped);
		if (!bRet)
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				return false;
			}
		}
		return true;
	}

	bool Socket::accept( int backlog)
	{
		mbListenSocket = true;
		setReuseAddr(true);

		if (::listen(mSocket, backlog) != 0)
			return false;

		return innerAccept();
	}

	bool Socket::send( const AsyncBufferS& bufs )
	{
		if (!mSendValid)
			return false;

		mSendValid = false;
		IOCP_OVERLAPPED* pIO = new IOCP_OVERLAPPED;
		memset(&pIO->overlapped, 0, sizeof(pIO->overlapped));

        pIO->sockRef = this;
		pIO->opType = LibAsync::OP_SEND;
		pIO->buf = (WSABUF*)malloc(sizeof(WSABUF) * bufs.size());

		int i = 0;
		for (AsyncBufferS::const_iterator it = bufs.begin(); it!=bufs.end(); it++, i++)
		{
			pIO->buf[i].buf = it->base;
			pIO->buf[i].len = it->len;
		}

		DWORD dwSendBytes = 0;
		DWORD dwFlag = 0;
		int ret = WSASend(mSocket, pIO->buf, bufs.size(), &dwSendBytes, dwFlag, (LPWSAOVERLAPPED)&pIO->overlapped, 0);
		if (SOCKET_ERROR == ret) //fail
		{
			int errCode = WSAGetLastError();
			if (WSA_IO_PENDING != errCode)
			{
				return false;
			}
		}
		// deal with send success event at IOCP
		return true;
	}

	bool Socket::recv( AsyncBufferS& bufs )
	{
		if (!mRecValid)
			return false;

		mRecValid = false;
		IOCP_OVERLAPPED* pIO = new IOCP_OVERLAPPED;
		memset(&pIO->overlapped, 0, sizeof(pIO->overlapped));

        pIO->sockRef = this;
		pIO->opType = LibAsync::OP_RECV;
		pIO->buf = (WSABUF*)malloc(sizeof(WSABUF) * bufs.size());

		int i=0;
		for (AsyncBufferS::iterator it= bufs.begin(); it!=bufs.end(); it++, i++)
		{
			pIO->buf[i].buf = it->base;
			pIO->buf[i].len = it->len;
		}

		DWORD dwRecvBytes = 0;
		DWORD dwFlag = 0;
		int ret = WSARecv(mSocket, pIO->buf, bufs.size(), &dwRecvBytes, &dwFlag, (LPWSAOVERLAPPED)pIO, 0);
		if (SOCKET_ERROR == ret)
		{
			int errCode = WSAGetLastError();
			if (WSA_IO_PENDING != errCode)
			{
				return false;
			}
		}
		// deal with receive success event at IOCP
		return true;
	}

    int Socket::sendDirect(const AsyncBufferS& bufs)
    {
        return 0;
    }

	void Socket::close()
	{
		if (mbAlive)
		{
			bool bret = CancelIo((HANDLE)mSocket);
			int ret = shutdown(mSocket, SD_BOTH);
			ret = closesocket(mSocket);
			mbAlive = false;
			mSocket = INVALID_SOCKET;
		}
	}
}