#include <unistd.h> 
#include <fcntl.h> 

#include "socket.h"

namespace LibAsync {

	bool Socket::isOpened() const
	{
		return ( -1 != mSocket);
	}

	void Socket::close()
	{
		//if( mSocket < 0 && !mbAlive)
		//	return;
		Socket::Ptr sockPtr = this;
		mLoop.unregisterEvent(sockPtr, mSocketEvetns);
		mbAlive = false;
		if ( -1 != mSocket)
		{
			::close(mSocket);
			mSocket = -1;
		}
		return ;
	}

	bool Socket::open( int family, int type, int protocol )
	{
		mInEpoll = false;
		mSocket = socket(family,  type,  protocol);
		if (-1 == mSocket)
		{
			return false;
		}

		int flags = fcntl(mSocket, F_GETFL, 0);
		if (flags == -1)
		{
			::close(mSocket);
			return false;
		}

		if( fcntl(mSocket, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			::close(mSocket);
			return false;
		}
		setReuseAddr(true);
		mRecBufs.clear();
		mSendBufs.clear();
		mSendValid = true;
		mRecValid = true;
		mSocketEvetns = 0;
		return true;
	}

	bool Socket::accept( int backlog /*= 10 * 1000*/ )
	{
		mbAlive = true;
		mbListenSocket = true;
		setReuseAddr(true);
		if (::listen(mSocket, backlog) != 0 )
			return false;
		return acceptAction( true );
	}

	bool Socket::connect( const std::string& ip, unsigned short port )
	{
		//if the connect is alrleay exit then error
		if (mbAlive)
			return false;

		SocketAddrHelper helper;
		if(!helper.parse(ip,port))
			return false;
		const struct addrinfo* info = helper.info();
		assert(info != NULL && "should never be false" );
		// if the socket is not opened then open a new socket 
		if( !isOpened() ) {			
			if(!open(info->ai_family, info->ai_socktype, info->ai_protocol))
				return false;
		}
		bool loop = true;
		while ( loop )
		{
			struct sockaddr_in  conAddr;
			bzero(&conAddr, sizeof(conAddr));
			conAddr.sin_family = info->ai_family;
			conAddr.sin_port = htons(port);
			inet_pton(info->ai_family, ip.c_str(), &conAddr.sin_addr);
			int ret = ::connect(mSocket, (struct sockaddr*)&conAddr, sizeof(conAddr));
			if (ret != 0)
			{
				if ( errno == EINTR )
					continue;
				if( errno == EINPROGRESS )
				{
					Socket::Ptr sockPtr (this);
					mSocketEvetns =  EPOLLOUT ;
					if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
					{	 
						onSocketError(ERR_EPOLLREGISTERFAIL);
						close();				 
						return false;
					}
					return true;
				}
				onSocketError(ERR_CONNREFUSED);
				close();
				return false;
			}
			else
			{
				loop = false;
			}
		}//while 
		mbAlive = true;
		onSocketConnected();
		return true;
	}

	bool Socket::recv( AsyncBufferS& bufs )
	{
		if ( !mRecValid || !mbAlive)
			return  false;
		mRecValid = false;
		mRecedSize = 0;	
		mRecBufs = bufs;
		//if(recvAction())
		//	return true;
		Socket::Ptr sockPtr (this);
		mSocketEvetns = ( mSocketEvetns | EPOLLIN );
		if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
		{
			onSocketError(ERR_EPOLLREGISTERFAIL);
			mRecedSize = 0;
			mRecValid = true;
			return false;
		}
		return true;
	}

	bool Socket::send( const AsyncBufferS& bufs ) {
		if( buffer_size(bufs) == 0 )
			return false;
		(new PostponeSend(this))->send(bufs);
		return true;
	}

	bool Socket::innerSend( const AsyncBufferS& bufs )
	{
		if ( !mSendValid || !mbAlive)
			return false;
		mSendValid = false;
		mSentSize = 0;
		mSendBufs = bufs;
		if ( sendAction(true))
			return true;
		Socket::Ptr sockPtr (this);
		mSocketEvetns = ( mSocketEvetns | EPOLLOUT );
		if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
		{
			mSentSize = 0;
			mSendValid = true;
			onSocketError(ERR_EPOLLREGISTERFAIL);
			return false;
		}
		return true;
	}

	//get the sent pos,
	// completeSizeæ˜¯å·²å®Œæˆçš„size
	//è¿”å›žä»¥åŠsend/recvçš„bufferä¸ªæ•°ï¼ŒåŒ…è£¹å·²ç»send/recvä¸€éƒ¨åˆ†çš„buffer
	//int pos æ˜¯è¾“å‡ºå‚æ•°ï¼Œè¾“å‡ºsend/recvä¸€åŠbufferçš„å‘é€é‡
	int  Socket::getCompletePos(const AsyncBufferS& bufs, int completeSize, int& pos)
	{
		AsyncBufferS::const_iterator iterBuf;
		int  sizePos = 0, bufNum = 0;
		for (iterBuf = bufs.begin(); iterBuf!= bufs.end(); iterBuf ++)
		{
			sizePos += iterBuf->len;
			if (sizePos > completeSize)
			{
				int noSent = sizePos - completeSize;
				pos = iterBuf->len - noSent;
				break;
			}
			bufNum ++;
		}

		return bufNum;
	}

	//å®žçŽ°bufså’Œiovecæ•°ç»„çš„è½¬æ?
	//startNumè¡¨ç¤ºbufséœ€è¦send/recvçš„èµ·å§‹buf
	//pos è¡¨ç¤ºèµ·å§‹bufå·²ç»send/recvçš„ä½ç½®ï¼Œä¹Ÿå°±æ˜¯æœ¬æ¬¡send/recvçš„èµ·å§‹ä½ç½®ã€?
	//maxNum æŒ‡å®šiovæ•°ç»„å¤§å°ï¼Œé˜²æ­¢è¶Šç•?
	void Socket::mapBufsToIovs(const AsyncBufferS& bufs, struct iovec* iov, int startNum, int pos, int maxNum)
	{
		int iovPos = 0;
		int bufPos = startNum;
		while(pos < maxNum && bufPos < bufs.size())
		{
			if (pos != 0 && bufPos == startNum )
			{
				/* å¤„ç†å‘é€äº†ä¸€éƒ¨åˆ†çš„ç¬¬ä¸€å—buf */
				iov[iovPos].iov_base = bufs[bufPos].base + pos;
				iov[iovPos].iov_len = bufs[bufPos].len - pos;
				iovPos ++ ;
				bufPos++;
				continue;
			}
			iov[iovPos].iov_base = bufs[bufPos].base;
			iov[iovPos].iov_len = bufs[bufPos].len;
			iovPos ++ ;
			bufPos++;
		}
	}

	bool Socket::acceptAction( bool firstAccept/* = false*/ )
	{
		struct sockaddr_in  clientAddr;
		socklen_t clientLen;
		int sockFd = ::accept(mSocket, (struct sockaddr*)&clientAddr, &clientLen);
		if (sockFd > 0)
		{
			if (firstAccept)
			{
				Socket::Ptr sockPtr (this);
				mSocketEvetns =  EPOLLIN;
				if(!mLoop.registerEvent(sockPtr, mSocketEvetns) )	 
					return false;
			}
			Socket::Ptr newSock = onSocketAccepted(sockFd);
			if( newSock )  {
				newSock->initialServerSocket();
			}
			return true;
		}
		else
		{
			if (EAGAIN == errno || EWOULDBLOCK == errno)
			{
				if ( firstAccept )
				{
					Socket::Ptr sockPtr (this);
					mSocketEvetns =  EPOLLIN ;
					if(!mLoop.registerEvent(sockPtr, mSocketEvetns) )	 
						return false;
					return true;
				}
			}
			else
			{
				if ( !firstAccept )
				{
					Socket::Ptr sockPtr (this);
					mLoop.unregisterEvent(sockPtr, mSocketEvetns);
				}
				onSocketError(LibAsync::ERR_ERROR);
			}
			// this false is not real false , just use to out the while loop in event loop 
			return false;
		}
	}

	bool Socket::recvAction()
	{
		//Èç¹û³öÏÖrecvÊ±±»ÖÐ¶Ï£¬ÔòÐèÒª¸ÄwhileÑ­»·£¬ÆäËûÇé¿öÏÂ²»ÐèÒª
		while( mRecedSize < buffer_size(mRecBufs))
		{
			//1,èŽ·å–å½“å‰recvæ•°æ®åœ¨vector::bufsä¸­çš„ä½ç½®
			int recvPos = 0;
			int recvNum = getCompletePos(mRecBufs, mRecedSize, recvPos);
			//2ï¼Œåˆ†é…å†…å­˜ï¼Œå®šä¹‰struct iovec
			int iovecSize = mRecBufs.size() - recvNum;
			struct iovec* iovRecv = (struct iovec*)malloc(sizeof(struct iovec) * iovecSize);
			if ( NULL == iovRecv )
				break;
			memset(iovRecv, 0, sizeof(struct iovec) * iovecSize);
			//3ï¼Œè½¬æ¢bufså’Œiovs ,å¹¶ç»„ç»‡recvMsgæ•°æ®
			mapBufsToIovs(mRecBufs, iovRecv, recvNum, recvPos, iovecSize);
			struct msghdr recvMsg;
			recvMsg.msg_name = NULL;
			recvMsg.msg_namelen = 0;
			recvMsg.msg_iov = iovRecv;
			recvMsg.msg_iovlen = iovecSize;
			recvMsg.msg_control = NULL;
			recvMsg.msg_controllen = 0;
			recvMsg.msg_flags = 0;
			//4,recv æ•°æ®
			int ret = ::recvmsg(mSocket, &recvMsg, MSG_DONTWAIT);
			free(iovRecv);
			iovRecv = NULL;
			if (ret > 0)
			{
				Socket::Ptr sockPtr (this);
				mRecedSize += ret;
				mSocketEvetns = ( mSocketEvetns & (~EPOLLIN) );
				mLoop.registerEvent(sockPtr, mSocketEvetns);
				size_t recvSize = mRecedSize;
				mRecedSize = 0;
				mRecValid = true;
				onSocketRecved(recvSize);
				return true;
			}
			else if ( 0 == ret)
			{
				Socket::Ptr sockPtr (this);
				mLoop.unregisterEvent(sockPtr,mSocketEvetns);
				mbAlive = false;
				mRecedSize = 0;
				mRecValid = true;
				onSocketError(ERR_EOF3);
				break;
			}
			else
			{
				if ( errno == EINTR )
					continue;
				if(errno == EWOULDBLOCK || errno == EAGAIN)
					return false;
				//if error then unregisterEvent of EPOLLIN
				Socket::Ptr sockPtr (this);
				mSocketEvetns = ( mSocketEvetns & (~EPOLLIN) );
				mLoop.registerEvent(sockPtr, mSocketEvetns);
				mRecedSize = 0;
				mRecValid = true;
				onSocketError(ERR_RECVFAIL);
				break;
			}
		}
		return true;
	}

	bool Socket::sendAction(bool firstSend/* = false*/)
	{
		//const AsyncBufferS& bufs= sock->mSendBufs;
		int sendTimes = 0;
		bool   sendSucc = false;
		while(mSentSize < buffer_size(mSendBufs))
		{
			//1,èŽ·å–å½“å‰å‘é€æ•°æ®åœ¨vector::bufsä¸­çš„ä½ç½®
			int sentPos = 0;
			int sentNum = getCompletePos(mSendBufs, mSentSize, sentPos);
			//2ï¼Œåˆ†é…å†…å­˜ï¼Œå®šä¹‰struct iovec
			int iovecSize = mSendBufs.size() - sentNum;
			struct iovec* iovSend = (struct iovec*)malloc(sizeof(struct iovec) * iovecSize);
			if ( NULL == iovSend )
				break;
			memset(iovSend, 0, sizeof(struct iovec) * iovecSize);
			//3ï¼Œè½¬æ¢bufså’Œiovs ,å¹¶ç»„ç»‡sendMsgæ•°æ®
			mapBufsToIovs(mSendBufs, iovSend, sentNum, sentPos, iovecSize);
			struct msghdr sendMsg;
			sendMsg.msg_name = NULL;
			sendMsg.msg_namelen = 0;
			sendMsg.msg_iov = iovSend;
			sendMsg.msg_iovlen = iovecSize;
			sendMsg.msg_control = NULL;
			sendMsg.msg_controllen = 0;
			sendMsg.msg_flags = 0;
			//4,å‘é€bufferï¼Œå¹¶å¤„ç†ç»“æžœ
			int ret = ::sendmsg(mSocket, &sendMsg, MSG_DONTWAIT);
			free(iovSend);
			iovSend = NULL;
			if (ret > 0)
			{
				mSentSize += ret;
				sendSucc = true;
			}
			else if (0 == ret)
			{
				sendTimes ++;
				if(sendTimes <= 4 )
					continue;
				else
					return false;
			}
			//else if ( 0 == ret)
			//{
			//	if(firstSend)
			//		return false;
			//	Socket::Ptr sockPtr (this);
			//	mSocketEvetns = ( mSocketEvetns | EPOLLOUT );
			//	if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
			//		return false;
			//	return true;
				/*
				mSocketEvetns = ( mSocketEvetns & (~EPOLLOUT) );
				mLoop.registerEvent(sockPtr, mSocketEvetns);
				mSentSize = 0;
				mSendValid = true;
				mSendBufs.clear();
				if(errno == 2)
					//return true;
					//sendSucc = false;
					onSocketError(-30);
				else
					onSocketError(ERR_EOF2);
				sendSucc = false;
				break;*/
			//}
			else
			{
				if( ret == 0 || ((errno == EWOULDBLOCK || errno == EAGAIN)))
					return false;
				if ( errno == EINTR )
					continue;
				Socket::Ptr sockPtr (this);
				mSocketEvetns = ( mSocketEvetns & (~EPOLLOUT) );
				mLoop.registerEvent(sockPtr, mSocketEvetns);
				mSentSize = 0;
				mSendValid = true;
				onSocketError(ERR_SENDFAIL);
				sendSucc = false;
				break;
			}//else

		}//while(mSentSize < buffer_size(bufs))
		if ( sendSucc )
		{
			Socket::Ptr sockPtr (this);
			mSocketEvetns = ( mSocketEvetns & (~EPOLLOUT) );
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

}
