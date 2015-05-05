
#include "DataCommunicatorUnite.h"
#include <poll.h>
#include <arpa/inet.h>
#include <TimeUtil.h>

#define MLOG mEnv.getLogger()
#define COMMFMT(x,y) "COMM[%lld]TID[%08X][%10s]\t"y,mId,pthread_self(),#x

namespace ZQ
{
namespace DataPostHouse
{

ASocket::ASocket( DataPostHouseEnv& env, DataPostDak& dak )
:mSock(-1 ),
mType(COMM_TYPE_NULL),
mUserData(0),
mDataDialog(0),
mEnv(env),
mDak(dak)
{
	internalInitialize();
}
ASocket::ASocket( DataPostHouseEnv& env ,  DataPostDak& dak, const SOCKET& s , const CommunicatorType& type , SharedObjectPtr userData  )
:mSock(s),
mType(type),
mUserData(userData),
mDataDialog(NULL),
mEnv(env),
mDak(dak)
{
	internalInitialize();
}


void ASocket::internalInitialize()
{	
	mId = mEnv.generateConnectionId();	
	mReadBuf	=	(char*)malloc( mEnv.getReadBufferSize() );
	assert( mReadBuf != NULL );
	mReadBufLen	=	mEnv.getReadBufferSize();
	mWriteBuf = "";
	
	updateActiveStamp();
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(ASocket,"communicator [%lld] is setup"),mId);
}

ASocket::~ASocket( )
{	
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(ASocket,"communicator [%lld] is destroyed"),mId);	
	if( mReadBuf )
	{
		free( mReadBuf );
		mReadBuf = NULL;
	}
	mDataDialog = 0;
}


bool ASocket::createSocket( int family, int type , int protocol )
{
	mSock	= socket(family,type,protocol);
	if( mSock == -1 )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(createSocket,"createSocket failed with family[%d] type[%d] protocol[%d] and errorCode[%d]"),
			family , type , protocol , errno);		
		return false;
	}

	//set socket no block
//	if( setNonBlock(mSock) == -1)
//		return false;
	return true;
}

bool ASocket::bind( const std::string& localIp , const std::string& localPort )
{
	assert( mSock != -1 );
	
	if( !mAddrInfoHelper.convert(localIp,localPort) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(bind,"can't get address information with ip[%s] port[%s] and error[%d]"),
			localIp.c_str() , localPort.c_str(), errno);
		return false;
	}
	
	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );

	int iRet = ::bind( mSock, adInfo->ai_addr ,(int)adInfo->ai_addrlen );
	if( iRet ==  -1 )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(ASocket , "failed to bind socket with ip[%s] port[%s] and errorCode string[%s]" ),
			localIp.c_str() , localPort.c_str() , strerror(errno));
		return false;
	}
	return true;
}

bool ASocket::listen( int backLog )
{
	assert( mSock != -1 );
	int iRet = ::listen( mSock , backLog );
	if( iRet == -1 )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(listen,"failed to invoke listen and errorCode string[%s]"),
			strerror(errno));
		return false;
	}
	return true;
}

ASocket* ASocket::accept()
{
//	assert( mSock != -1 );
    if(mSock == (-1)) {
        return NULL;
    }

	sockaddr_storage addr;
	int addLen = sizeof(addr);

	int s = ::accept( mSock, (sockaddr*)&addr , (socklen_t*)&addLen );
	if( s == -1 )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(accept,"accept failed and errorCode string[%s]"),strerror(errno));
		return NULL;
	}
	else
	{
		ASocket* pSocket = new ASocket( mEnv , mDak, s );	
		pSocket->initializeSockName();			
		pSocket->mType = COMM_TYPE_TCP;
		pSocket->mCompletionKey.dataCommunicator = pSocket;
		pSocket->mCompletionKey.mStatus = true;

		MLOG(ZQ::common::Log::L_DEBUG,"COMM[%lld] accept new socket [%d]",pSocket->getCommunicatorId(), s);

		return pSocket;
	}
}

void ASocket::initializeSockName( )
{
	mAddrRemote.addrLen = sizeof(mAddrRemote.u.storage );
	if( getpeername( mSock, (sockaddr*)&mAddrRemote.u ,(socklen_t*)&mAddrRemote.addrLen ) == -1 )
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(initializeSockName,"can't get remote addr,error code[%d] string[%s]"), errno, strerror(errno));
	}

	mAddrLocal.addrLen = sizeof( mAddrLocal.u.storage );
	if( getsockname( mSock , (sockaddr*)&mAddrLocal.u , (socklen_t*)&mAddrLocal.addrLen ) == -1)
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(initializeSockName,"can't get local addr,error code[%d] string[%s]"), errno, strerror(errno));
	}
}

bool ASocket::connect( const std::string& remoteIp , const std::string& remotePort ,uint32 timeout  )
{
	assert( mSock != -1 );
	if( !mAddrInfoHelper.convert( remoteIp , remotePort ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(connect,"can't get address information with ip[%s] port[%s] and error string[%s]"),
			remoteIp.c_str(), remotePort.c_str(), strerror(errno));
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );

	bool bCheckTimeout = ( timeout != (uint32)-1 );	
	
	if( bCheckTimeout)
	{
		//set to nonblock mode
		//ioctlsocket(mSock, FIONBIO, (u_long FAR*) &iMode);
		fcntl(mSock, F_SETFL, O_NONBLOCK);
	}
	
	int iRet = ::connect( mSock , adInfo->ai_addr , (int)adInfo->ai_addrlen );
	if( iRet == -1 )
	{
		if( !bCheckTimeout )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to connect ip[%s] port[%s] and errorCode[%s]"),
				remoteIp.c_str() , remotePort.c_str() , strerror(errno));				
			return false;
		}
	}
	if( bCheckTimeout )
	{
		struct pollfd pfd;
		pfd.fd 		= mSock;
		pfd.events	= POLLOUT;
		int iRet = poll(&pfd,1,timeout);
		if( iRet < 0 )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to detect socket status, error[%d]"), errno );
			return false;
		}
		int so_error;
		socklen_t len = sizeof so_error;
		if( getsockopt(mSock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0 )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to connect to peer[%s:%s],error[%d]"),remoteIp.c_str() , remotePort.c_str() , errno );
			return false;
		}
		//reset to block mode
		fcntl(mSock, F_SETFL, O_NONBLOCK);
	}
	return true;
}

int ASocket::setNonBlock(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
	{
		MLOG(ZQ::common::Log::L_ERROR,"Set socked[%d] nonblock error string[%s]",sockfd,strerror(errno));
        return -1;
    }
    return 0;
}

void ASocket::attchDialog( IDataDialogPtr dialog ) 
{
	mDataDialog = dialog;
}

void ASocket::attachUserData( SharedObjectPtr userData )
{
	mUserData = userData;
}

CommunicatorType ASocket::getCommunicatorType( ) const 
{
	return mType;
}

int64 ASocket::getCommunicatorId( ) const
{
	return mId;
}
bool ASocket::getCommunicatorAddrInfo( CommAddr& local , CommAddr& remote ) const
{
	local = mAddrLocal;
	remote = mAddrRemote;
	return true;
}
bool ASocket::isValid() const
{
	return ( mSock != -1 );
}

int	ASocket::getCommunicatorDescriptor( ) 
{
	return mSock;
}

DataCompletionKey* ASocket::getCompletionKey( )
{
	return &mCompletionKey;
}

int32 ASocket::read( int8* buffer , size_t bufSize ,int32 timeoutInterval /* = -1 */ )
{
	assert( mSock != -1 );
	assert( buffer != NULL );
	assert( bufSize != 0 );

	updateActiveStamp();
	
	if( timeoutInterval <= 0 )
	{
		int iRet = ::recv( mSock , reinterpret_cast<char*>(buffer) , static_cast<int>(bufSize) , 0 );
		if( iRet == -1 )
		{
			MLOG(ZQ::common::Log::L_ERROR,
				COMMFMT(read,"failed to invoke recv and errorCode[%s]"),
				strerror(errno));
			return ERROR_CODE_OPERATION_FAIL;
		}
		return static_cast<int32>(iRet);
	}
	else
	{
		struct pollfd pfd;
		pfd.fd = mSock;
		pfd.events = POLLIN;
		int iRet = poll(&pfd,1,timeoutInterval);
		if(iRet == -1)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(read,"poll return error code[%d] string[%s]"),errno,strerror(errno));
			return ERROR_CODE_OPERATION_FAIL;
		}
		else if( iRet == 0)
		{
			return ERROR_CODE_OPERATION_TIMEOUT;
		}
		else
		{
			if((pfd.revents & POLLIN) == POLLIN )
			{
				iRet = ::send( mSock , buffer , bufSize , 0 );
				if( iRet == -1 )
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(read,"failed to invoke send and errorCode[%d]"),
						errno);
					return ERROR_CODE_OPERATION_FAIL;
				}
				else
				{
					return iRet ;
				}
			}
			else
			{
				MLOG(ZQ::common::Log::L_ERROR,COMMFMT(read,
					"poll return successful but the socket not in the set"));
				return ERROR_CODE_OPERATION_FAIL;
			}
		}
	}
}

int32 ASocket::readAsync( )
{
	if( !isValid() )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(readAsync,"failed to invoke read because communicator is not valid") );
		return ERROR_CODE_OPERATION_FAIL;
	}

	updateActiveStamp();
	memset(mReadBuf, 0, mReadBufLen);
	int dwFlag = MSG_DONTWAIT;

	int iRet = recv( mSock , mReadBuf , mReadBufLen , dwFlag);
	
	//test
//	MLOG(ZQ::common::Log::L_INFO,"%s",mReadBuf);

	if( iRet == -1 )
	{
		if( errno == EAGAIN || errno == EWOULDBLOCK )
		{
			return ERROR_CODE_OPERATION_PENDING;
		}
		else
		{
			MLOG(ZQ::common::Log::L_INFO,COMMFMT(readAsync,"failed to invoke recv and errorCode[%d] string[%s]"), errno,strerror(errno));
			return ERROR_CODE_OPERATION_FAIL;
		}
	}
	else
	{
		return iRet;		
	}
}

int32 ASocket::write( const int8* buffer , size_t bufSize , int32 timeoutInterval /* = -1 */ )
{
	if( !isValid() )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke write because communicator is not valid") );
		return ERROR_CODE_OPERATION_FAIL;
	}
	assert( buffer != NULL );
	assert( bufSize != 0 );
	
	updateActiveStamp();
	
	if ( timeoutInterval <= 0 )
	{
		int32 retVal = bufSize;
		size_t pos = 0;
		//int64 startTime = ZQ::common::now();
		while( bufSize > 0 )
		{
			int iRet = ::send( mSock , buffer + pos , bufSize , 0 );
			if( iRet <= 0 )
			{			
				MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke send and errorCode[%s]"),
						strerror(errno) );
				return ERROR_CODE_OPERATION_FAIL;
			}
			else
			{
				bufSize -= iRet;
				pos += iRet;
			}
		}
		//int64 delta = ZQ::common::now() - startTime;
		//if( delta > 50 )
		//{
		//	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(write,"took[%lld]ms to send[%d]bytes to peer"),delta , retVal );
		//}
		return retVal;
	}
	else
	{
		struct pollfd pfd;
		pfd.fd = mSock;
		pfd.events = POLLOUT;
		int iRet = poll(&pfd,1,timeoutInterval);
		if(iRet == -1)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"poll return error code[%d] string[%s]"),errno,strerror(errno));
			return ERROR_CODE_OPERATION_FAIL;
		}
		else if( iRet == 0)
		{
			return ERROR_CODE_OPERATION_TIMEOUT;
		}
		else
		{
			if((pfd.revents & POLLOUT) == POLLOUT )
			{
				iRet = ::send( mSock , buffer , bufSize , 0 );
				if( iRet == -1 )
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke send and errorCode[%d]"),
						errno);
					return ERROR_CODE_OPERATION_FAIL;
				}
				else
				{
					return iRet ;
				}
			}
			else
			{
				MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,
					"poll return successful but the socket not in the set"));
				return ERROR_CODE_OPERATION_FAIL;
			}
		}

	}
}

int32 ASocket::writeAsync( const int8* buffer , size_t bufSize )
{
	if( !isValid() )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(writeAsync,"failed to invoke write because communicator is not valid") );
		return ERROR_CODE_OPERATION_FAIL;
	}
	assert( buffer != NULL );
	assert( bufSize != 0 );

	updateActiveStamp();
	
	int dwFlag = MSG_DONTWAIT;

	int iRet = ::send( mSock , buffer, bufSize, dwFlag);

	if( iRet == -1 )
	{
		if( errno == EAGAIN || errno == EWOULDBLOCK)
		{
			mWriteBuf = std::string(buffer,bufSize);
			return ERROR_CODE_OPERATION_PENDING;
		}
		else
		{
			return ERROR_CODE_OPERATION_FAIL;
		}
	}
	else
	{
		mWriteBuf = "";
		return ERROR_CODE_OPERATION_OK ;
	}
}

void ASocket::clear()
{
	IDataCommunicatorPtr tmpComm = 0 ;
	{
		ZQ::common::MutexGuard gd(mLocker);
		if( !mCompletionKey.dataCommunicator )
			return;
		tmpComm = mCompletionKey.dataCommunicator;
		mCompletionKey.dataCommunicator = 0;
	}
	
	if( mDataDialog != 0 )
	{
		mDataDialog->onCommunicatorDestroyed(this);
	}

	mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );	

	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(clear,"communicator clear resources"));
	tmpComm = 0;
}

void ASocket::close()
{
	{
		ZQ::common::MutexGuard gd(mLocker);
		if( mSock != -1 )
		{
			int tmpSock = mSock;
			mSock = -1;
			MLOG(ZQ::common::Log::L_INFO,COMMFMT(close,"close socket [%d]"),tmpSock);
			mDak.removeOutEpoll(mId,tmpSock,&mCompletionKey	);
			if(mType != COMM_TYPE_UDP)
			{//do not close socket here if the communicator is UDP
				//because UDP use a dummy socket 
				::shutdown( tmpSock , SHUT_RDWR );
				int rc = ::close(tmpSock);
				if(rc != 0)
					MLOG(ZQ::common::Log::L_WARNING, COMMFMT(close,"close socket [%d] failed string [%s]]"),tmpSock, strerror(errno));
				else
					MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(close,"socket [%d] closed"),tmpSock);
			}
		}
	}
	clear();
}

int32 ASocket::onDataAsyncError(void)
{
#pragma message(__MSGLOC__"Should I close communicator if error occurred ?")
	assert(mDataDialog != 0 );
	mDataDialog->onError();
	onCommunicatorClosed();
	return ERROR_CODE_OPERATION_OK;
}
void ASocket::onCommunicatorClosed( ) 
{	
	close( );//close socket and inform dialog the communicator is closed
	assert( mEnv.getDataDialogFactory() != 0 );
	clear( );
}

int32 ASocket::onDataAsyncResult(struct ::epoll_event& epollevent)
{
	if(!isValid())
	{
		onCommunicatorClosed();
		return ERROR_CODE_OPERATION_FAIL;
	}
	
	if( !mDataDialog)
		return ERROR_CODE_OPERATION_FAIL;

#if defined _DEBUG || defined DEBUG
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(onDataAsyncResult,"Has event [%d]"),epollevent.events);
#endif

	if( (epollevent.events & EPOLLIN) == EPOLLIN ) //have data read
	{
		if(!isValid() )
		{//socket is closed
			onCommunicatorClosed();			
			return ERROR_CODE_OPERATION_FAIL;
		}

		do 
		{
			int32 iRet = readAsync();
			if( iRet > 0 )
			{
				try
				{
					mDataDialog->onRead( mReadBuf , static_cast<size_t>(iRet) );
				}
				catch(...)
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when call dialog's onRead()"));			
				}
			}
			else
			{
				if( iRet == 0 )
				{
					MLOG(ZQ::common::Log::L_INFO,COMMFMT(onDataAsyncResult,"communicator has been closed"));
					onCommunicatorClosed();
					return ERROR_CODE_OPERATION_FAIL;
				}
				else if ( iRet == ERROR_CODE_OPERATION_PENDING  )
				{
					return ERROR_CODE_OPERATION_OK;
				}
				else
				{
					MLOG(ZQ::common::Log::L_INFO,COMMFMT(onDataAsyncResult,"Failed to invoke readAsync"));
					onCommunicatorClosed();
					return ERROR_CODE_OPERATION_FAIL;
				}
			}

			if(!isValid() )
			{//socket is closed
				onCommunicatorClosed();			
				return ERROR_CODE_OPERATION_FAIL;
			}

		} while (true);
	}
	else if((epollevent.events & EPOLLOUT) == EPOLLOUT)
	{
		int32 size = 0;
		if(mWriteBuf.size())
		{
			size = writeAsync(mWriteBuf.c_str(),mWriteBuf.length());
			if(size <= 0)
			{
				MLOG(ZQ::common::Log::L_ERROR,"onDataAsyncResult() writeAsync failed error string[%s]",strerror(errno));
				onCommunicatorClosed();
				size = 0;
			}
		}

		try
		{
			mDataDialog->onWritten( static_cast<size_t>(size) );
		}
		catch(...)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when invoke dialog's onWritten"));
			return ERROR_CODE_OPERATION_FAIL;
		}
		if(size > 0)
			return ERROR_CODE_OPERATION_OK;
		else
			return ERROR_CODE_OPERATION_FAIL;
	}
	else if((epollevent.events & EPOLLHUP) == EPOLLHUP || (epollevent.events & EPOLLERR) == EPOLLERR)
	{
		MLOG(ZQ::common::Log::L_INFO,"onDataAsyncResult() have a hang up or error event");
		onCommunicatorClosed();
		return ERROR_CODE_OPERATION_FAIL;

	}
	else
	{
		MLOG(ZQ::common::Log::L_ERROR,"onDataSsyncResult() dont known the event type[%d]",epollevent.events);
		onCommunicatorClosed();
		return ERROR_CODE_OPERATION_FAIL;
	}	

}

IDataDialogPtr ASocket::getDataDialog( )
{
	return mDataDialog;
}

SharedObjectPtr	ASocket::getUserData( ) 
{
	return mUserData;
}

bool sockaddrToString(struct ::sockaddr* psaddr, char* buf, size_t bufLen, int& port )
{
    if(buf == NULL)
        return false;

    sockaddr_in* inadd;
    sockaddr_in6* in6add;

    if(psaddr->sa_family == AF_INET)
    {
        inadd = (sockaddr_in*)psaddr;
        port = ntohs(inadd->sin_port);
        const char* pd = inet_ntop(AF_INET,(void*)&(inadd->sin_addr), buf,bufLen);
        if(pd == NULL)
            return false;
    }

    else if(psaddr->sa_family == AF_INET6)
    {
        in6add = (sockaddr_in6*)psaddr;
        port = ntohs(in6add->sin6_port);
        const char* pd = inet_ntop(AF_INET6,(void*)&(in6add->sin6_addr), buf,bufLen);
        if(pd == NULL)
            return false;
    }

    else
        return false;

    return true;

}

void ASocket::getLocalAddress(std::string& localIP, std::string& localPort) const
{
	ZQ::DataPostHouse::CommAddr local, peer;
	getCommunicatorAddrInfo(local, peer);
	
	char buf[256] = {0};
	int nPort = 0;
	if( !sockaddrToString((struct ::sockaddr*)&local.u.addr, buf, sizeof(buf), nPort) )
	{
		MLOG(ZQ::common::Log::L_ERROR,"getLocalAddress() socket address to string failed" );
		return;
	}
	
	localIP = buf;
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", nPort);
	localPort = buf;
}

void ASocket::getRemoteAddress(std::string& remoteIP, std::string& remotePort) const
{	
	ZQ::DataPostHouse::CommAddr local, peer;
	getCommunicatorAddrInfo(local, peer);
	
	char buf[256] = {0};
	int nPort = 0;
	if( !sockaddrToString((struct ::sockaddr*)&peer.u.addr, buf, sizeof(buf), nPort) )
	{
		MLOG(ZQ::common::Log::L_ERROR,"getRemoteAddress() socket address to string failed" );
		return;
	}
	
	remoteIP = buf;
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", nPort);
	remotePort = buf;
}

void ASocket::updateActiveStamp()
{
	_lastActiveTime = SYS::getTickCount();
}

uint32 ASocket::getIdleTime()
{
	return (uint32)(SYS::getTickCount() - _lastActiveTime);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
AServerSocketTcp::AServerSocketTcp(  DataPostDak& dak , DataPostHouseEnv& env )
:ASocket(env,dak),
mDak(dak)
{
}

AServerSocketTcp::~AServerSocketTcp( )
{
	close();
	clear();
}

bool AServerSocketTcp::startServer( const std::string& localIp , 
									const std::string& localPort , 
									SharedObjectPtr userData,
									int32 maxBacklog )
{
	//step 1
	//create socket and bind it
	int family = (localIp.find(":") != std::string::npos) ? AF_INET6 :AF_INET;
	mAddrInfoHelper.init( family , SOCK_STREAM , IPPROTO_TCP);
	
	if( !mAddrInfoHelper.convert( localIp , localPort ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(startServer,"can't get address information with ip[%s] port[%s] and error string[%s]"),
			localIp.c_str(), localPort.c_str() , strerror(errno) );
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );
	if (!createSocket( adInfo->ai_family, SOCK_STREAM , IPPROTO_TCP ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"create server socket failed and errorCode [%s]"), 
				strerror(errno) );
		return false;
	}

	int breuse = true;
	int re = setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(void*)&breuse, sizeof(breuse));
	if(re == -1)
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(startServer,"set reuse address is failed code[%d] string[%s]"),
		errno,strerror(errno)); 
	}
/*
	int nodelay = true;
	re = setsockopt(mSock,IPPROTO_TCP,TCP_NODELAY,(void*)&nodelay, sizeof(nodelay));
	if(re == -1)
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(startServer,"set socket no delay is failed code[%d] string[%s]"),
		errno,strerror(errno)); 
	}
*/
	if(!bind(localIp,localPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to bind local address[%s][%s] and errorCode[%s]"),
			localIp.c_str() , localPort.c_str() , strerror(errno) );
		return false;
	}

	if(!listen(maxBacklog))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to listen and errorCode[%s]"),strerror(errno) );
		return false;
	}
	mUserData = userData;	
/*
	setNonBlock(getCommunicatorDescriptor());
	mCompletionKey.dataCommunicator = this;
	mCompletionKey.mStatus = true;
	if(!mDak.addnewCommunicator(mCompletionKey.dataCommunicator))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"add this socket to epoll failed"));
		return false;
	}
*/
	return start();
}
bool AServerSocketTcp::init()
{
	return true;
}

int AServerSocketTcp::run()
{

	while (true)
	{
		ASocket* pSocket = accept();
		if( pSocket == NULL )
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENFILE )
				continue;
			//detect if we close the listen socket
			MLOG(ZQ::common::Log::L_WARNING,"AServerSocketTcp::run() accept function return failed");
			break;
		}
		else
		{
			IDataCommunicatorExPtr pComm = pSocket;
			pSocket->attachUserData(mUserData);
			IDataDialogPtr  dialog = mEnv.getDataDialogFactory()->createDataDialog(pComm);			
			if(dialog)
			{			
				pSocket->attchDialog( dialog );
				dialog->onCommunicatorSetup( pComm );
				if(!mDak.addnewCommunicator(pComm))
				{//failed to add communicator to Dak
					pComm->onCommunicatorClosed();
					continue;
				}
			}
			else
			{
				pComm->onCommunicatorClosed();
			}
		}
	}

	return 1;
}

int32 AServerSocketTcp::onDataAsyncResult(struct ::epoll_event& epollevent)
{
	if(mSock == -1 )
		return ERROR_CODE_OPERATION_FAIL;

#if defined _DEBUG || defined DEBUG
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(onDataAsyncResult,"Has event [%d]"),epollevent.events);
#endif

	if( (epollevent.events & EPOLLIN) == EPOLLIN ) //have data read
	{
		do{
		ASocket* pSocket = accept();
		if( pSocket == NULL )
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				break;
			//detect if we close the listen socket
			MLOG(ZQ::common::Log::L_ERROR,"AServerSocketTcp::run() accept function return failed");
			break;
		}
		else
		{
			IDataCommunicatorExPtr pComm = pSocket;
			pSocket->attachUserData(mUserData);
			IDataDialogPtr  dialog = mEnv.getDataDialogFactory()->createDataDialog(pComm);			
			if(dialog)
			{			
				pSocket->attchDialog( dialog );
				dialog->onCommunicatorSetup( pComm );
//				setNonBlock(pSocket->getCommunicatorDescriptor() );
				if(!mDak.addnewCommunicator(pComm))
				{//failed to add communicator to Dak
					pComm->onCommunicatorClosed();
				}
			}
			else
			{
				pComm->close();
			}
		}
		}while(1);
		return ERROR_CODE_OPERATION_OK;
	}
	else if((epollevent.events & EPOLLOUT) == EPOLLOUT)
	{
		return ERROR_CODE_OPERATION_OK;
	}
	else if((epollevent.events & EPOLLHUP) == EPOLLHUP || (epollevent.events & EPOLLERR) == EPOLLERR)
	{
		MLOG(ZQ::common::Log::L_INFO,"onDataAsyncResult() have a hang up or error event");
		close();
		clear();
		return ERROR_CODE_OPERATION_FAIL;

	}
	else
	{
		MLOG(ZQ::common::Log::L_ERROR,"onDataSsyncResult() dont known the event type[%d]",epollevent.events);
		close();
		clear();
		return ERROR_CODE_OPERATION_FAIL;
	}	

}


void AServerSocketTcp::final()
{
	//do nothing
}

void AServerSocketTcp::stop( )
{
	//how to close the server socket ?
	//just close it ?
	MLOG(ZQ::common::Log::L_INFO,"AserverSocketTcp::stop() stop the service");
	close();
	clear();
	waitHandle(100*1000);
}

//////////////////////////////////////////////////////////////////////////
///AClientSocketTcp
AClientSocketTcp::AClientSocketTcp( DataPostDak& dak , DataPostHouseEnv& env ,IDataDialogFactoryPtr fac , SharedObjectPtr userData)
:ASocket(env,dak),
mDak(dak),
mFac(fac),
mUserData(userData)
{
}
AClientSocketTcp::~AClientSocketTcp()
{

}

bool AClientSocketTcp::connectTo( const std::string& remoteIp , const std::string& remotePort , uint32 timeout, const std::string& localIp, const std::string& localPort )
{	
	if( !createSocket( remoteIp.find(":") != std::string::npos ? AF_INET6 : AF_INET , SOCK_STREAM , IPPROTO_TCP ) )
	{
		return false;
	}

	if (!localIp.empty())
	{	
	if( !this->bind(localIp ,localPort ) )
	{
		return false;
	}
	}
	if( !this->connect(remoteIp , remotePort , timeout ))
	{
		return false;
	}
	mType = COMM_TYPE_TCP;
	mCompletionKey.dataCommunicator = this;
	mCompletionKey.mStatus = true;

	initializeSockName();

	
	return true;
}
bool AClientSocketTcp::addToDak()
{
	ZQ::DataPostHouse::IDataDialogPtr dialog = mFac->createDataDialog(this);
	if(dialog)
	{
		attchDialog(dialog);
		attachUserData(mUserData);
		dialog->onCommunicatorSetup( this );
	}
	else
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(connectTo,"no dialog created for this communicator"));
		return false;
	}

	if( !mDak.addnewCommunicator(this) )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(ClientService,"failed to add communicator to dak"));
		return false;
	}
	readAsync();
	return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//AServerSocketUdp
AServerSocketUdp::AServerSocketUdp( DataPostDak& dak , DataPostHouseEnv& env )
:AUdpSocket(env,dak),
mDak(dak)
{
	mType = COMM_TYPE_UDP;
}
AServerSocketUdp::~AServerSocketUdp()
{
	close();	
}
bool AServerSocketUdp::startServer( const std::string& localIp , const std::string& localPort ,SharedObjectPtr userData )
{
	//step 1
	//create socket and bind it
	int family = (localIp.find(":") != std::string::npos) ? AF_INET6 :AF_INET;
	mAddrInfoHelper.init( family , SOCK_DGRAM , IPPROTO_UDP );
	if( !mAddrInfoHelper.convert( localIp , localPort ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(startServer,"can't get address information with peer[%s:%s] and errorCode[%d]"),
			localIp.c_str(), localPort.c_str() , errno );
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );
	if (!createSocket(adInfo->ai_family, SOCK_DGRAM , IPPROTO_UDP ))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"create server socket failed and errorCode [%d]"), errno );
		return false;
	}

	if(!bind(localIp,localPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to bind local address[%s][%s] and errorCode[%d]"),
			localIp.c_str() , localPort.c_str() , errno );
		return false;
	}

	mAddrLocal.addrLen = sizeof(mAddrLocal.u.storage);
	getsockname(mSock,(struct sockaddr*)&mAddrLocal.u.storage,(socklen_t*)&mAddrLocal.addrLen);

	mCompletionKey.dataCommunicator = this;
	mCompletionKey.mStatus = true;
	mUserData = userData;	

	mServeCommunicator = createCommunicator();	
	IDataDialogPtr  dialog = mEnv.getDataDialogFactory()->createDataDialog( mServeCommunicator );
	mServeCommunicator->mDataDialog = dialog;
	mDialog = IDgramDialogPtr::dynamicCast(dialog);
	mDataDialog = mDialog;
	if(!mDialog)
	{
		MLOG(ZQ::common::Log::L_ERROR,CLOGFMT(AServerSocketUdp,"startServer() want to get a IDgramDialogPtr instance but not, failed to start udp server"));
		return false;
	}

	mDak.addnewCommunicator( mCompletionKey.dataCommunicator );
	
	readAsync();

	return true;
}

void AServerSocketUdp::clear( )
{
	if( mDataDialog != 0 )
	{
		mDataDialog->onCommunicatorDestroyed(mServeCommunicator);
		mDataDialog = NULL;
		mServeCommunicator = NULL;
	}
	if( mDialog)
	{
		mDialog = NULL;
	}
// 	if( mCompletionKey.dataCommunicator )
// 		mCompletionKey.dataCommunicator = NULL;	
}

void AServerSocketUdp::stop( )
{
	//how to close the server socket ?
	//just close it ?
	mEnv.getDataDialogFactory()->releaseDataDialog(mDialog,mServeCommunicator);
	close();
	clear();
}
void AServerSocketUdp::close()
{
	if( mSock != INVALID_SOCKET )
	{
		assert( mType == COMM_TYPE_UDP);		
		//do not close socket here if the communicator is UDP
		//because UDP use a dummy socket 
		//::shutdown( mSock , SD_BOTH  );
		::close(mSock);
		mSock = INVALID_SOCKET;
		MLOG(ZQ::common::Log::L_INFO,COMMFMT(AServerSocketUdp,"close() close udp communicator [%lld]"), mId );
	}
}
void AServerSocketUdp::onCommunicatorClosed()
{	
}

AUdpSocketPtr AServerSocketUdp::createCommunicator( )
{
	//DO NOT permit to use WriteAsync in UDP mode
	AUdpSocket* p =  new AUdpSocket(mEnv, mDak );
	p->mAddrLocal	= mAddrLocal;
	p->mAddrRemote	= mAddrRemote;
	p->mId			= mId;//mEnv.generateConnectionId();
	p->mSock		= mSock;
	p->mType		= COMM_TYPE_UDP;
	p->mUserData	= mUserData;
	p->mDataDialog	= mDataDialog;
	return p;

}

int32 AServerSocketUdp::readAsync( )
{
	if(!isValid())
	{
		return ERROR_CODE_OPERATION_CLOSED;
	}

	memset(mReadBuf, 0, mReadBufLen);

	int dwFlags = MSG_DONTWAIT;
	int iRet = recvfrom( mSock , mReadBuf, mReadBufLen , dwFlags , 
							(sockaddr*)&mAddrRemote.u.storage , (socklen_t*)&mAddrRemote.addrLen );
	if( iRet == -1 )
	{
		if(EAGAIN == errno || EWOULDBLOCK == errno)
		{
			return ERROR_CODE_OPERATION_PENDING;
		}
		else
		{
			return ERROR_CODE_OPERATION_FAIL;
		}
	}
	else
	{
		return iRet;
	}
}

int32 AServerSocketUdp::onDataAsyncError( void )
{
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(AServerSocketUdp,"onDataAsyncError() got error[%d]"), errno );
	struct epoll_event evt;
	memset(&evt,0,sizeof(evt));
	evt.events = EPOLLIN;//dummy event
	onDataAsyncResult(evt);
	return ERROR_CODE_OPERATION_OK;
}

int32 AServerSocketUdp::onDataAsyncResult(struct ::epoll_event& epollevent ) 
{

	assert( mDataDialog != 0 );
#if defined _DEBUG || defined DEBUG
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(onDataAsyncResult,"Has event [%d]"),epollevent.events);
#endif

	if( (epollevent.events & EPOLLIN) == EPOLLIN)
	{
		//here comes a new data package
		//just create a new communicator
		do 
		{
			int32 iRet = readAsync();
			if( iRet > 0 )
			{//should never be here
				mServeCommunicator->mAddrRemote = mAddrRemote;
				IDataCommunicatorExPtr pComm = mServeCommunicator;
				try
				{
					mDialog->onData( mReadBuf, mReadBufLen, pComm );
				}
				catch( ... )
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when call dialog's onData()"));
				}				
			}
			else
			{
				if ( iRet == ERROR_CODE_OPERATION_PENDING )
				{
					return ERROR_CODE_OPERATION_OK;
				}
				else
				{
					MLOG(ZQ::common::Log::L_WARNING,COMMFMT(onDataAsyncResult,"Failed to invoke readAsync, error [%d]"),errno);
					//No communicator need to be closed
					return ERROR_CODE_OPERATION_FAIL;
				}
			}

		} while (true);
	}
	else if((epollevent.events & EPOLLOUT) == EPOLLOUT )
	{
		int32 size = 0;
		if(mWriteBuf.size())
		{
			size = writeAsync(mWriteBuf.c_str(),mWriteBuf.length());
			if(size < 0)
			{
				MLOG(ZQ::common::Log::L_ERROR,"onDataAsyncResult() writeAsync failed error string[%s]",strerror(errno));
				return ERROR_CODE_OPERATION_FAIL;
			}
		}
		return ERROR_CODE_OPERATION_OK;
	}
	else if((epollevent.events & EPOLLHUP) == EPOLLHUP || (epollevent.events & EPOLLERR) == EPOLLERR)
	{
		MLOG(ZQ::common::Log::L_INFO,"onDataAsyncResult() have hang up event");
		onCommunicatorClosed();
		return ERROR_CODE_OPERATION_FAIL;
		
	}
	else
	{
		MLOG(ZQ::common::Log::L_ERROR,"onDataAsyncResult() dont known the event type [%d]",epollevent.events);
		onCommunicatorClosed();
		return ERROR_CODE_OPERATION_FAIL;

	}

}

//////////////////////////////////////////////////////////////////////////
///AUdpSocket
AUdpSocket::AUdpSocket(DataPostHouseEnv& env, DataPostDak& dak)
:ASocket(env,dak)
{
}

int32 AUdpSocket::writeTo( const int8* buffer, size_t bufSize , const std::string& peerIp , const std::string& peerPort )
{
	addrInfoHelper helper;
	helper.init(PF_UNSPEC , SOCK_DGRAM , IPPROTO_UDP , AI_CANONNAME );
	if(!helper.convert(peerIp,peerPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(AUdpSocket,"writeTo() do not understand peer addr[%s:%s]"),
			peerIp.c_str() , peerPort.c_str() );
		return ERROR_CODE_OPERATION_FAIL;
	}
	addrinfo* info = helper.getAddrInfo();
	int rc = sendto( mSock , buffer , (int)bufSize , 0,
						info->ai_addr , (int)info->ai_addrlen);
	if( rc < 0  )
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AUdpSocket,"writeTo() failed with error[%d]"), errno );
		return ERROR_CODE_OPERATION_FAIL;
	}
	return rc;
}

int32 AUdpSocket::write(const int8 *buffer, size_t bufSize, int32 timeout )
{
	int rc = sendto(mSock , buffer , (int)bufSize , 0 , 
						(struct sockaddr*)&mAddrRemote.u.storage, (int)mAddrRemote.addrLen);
	if( rc < 0 )
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AUdpSocket,"write() failed with error[%d]"),  errno );
		return ERROR_CODE_OPERATION_FAIL;
	}
	return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////
TcpCommunicatorSettings::TcpCommunicatorSettings( IDataCommunicatorPtr comm  )
{
	mSock = -1;
	if( comm )
	{
		IDataCommunicatorExPtr c = IDataCommunicatorExPtr::dynamicCast(comm);
		if( c )
		{
			mSock = c->getCommunicatorDescriptor();
		}
	}
}
TcpCommunicatorSettings::~TcpCommunicatorSettings()
{
}
void TcpCommunicatorSettings::attachCommunicator( IDataCommunicatorPtr comm  )
{
	if( comm )
	{
		IDataCommunicatorExPtr c = IDataCommunicatorExPtr::dynamicCast(comm);
		if( c )
		{
			mSock = c->getCommunicatorDescriptor();
		}
	}
}

bool TcpCommunicatorSettings::nodelay( bool bNodelay )
{
	if(mSock < 0 )
		return false;
	
	int value = bNodelay ? 1 : 0;

	return 0 == setsockopt( mSock , IPPROTO_TCP , TCP_NODELAY , (const char*)&value , sizeof(value));
}

bool TcpCommunicatorSettings::holdon( bool bHoldOn )
{
	if(mSock < 0 )
		return false;
		
	int value = bHoldOn ? 1 : 0;
		
	return 0 == setsockopt( mSock , IPPROTO_TCP , TCP_CORK , (const char*)&value , sizeof(value));	
}

bool TcpCommunicatorSettings::setWriteBufSize( size_t sz )
{
	if(mSock < 0 )
		return false;

	int sendBuf = (int)sz;

	return 0 == setsockopt( mSock, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBuf , sizeof(sendBuf) );
}
bool TcpCommunicatorSettings::setReadBufSize( size_t sz )
{
	if(mSock < 0 )
		return false;
	int recvBuf = (int)sz;
	return 0 == setsockopt( mSock, SOL_SOCKET, SO_RCVBUF, (const char*)&recvBuf , sizeof(recvBuf) );
}

bool TcpCommunicatorSettings::setSendTimeout( uint32 to )
{
	if(mSock < 0 )
		return false;
	struct timeval timeo;
	timeo.tv_sec = (int)(to/1000);
	timeo.tv_usec = (int)(to%1000)*1000	;
	return 0 == setsockopt(mSock, SOL_SOCKET, SO_SNDTIMEO,(const char*)&timeo,sizeof(timeo));
}

}}//namespace ZQ::DataPostHouse

