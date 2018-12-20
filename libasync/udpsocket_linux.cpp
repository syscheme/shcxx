#include "udpsocket.h"

namespace LibAsync {
	bool UDPSocket::recvAction()
	{
		char* buffer = mRecBufs[0].base;
		size_t len = mRecBufs[0].len;
		memset(buffer, '\0', len);
		struct sockaddr_in   srcAddr;
		size_t  addrlen = sizeof(srcAddr);
		int ret = ::recvfrom(mSocket, buffer, len, MSG_DONTWAIT, (struct sockaddr*) &srcAddr, (socklen_t*)&addrlen);
		char strAddr[INET_ADDRSTRLEN];
		memset(strAddr, '\0', INET_ADDRSTRLEN);
		std::string peerIp;
		if (NULL != ::inet_ntop(AF_INET, (void *)&srcAddr.sin_addr, strAddr, INET_ADDRSTRLEN))
			peerIp.assign(strAddr);
		unsigned short peerPort = htons(srcAddr.sin_port);

		if (ret >= 0)
		{ 
			if (mShutdown)
			{
				mRecValid = true;
				if (ret == 0)
				{
					realClose();
					return true;
				}
				if (!recv(mShutdown))
				{
					if (NULL != mLingerPtr)
						mLingerPtr->updateTimer(0);
					else
						realClose();
				}
				return true;
			}
			Socket::Ptr sockPtr(this);
			mSocketEvetns = (mSocketEvetns & (~EPOLLIN));
			mLoop.registerEvent(sockPtr, mSocketEvetns);
			size_t recvSize = ret;
			mRecValid = true;
			onSocketRecved(recvSize, peerIp, peerPort);
			return true;

		}
		else
		{
			if (EINTR == errno)
				return recvAction();
				//continue;
			if (EWOULDBLOCK == errno || EAGAIN == errno)
				return false;
			if (mShutdown)
			{
				mRecValid = true;
				if (NULL != mLingerPtr)
					mLingerPtr->updateTimer(0);
				else
					realClose();
				return true;
			}
			Socket::Ptr sockPtr(this);
			mSocketEvetns = (mSocketEvetns & (~EPOLLIN));
			mLoop.registerEvent(sockPtr, mSocketEvetns);
			mRecValid = true;
			onSocketError(ERR_RECVFAIL);
			return true;
		}
		return true;
	}

	bool UDPSocket::sendAction(bool firstSend/* = false*/)
	{
		if (mWriteable)
		{
			mWriteable = false;
			Socket::Ptr sockPtr (this);
			mSocketEvetns = ( mSocketEvetns & (~EPOLLOUT) );
			mLoop.registerEvent(sockPtr, mSocketEvetns);
			onWritable();
			return false;
		}

		if (mPeerAddr.empty() || mPeerPort == 0)
			return true;
		bool   sendSucc = false;
		char* buffer = mSendBufs[0].base;
		size_t len = mSendBufs[0].len;
		SocketAddrHelper sendHelper(false);
		if (!sendHelper.parse(mPeerAddr, mPeerPort))
			return true;
		const struct addrinfo* info = sendHelper.info();
		assert(info != NULL && "should never be false");
		struct sockaddr_in  sendAddr;
		bzero(&sendAddr, sizeof(sendAddr));
		sendAddr.sin_family = info->ai_family;
		sendAddr.sin_port = htons(mPeerPort);
		inet_pton(info->ai_family, mPeerAddr.c_str(), &sendAddr.sin_addr);

		while (mSentSize < len)
		{
			buffer += mSentSize;
			size_t  sendLen = len - mSentSize;
			int ret = ::sendto(mSocket, buffer, sendLen, MSG_DONTWAIT, (const struct sockaddr*)&sendAddr, sizeof(sendAddr));
			if (ret > 0)
			{
				mSentSize += ret;
				sendSucc = true;
			}
			else
			{
				if (ret == 0 || ((errno == EWOULDBLOCK || errno == EAGAIN)))
					return false;
				if (errno == EINTR)
					continue;
				Socket::Ptr sockPtr(this);
				mSocketEvetns = (mSocketEvetns & (~EPOLLOUT));
				mLoop.registerEvent(sockPtr, mSocketEvetns);
				mSentSize = 0;
				mSendValid = true;
				onSocketError(ERR_SENDFAIL);
				sendSucc = false;
				break;

			}	
		}
		if (sendSucc)
		{
			Socket::Ptr sockPtr(this);
			mSocketEvetns = (mSocketEvetns & (~EPOLLOUT));
			mLoop.registerEvent(sockPtr, mSocketEvetns);
			size_t sentSize = mSentSize;
			mSentSize = 0;
			mSendBufs.clear();
			mSendValid = true;
			onSocketSent(sentSize);
			return true;
		}
		return true;
	}

	int	UDPSocket::sendDirect(AsyncBuffer buf)
	{
		if (!openudp())
			return ERR_UDPSOCKETNOTALIVE;
		if (mPeerAddr.empty() || mPeerPort == 0)
			return ERR_ERROR;

		if (!mSendValid || !mbAlive)
			return ERR_SOCKETVAIN;
		if (buf.len <= 0)
			return 0;
		mSendValid = false;
	SEND_DATA:
		const struct addrinfo* peer = mPeerInfo.info();
		int ret = ::sendto(mSocket, buf.base, buf.len, MSG_DONTWAIT, peer->ai_addr, peer->ai_addrlen );
		if (ret > 0)
		{
			mSendValid = true;
			return ret;
		}
		else
		{
			if (errno == EINTR)
				goto SEND_DATA;

			if (ret == 0 || ((errno == EWOULDBLOCK || errno == EAGAIN)))
			{
				mSendValid = true;
				if (ret == 0)
					return ERR_BUFFERTOOBIG;
				else
					return ERR_EAGAIN;
			}
			mSendValid = true;
			return ERR_SENDFAIL;
		}//else
	}
}//namespace libasync
