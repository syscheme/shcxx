/*
evetnloop for linux 
*/
#include "eventloop.h"
#include "socket.h"

#include <TimeUtil.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <signal.h>

namespace LibAsync {

void EventLoop::createLoop()
{
	  mEpollFd = ::epoll_create(20000);
	 //mEpollFd = ::epoll_create1(EPOLL_CLOEXEC);
	  if ( mEpollFd == -1)
			::abort();
	
	  int flags = fcntl(mEpollFd, F_GETFL, 0);
	  if (flags == -1)
		::abort();

	  if( fcntl(mEpollFd, F_SETFL, flags | O_NONBLOCK) == -1)
	  	::abort();
	
	  bool eventfdCreate = mDummyEventFd.create();
	  assert(eventfdCreate && "create DummyEventFd falled.");
	  struct epoll_event ev;
	  ev.events = EPOLLIN;
	  ev.data.fd = mDummyEventFd.fd();
	  if( ::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mDummyEventFd.fd(), &ev) != 0)
	  	assert(false && "add DummyEventFd to epoll failed.");

}

void EventLoop::destroyLoop()
{
	  struct epoll_event ev;
	  ev.events = EPOLLIN;
	  ev.data.fd = mDummyEventFd.fd();
	  ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, mDummyEventFd.fd(), &ev);
	  if (mEpollFd != -1)
	  {
			::close(mEpollFd);
			mEpollFd = -1;
	  }

//	  if (mEventFd != -1)
//	  {
//			::close(mEventFd);
//			mEventFd = -1;
//	  }
	  mDummyEventFd.destroy();
}

void  EventLoop::ignoreSigpipe()
{
	signal(SIGPIPE, SIG_IGN);
}

bool EventLoop::registerEvent(Socket::Ptr sock, int event)
{
	 // Socket* sockPoint = sock._ptr;//::ZQ::common::Pointer::dynamicCast(sock).get();
	  if( sock->mSocket == -1 && !sock->mbListenSocket) {
		assert(false && "get a -1 socket to registe.");
	  }
	  if( event <= 0 )
	  {
			unregisterEvent(sock, event);
			return false;
	  }
	  struct epoll_event ev;
	  ev.events = (__uint32_t) event;
	  ev.data.ptr = reinterpret_cast<void*>(sock._ptr);
	  if (::epoll_ctl(mEpollFd, EPOLL_CTL_MOD, sock->mSocket, &ev) != 0) {
		  if (::epoll_ctl(mEpollFd, EPOLL_CTL_ADD, sock->mSocket, &ev) != 0) {
			  assert(false);
			  return false;
		  }
	  }
	  sock->mSocketEvetns = event;
	  sock->mInEpoll = true;
	  return true;
}

bool EventLoop::unregisterEvent(Socket::Ptr sock, int event)
{
	  struct epoll_event ev;
	  ev.events = (__uint32_t)event;
	  ev.data.ptr = (void*)(sock._ptr);
	  ::epoll_ctl(mEpollFd, EPOLL_CTL_DEL, sock->mSocket, &ev) ;
	  sock->mInEpoll = false;
	  return true;
// 	  {
// 			sock->mInEpoll = false;
// 			return true;
// 	  }
// 	  else{
// 			int errNUM = errno;
// 			return false;
// 	  }
}

uint64 currentTime() {
	    struct timeval v;
		gettimeofday( &v , NULL );
		return (uint64)v.tv_sec * 1000 * 1000 + v.tv_usec;
}

void EventLoop::processEvent( int64 expireAt )
{
	//int64 processStart = currentTime();
	int waitTimeOut = (int)(expireAt - ::ZQ::common::TimeUtil::now());
	if (waitTimeOut < 0)
		waitTimeOut = 0;
	struct epoll_event events[EPOLLEVENTS];
	int res = ::epoll_wait(mEpollFd, events, EPOLLEVENTS, waitTimeOut);
	//int64 loopStart = currentTime();
	int i;
	for(i = 0; i< res; i++)
	{
		if (mDummyEventFd.isA(events[i].data.fd))
		{
			//wake up event loop.
			mDummyEventFd.read();
			continue;
		}
		Socket::Ptr sock(reinterpret_cast<Socket*>(events[i].data.ptr));
		if( events[i].events & EPOLLERR )
		{
			//mLog(ZQ::common::Log::L_ERROR, CLOGFMT(EventLoop, "process get events[%d] client[%p], errno[%d]."), events[i].events, sock._ptr, errno);

			//if(errno != ENOENT && errno != EBADF &&  sock->mSocket != -1)
			
			unregisterEvent(sock, sock->mSocketEvetns);
			if( !sock->socketShutdownStaus() )
			{
				sock->onSocketError(ERR_EPOLLEXCEPTION);
			}
			else
			{
				sock->realClose();
			}
			continue;
		}

		if (sock == NULL || sock->mSocketEvetns == 0)
		{
			assert(false && "get null sock or mSocketEvents=0");
			unregisterEvent(sock, sock->mSocketEvetns);
			continue;;
		}


		if( !sock->mInEpoll )
		{
			 //mLog(ZQ::common::Log::L_DEBUG, CLOGFMT(EventLoop, "process event conection event[%d] get client[%p] not in EOPLL."), events[i].events, sock._ptr);
			unregisterEvent(sock, sock->mSocketEvetns);
			continue;
		}

		//int fd = events[i].data.fd;
		if ( !sock->alive() && !sock->mbListenSocket)
		{
			/*if (events[i].events & EPOLLOUT)
			  {
			  sock->mbAlive = true;
			  sock->mSocketEvetns = ( sock->mSocketEvetns & (~EPOLLOUT) );
			  registerEvent(sock, sock->mSocketEvetns);
			  sock->onSocketConnected();
			  continue;
			  }*/
			 //mLog(ZQ::common::Log::L_DEBUG, CLOGFMT(EventLoop, "process event conection event[%d] come client[%p]."), events[i].events, sock._ptr);
			int retVal = -1;
			socklen_t retValLen = sizeof (retVal);
			if (getsockopt (sock->mSocket, SOL_SOCKET, SO_ERROR, &retVal, &retValLen) >= 0)
			{
				if( retVal == 0)
				{
					sock->mbAlive = true;
					sock->mSocketEvetns = 0;//( sock->mSocketEvetns & ~(EPOLLOUT | EPOLLIN | EPOLLERR) );
					//registerEvent(sock, sock->mSocketEvetns);
					unregisterEvent(sock, sock->mSocketEvetns);
					sock->onSocketConnected();
					continue;
				}	   
			}
			if ( errno == EINTR || errno == EINPROGRESS )
				continue;
			unregisterEvent(sock, sock->mSocketEvetns);
			
			if( !sock->socketShutdownStaus() )
			{
				sock->onSocketError(ERR_EPOLLEXCEPTION);
			}
			else
			{
				sock->realClose();
			}
			//sock->onSocketError(ERR_CONNREFUSED);
			continue;
		}
		if ( !sock->alive())
		{
			unregisterEvent(sock, sock->mSocketEvetns);
			continue;
		}
		if ( (events[i].events & EPOLLIN) || ( events[i].events & EPOLLPRI))//recv data
		{
			if (sock->mbListenSocket)
			{
				int loopAccept = 0;
				while(/*loopAccept < 16*/sock->acceptAction() )
				{
					//if ( !sock->acceptAction() )
					//	break;
					loopAccept ++;
				}
				//mLog(ZQ::common::Log::L_DEBUG, CLOGFMT(EventLoop, "process event accept event[%d] come client[%p], accept[%d]."), events[i].events, sock._ptr, loopAccept);				
			}
			else
			{
				sock->recvAction();	
			}
			continue;
		}
		else if( events[i].events & EPOLLOUT ) //send data
		{
			sock->sendAction();
			continue;
		}//send data
		else if( /*(events[i].events & EPOLLRDHUP) ||*/ ( events[i].events & EPOLLHUP)) //peer close
		{
			/*
			unregisterEvent(sock,sock->mSocketEvetns);
			sock->mbAlive = false;
			sock->mSentSize = 0;
			sock->mSendValid = true;
			sock->mRecedSize = 0;
			sock->mRecValid = true;
			sock->onSocketError(ERR_EOF);
			*/
			if( !sock->socketShutdownStaus() )
			{
				sock->errorAction(ERR_EOF);
			}
			else
			{
				sock->realClose();
			}
			//sock->errorAction(ERR_EOF);
			continue;
		}
		else
		{
			/*
			unregisterEvent(sock, sock->mSocketEvetns);
			sock->mbAlive = false;
			sock->mSentSize = 0;
			sock->mSendValid = true;
			sock->mRecedSize = 0;
			sock->mRecValid = true;
			sock->onSocketError(ERR_EPOLLEXCEPTION);*/
			if( !sock->socketShutdownStaus() )
			{
				sock->errorAction(ERR_EPOLLEXCEPTION);
			}
			else
			{
				sock->realClose();
			}
			//sock->errorAction(ERR_EPOLLEXCEPTION);
			continue;
		}
	}//for(i = 0; i< res; i++)
   /*
   int64 loopEnd = currentTime();
   int processWait = 0;
   if( mPreTime != 0 )
   		processWait = (int) (processStart - mPreTime);
   int loopWaitTime = (int) (loopStart - processStart);

   int loopTime = (int) (loopEnd - loopStart);
   mPreTime = loopEnd;
  mLog(ZQ::common::Log::L_DEBUG, CLOGFMT(EventLoop, "processEvent() loop[%p] processWait[%d] waitTime[%d]us, loopTime[%d]us,events[%d]"), this, processWait, loopWaitTime, loopTime, res); 
  */
return;
}

void EventLoop::wakeupLoop()
{
	  //int writeData = 1;
	  //if (-1 != mEventFd)
			//::write(mEventFd, &writeData, sizeof(writeData));
	mDummyEventFd.write();
}

////DummyEventFD
DummyEventFd::DummyEventFd() {
	mSockPair[0] = mSockPair[1] = -1;
}
DummyEventFd::~DummyEventFd() {
	destroy();
}

static bool createDummyPipe( int* sockpair ) {
	int sock = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	if(sock< 0)
		return false;
	struct sockaddr_in bindaddr;
	memset(&bindaddr,0,sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(0);//^_^
	bindaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if(bind(sock,(const struct sockaddr*)&bindaddr,sizeof(bindaddr)) < 0 ) {
		close(sock);
		return false;
	}
	socklen_t addrlen = sizeof(bindaddr);
	if(getsockname(sock,(struct sockaddr*)&bindaddr,&addrlen) < 0 ) {
		close(sock);
		return false;
	}
	if( listen(sock,1) < 0 ) {
		close(sock);
		return false;
	}
	sockpair[0] = sock;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if( sock < 0 ) {
		close(sockpair[0]);
		return false;
	}

	//set sock to NON_BLOCK
	int flags = fcntl(sock, F_GETFL, 0);
	if ( -1 == flags)
	{
		close(sockpair[0]);
		close(sock);
		return false;
	}
	if( -1 == fcntl(sock, F_SETFL, flags | O_NONBLOCK) )
	{
		close(sockpair[0]);
		close(sock);
		return false;
	}

	if(connect(sock,(const struct sockaddr*) &bindaddr,sizeof(bindaddr)) < 0 ) {
		if (errno != EINTR && errno != EINPROGRESS)
		{
			close(sockpair[0]);
			close(sock);
			return false;	
		}
	}
	sockpair[1] =  sock;

	addrlen = sizeof(bindaddr);
	sockpair[0] =  accept(sockpair[0],(struct sockaddr*)&bindaddr,&addrlen);
	return sockpair[0] >= 0;
}

bool DummyEventFd::create() {
	destroy();
	if(!createDummyPipe(mSockPair))
		return false;
	return true;
}

bool DummyEventFd::isA( int fd ) const {
	assert( mSockPair[0] >= 0 );
	return mSockPair[0] == fd;
}

void DummyEventFd::read() {
	char buf[64*1024];
	recv(mSockPair[0],buf,sizeof(buf),0);
}

void DummyEventFd::write() {
	char b=1;
	send(mSockPair[1],&b,1,0);
}

void DummyEventFd::destroy() {
	if(mSockPair[0] >= 0 ) {
		close(mSockPair[0]);
		mSockPair[0] = -1;
	}
	if(mSockPair[1] >= 0 ) {
		close(mSockPair[1]);
		mSockPair[1] = -1;
	}
}

}
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312
