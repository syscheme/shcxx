#include <unistd.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>
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
		//mLoop.getLog()(ZQ::common::Log::L_DEBUG, CLOGFMT(Socket, "clsoe() socket[%p]" ), this);
		Socket::Ptr sockPtr = this;
		mLoop.unregisterEvent(sockPtr, mSocketEvetns);
		/*mbAlive = false;
		if ( -1 != mSocket)
		{
			::close(mSocket);
			mSocket = -1;
		}*/
		if( !socketShutdown() )
		{
			realClose();
			mLingerPtr = NULL;
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
		bool bOK = setReuseAddr(true);
		assert(bOK);
		setNoDelay(true);
		setSysLinger();
		mRecBufs.clear();
		mSendBufs.clear();
		mSendValid = true;
		mRecValid = true;
		mSocketEvetns = 0;
		return bOK;
	}

	bool Socket::accept( int backlog /*= 10 * 1000*/ )
	{
		mbAlive = true;
		mbListenSocket = true;
		setReuseAddr(true);
		if (::listen(mSocket, backlog) != 0 )
			return false;
		setDeferAccept();
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
		int retryCount = 0;
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
				if ( errno == EINTR ) {
					continue;
				} else if( errno == EINPROGRESS )	{
					Socket::Ptr sockPtr (this);
					mSocketEvetns =  EPOLLOUT | EPOLLERR | EPOLLHUP;
					if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
					{	 
						onSocketError(ERR_EPOLLREGISTERFAIL);
						close();				 
						return false;
					}
				//mLoop.getLog()(ZQ::common::Log::L_DEBUG, CLOGFMT(Socket, "register not connect socket[%p] to epoll" ), this);
					return true;
				} else if ( errno == EADDRNOTAVAIL ) {
					if( ++retryCount < 8 ) {
						continue;
					}
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
		if ( !mRecValid || !mbAlive || mShutdown )
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

	bool Socket::recv(bool shutdown)
	{
		if ( !mRecValid || !mbAlive )
			return  false;
		if(buffer_size(mRecBufs) <= 0 )
			return false;
		mRecValid = false;
		mRecedSize = 0;	
		for ( LibAsync::AsyncBufferS::iterator recvIt = mRecBufs.begin(); recvIt != mRecBufs.end(); recvIt ++ )
		{
			if(recvIt->base != NULL)
				memset(recvIt->base, '\0', recvIt->len);
		}
		//mRecBufs = bufs;
		//if(recvAction())
		//	return true;
		Socket::Ptr sockPtr (this);
		mSocketEvetns = EPOLLIN ;
		if( !mLoop.registerEvent(sockPtr, mSocketEvetns) )
		{
			//onSocketError(ERR_EPOLLREGISTERFAIL);
			mRecedSize = 0;
			mRecValid = true;
			return false;
		}
		return true;	
	}

	bool Socket::send( const AsyncBufferS& bufs ) {
		if( buffer_size(bufs) == 0 )
			return false;
		if ( !mSendValid || !mbAlive)
			return false;
		mSendValid = false;

		(new PostponeSend(this))->send(bufs);
		return true;
	}

	bool Socket::innerSend( const AsyncBufferS& bufs )
	{
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
		socklen_t clientLen = sizeof(clientAddr);
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
				if(mShutdown)
				{
					mRecValid = true;
					if( !recv(mShutdown))
					{
						if(NULL != mLingerPtr)
							mLingerPtr->updateTimer(0);
						else
							realClose();
					}
					return true;
				}
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
				if(mShutdown)
				{
					mRecValid = true;
					if(NULL != mLingerPtr)
						mLingerPtr->updateTimer(0);
					else
						realClose();
					return true;
				}
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
				if(mShutdown)
				{
					mRecValid = true;
					if(NULL != mLingerPtr)
						mLingerPtr->updateTimer(0);
					else
						realClose();
					return true;
				}
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
		if (mWriteable)
		{
			mWriteable = false;
			Socket::Ptr sockPtr (this);
			mSocketEvetns = ( mSocketEvetns & (~EPOLLOUT) );
			mLoop.registerEvent(sockPtr, mSocketEvetns);
			onWritable();
			return false;
		}
		//const AsyncBufferS& bufs= sock->mSendBufs;
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
				if(mSentSize >= buffer_size(mSendBufs))
					assert(false && "send error");
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

	void Socket::errorAction(int err)
	{
		if(mShutdown)
		{
			mRecValid = true;
			if(NULL != mLingerPtr)
				mLingerPtr->updateTimer(0);
			else
				realClose();
			return ;
		}
		Socket::Ptr sockPtr (this);
		mLoop.unregisterEvent(sockPtr, mSocketEvetns);
		mbAlive = false;
		mSentSize = 0;
		mSendValid = true;
		mRecedSize = 0;
		mRecValid = true;
		onSocketError(err);
	}

	int Socket::sendDirect(const AsyncBufferS& bufs)
	{	
		if ( !mSendValid || !mbAlive )  
			return ERR_SOCKETVAIN;
		if( buffer_size(bufs) == 0 )
			return 0;
		mSendValid = false;
		int iovecSize = bufs.size();
		struct iovec* iovSend = (struct iovec*)malloc(sizeof(struct iovec) * iovecSize);
		if ( NULL == iovSend )
		{
			mSendValid = true;
			return ERR_MEMVAIN;
		}
		memset(iovSend, 0, sizeof(struct iovec) * iovecSize);
		//3ï¼Œè½¬æ¢bufså’Œiovs ,å¹¶ç»„ç»‡sendMsgæ•°æ®
		mapBufsToIovs(bufs, iovSend, 0, 0, iovecSize);
		struct msghdr sendMsg;
		sendMsg.msg_name = NULL;
		sendMsg.msg_namelen = 0;
		sendMsg.msg_iov = iovSend;
		sendMsg.msg_iovlen = iovecSize;
		sendMsg.msg_control = NULL;
		sendMsg.msg_controllen = 0;
		sendMsg.msg_flags = 0;
		//4,å‘é€bufferï¼Œå¹¶å¤„ç†ç»“æžœ
SEND_DATA:
		int ret = ::sendmsg(mSocket, &sendMsg, MSG_DONTWAIT);
		if (ret > 0)
		{
			free(iovSend);
			iovSend = NULL;
			mSendValid = true;
			return ret;
		}
		else
		{
			if ( errno == EINTR )
				goto SEND_DATA;

			if( ret == 0 || ((errno == EWOULDBLOCK || errno == EAGAIN)))
			{
				free(iovSend);
				iovSend = NULL;
				mSendValid = true;
				if( ret == 0 )
					return ERR_BUFFERTOOBIG;
				else 
					return ERR_EAGAIN;
			}
			free(iovSend);
			iovSend = NULL;
			mSendValid = true;
			return ERR_SENDFAIL;
		}//else

	}

	bool    Socket::registerWrite()
	{
		Socket::Ptr sockPtr (this);
		mSocketEvetns = ( mSocketEvetns | EPOLLOUT );
		mWriteable = true;
		if (mLoop.registerEvent(sockPtr, mSocketEvetns))
			return true;
		else
		{
			assert(false && "failed to register event");
			mWriteable = false;
			return false;
		}
		
	}

	bool Socket::setNoDelay( bool nodelay ) {
		int iNoDelay = nodelay ? 1:0;
		return 0 == setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&iNoDelay, sizeof(iNoDelay));
	//	return true;
	}

	bool Socket::setDeferAccept( ){
		int val = 1;
		return 0 == setsockopt( mSocket, SOL_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val)) ;
	}
	
	bool Socket::setSysLinger()
	{
		struct linger sLinger;
		sLinger.l_onoff = 1;
		sLinger.l_linger = 0;
		return 0 == setsockopt( mSocket, SOL_SOCKET, SO_LINGER, (const char*)&sLinger, sizeof(sLinger));
	}

	bool Socket::socketShutdown()
	{
		//mbAlive = false;
		if (-1 != mSocket)
		{
			if ( -1 == ::shutdown(mSocket, SHUT_WR) )
				return false;
		}
		//mLoop.getLog()(ZQ::common::Log::L_DEBUG, CLOGFMT(Socket, "socketshutdown() socket[%p]" ), this);
		mShutdown = true;
		AsyncBuffer buf;
		buf.len = 2 * 64 * 1024;
		buf.base = (char*)malloc(sizeof(char)* buf.len);
		if ( buf.base == NULL )
			return false;
		mLingerRecv.push_back(buf);
		Socket::Ptr sockPtr = this;
		mLingerPtr = new LingerTimer(sockPtr);
		mRecValid = true;
		mRecBufs = mLingerRecv;
		if( !recv(mShutdown) )
			return false;
		if(mLingerPtr != NULL)
			mLingerPtr->updateTimer(mLingerTime);
		return true;
	}

	bool Socket::realClose()
	{
		mLoop.getLog()(ZQ::common::Log::L_DEBUG, CLOGFMT(Socket, "realClose() socket[%p]" ), this);
		Socket::Ptr sockPtr = this;
		mLoop.unregisterEvent(sockPtr, mSocketEvetns);
		mbAlive = false;
		if ( -1 != mSocket)
		{
			::close(mSocket);
			mSocket = -1;
		}
		for ( LibAsync::AsyncBufferS::iterator recvIt = mLingerRecv.begin(); recvIt != mLingerRecv.end(); recvIt ++ )
		{
			if ( recvIt->base != NULL )
			{
				free(recvIt->base);
				recvIt->base = NULL;
			}
		}
		return true;
	}
	
	LingerTimer::LingerTimer(SocketPtr sock)
	:socketPtr(sock),
	Timer(sock->getLoop())
	{
	}

	LingerTimer::~LingerTimer()
	{
		if(NULL != socketPtr)
			socketPtr = NULL;
	}

	void  LingerTimer::onTimer()
	{
		cancelTimer();
		if (NULL != socketPtr)
		{
			if( socketPtr->socketShutdownStaus())
				socketPtr->realClose();
			socketPtr = NULL;
		}
	}
}//libAsync
