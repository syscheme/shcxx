
#include "DataCommunicatorUnite.h"
#include "TimeUtil.h"
#include <MSWSock.h>

#define MLOG mEnv.getLogger()
#define COMMFMT(x,y) "COMM[%lld]TID[%08X][%10s]\t"##y,mId,GetCurrentThreadId(),#x

namespace ZQ
{
namespace DataPostHouse
{

std::string getSocketErrorstring( int err )
{
	std::string strRet;
	char buffer[4096];buffer[0] = 0 ; buffer[sizeof(buffer)-1] = 0;

	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0,err,0,buffer,sizeof(buffer)-1,0);
	strRet = buffer;
	return strRet;
}

ASocket::ASocket( DataPostHouseEnv& env , DataPostDak& dak)
:mSock(INVALID_SOCKET ),
mType(COMM_TYPE_NULL),
mUserData(NULL),
mDataDialog(NULL),
mEnv(env),
mDak(dak)
{
	internalInitialize();
}
ASocket::ASocket( DataPostHouseEnv& env , DataPostDak& dak, const SOCKET& s , const CommunicatorType& type , SharedObjectPtr userData  )
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
	mReadBuffer.buf	=	(char*)malloc( mEnv.getReadBufferSize() );
	assert( mReadBuffer.buf != NULL );
	mReadBuffer.len	=	mEnv.getReadBufferSize();
	//MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(ASocket,"communicator [%lld] is setup"),mId);

	updateActiveStamp();
}

ASocket::~ASocket( )
{	
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(ASocket,"communicator [%lld] is destroyed"),mId);	
	if( mReadBuffer.buf )
	{
		free( mReadBuffer.buf );
	}	
}


bool ASocket::createSocket( int family, int type , int protocol )
{
	mSock	= WSASocket(family,type,protocol,NULL,0,WSA_FLAG_OVERLAPPED);
	if( mSock == INVALID_SOCKET )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(createSocket,"createSocket failed with family[%d] type[%d] protocol[%d] and errorCode[%u]"),
			family , type , protocol , WSAGetLastError() );		
		return false;
	}
	return true;
}

bool ASocket::bind( const std::string& localIp , const std::string& localPort )
{
	assert( mSock != INVALID_SOCKET );
	if( !mAddrInfoHelper.convert(localIp,localPort) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(bind,"can't get address information with peer[%s:%s] and errorCode[%u]"),
			localIp.c_str() , localPort.c_str()	,WSAGetLastError() );
		return false;
	}
	
	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );

	int iRet = ::bind( mSock, adInfo->ai_addr ,(int)adInfo->ai_addrlen );
	if( iRet ==  SOCKET_ERROR )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(ASocket , "failed to bind socket with peer[%s:%s] and errorCode[%u]" ),
			localIp.c_str() , localPort.c_str() , WSAGetLastError()	);
		return false;
	}
	return true;
}

bool ASocket::listen( int backLog /* = 100 */ )
{
	assert( mSock != INVALID_SOCKET );
	int iRet = ::listen( mSock , backLog );
	if( iRet == SOCKET_ERROR )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(listen,"failed to invoke listen and errorCode[%u]"),
			WSAGetLastError());
		return false;
	}
	return true;
}

ASocket* ASocket::accept()
{
	assert( mSock != INVALID_SOCKET );

	sockaddr_storage addr;
	int addLen = sizeof(addr);

	SOCKET s = ::accept( mSock, (sockaddr*)&addr , &addLen );
	if( s == INVALID_SOCKET )
	{
		if( WSAGetLastError() !=  10004 )
		{
			MLOG(ZQ::common::Log::L_ERROR,
				COMMFMT(accept,"accept failed and errorCode[%u]"),
				WSAGetLastError());
		}
		else
		{//listen socket is closed
			MLOG(ZQ::common::Log::L_INFO,
				COMMFMT(accept,"listen socket is closed. errorCode[%u]"),
				WSAGetLastError());
		}
		return NULL;
	}
	else
	{
		ASocket* pSocket = new ASocket( mEnv , mDak, s );	
		pSocket->initializeSockName();			
		pSocket->mType = COMM_TYPE_TCP;
		pSocket->mCompletionKey.dataCommunicator = pSocket;
		pSocket->mCompletionKey.mStatus = true;
		assert( pSocket != NULL );
		return pSocket;
	}
}

void ASocket::initializeSockName( )
{
	mAddrRemote.addrLen = sizeof(mAddrRemote.u.storage );
	if( getpeername( mSock, (sockaddr*)&mAddrRemote.u , &mAddrRemote.addrLen ) == SOCKET_ERROR  )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(initializeSockName,"can't get remote addr"));
	}

	mAddrLocal.addrLen = sizeof( mAddrLocal.u.storage );
	if( getsockname( mSock , (sockaddr*)&mAddrLocal.u , &mAddrLocal.addrLen ) == SOCKET_ERROR  )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(initializeSockName,"can't get local addr"));
	}
}
bool ASocket::connect( const std::string& remoteIp , const std::string& remotePort ,uint32 timeout )
{
	assert( mSock != INVALID_SOCKET );
	if( !mAddrInfoHelper.convert( remoteIp , remotePort ) )
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(connect,"can't get address information with peer[%s:%s] and errorCode[%u]"),
			remoteIp.c_str(), remotePort.c_str() , WSAGetLastError() );
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );
	bool bCheckTimeout = ( timeout != (uint32)-1 );
	
		
	if( bCheckTimeout)
	{
		//set to nonblock mode
		int iMode = 1;
		ioctlsocket(mSock, FIONBIO, (u_long FAR*) &iMode);
	}
	
	int iRet = ::connect( mSock , adInfo->ai_addr , (int)adInfo->ai_addrlen );
	if( iRet == SOCKET_ERROR )
	{
		if( ! ( bCheckTimeout && WSAGetLastError() == WSAEWOULDBLOCK ) )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to connect peer [%s:%s] and errorCode[%u]"),
				remoteIp.c_str() , remotePort.c_str() , WSAGetLastError() );
			return false;
		}
	}
	if( bCheckTimeout )
	{
		struct   timeval   t ;  
		fd_set   r;  

		FD_ZERO(&r);  
		FD_SET( mSock,   &r);  
		t.tv_sec	= timeout/1000;
		t.tv_usec	= timeout%1000*1000;
		int ret   =   ::select( (int)(mSock + 1),   0,   &r,   0,   &t); 
		if( ret < 0 )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to detect socket status, error[%u]"), WSAGetLastError() );
			return false;
		}
		int so_error;
		socklen_t len = sizeof so_error;

		if( getsockopt(mSock, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len) < 0 )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(connect,"failed to connect to peer[%s:%s],error[%u]"),remoteIp.c_str() , remotePort.c_str() , WSAGetLastError() );
			return false;
		}
		//reset to block mode
		int iMode = 0;
		ioctlsocket(mSock, FIONBIO, (u_long FAR*) &iMode);
	}

	return true;
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
	return ( mSock != INVALID_SOCKET );
}

SOCKET	ASocket::getCommunicatorDescriptor( ) 
{
	return mSock;
}

DataCompletionKey* ASocket::getCompletionKey( )
{
	return &mCompletionKey;
}

int32 ASocket::read( int8* buffer , size_t bufSize ,int32 timeoutInterval /* = -1 */ )
{
	updateActiveStamp();

	assert( mSock != INVALID_SOCKET );
	assert( buffer != NULL );
	assert( bufSize != 0 );

	if( timeoutInterval <= 0 )
	{
		int iRet = ::recv( mSock , reinterpret_cast<char*>(buffer) , static_cast<int>(bufSize) , 0 );
		if( iRet == SOCKET_ERROR  )
		{
			MLOG(ZQ::common::Log::L_ERROR,
				COMMFMT(read,"failed to invoke recv and errorCode[%u]"),
				WSAGetLastError());
			return ERROR_CODE_OPERATION_FAIL;
		}
		return static_cast<int32>(iRet);
	}
	else
	{
		struct  timeval selectTime;	
		selectTime.tv_sec	=	timeoutInterval / 1000;
		selectTime.tv_usec	=	(timeoutInterval % 1000 ) * 1000;
		fd_set	readSet;
		FD_ZERO(&readSet);
		FD_SET(mSock,&readSet);
		
		int iRet = 0;

		iRet = ::select( 1 , &readSet , NULL , NULL,&selectTime );
		if( SOCKET_ERROR == iRet )
		{
			MLOG(ZQ::common::Log::L_ERROR,
				COMMFMT(read,"failed to invoke select and errorCode[%u]"),
				WSAGetLastError() );
			return ERROR_CODE_OPERATION_FAIL;
		}
		else
		{			
			if( iRet != 1 )
			{
				MLOG(ZQ::common::Log::L_ERROR,
					COMMFMT(read,"failed to invoke select and errorCode[%u] and return value [%d]"),
					WSAGetLastError() ,iRet );
				return ERROR_CODE_OPERATION_FAIL;
			}
			else
			{
				if( FD_ISSET(mSock,&readSet))
				{
					iRet = ::recv( mSock , reinterpret_cast<char*>(buffer) , static_cast<int>(bufSize) , 0 );
					if( iRet == SOCKET_ERROR  )
					{
						MLOG(ZQ::common::Log::L_ERROR,
							COMMFMT(read,"failed to invoke recv and errorCode[%u]"),
							WSAGetLastError() );
						return ERROR_CODE_OPERATION_FAIL;
					}
					else
					{
						return iRet;
					}
				}
				else
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(read,"should not be here !"));
					return ERROR_CODE_OPERATION_FAIL;
				}
			}
		}
	}
}

int32 ASocket::readAsync( )
{
// 	if( !isValid() )
// 	{
// 		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(readAsync,"failed to invoke WSARecv becasue communicator is not valid")  );
// 		return ERROR_CODE_OPERATION_FAIL;
// 	}
	
	updateActiveStamp();

	DWORD dwReadByte = 0;
	DWORD dwFlag = 0;
	
	memset( &mReadOverlapped , 0 ,sizeof(mReadOverlapped) );
	
	assert( mReadBuffer.len == mEnv.getReadBufferSize() );
	int iRet = WSARecv( mSock , &mReadBuffer , 1 , &dwReadByte , &dwFlag , &mReadOverlapped, NULL );
	
	assert( iRet == 0 || iRet == SOCKET_ERROR );	

	if( iRet == SOCKET_ERROR  )
	{//it's OK
		if( WSAGetLastError() == WSA_IO_PENDING )
		{
			return ERROR_CODE_OPERATION_PENDING;
		}
		else
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(readAsync,"failed to invoke WSARecv, errorCode[%u]"),WSAGetLastError() );
			return ERROR_CODE_OPERATION_FAIL;
		}
	}
	else
	{
		return ERROR_CODE_OPERATION_PENDING;		
	}
}

int32 ASocket::write( const int8* buffer , size_t bufSize , int32 timeoutInterval /* = -1 */ )
{
	if( !isValid() )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"invalid communicator, reject writing") );
		return ERROR_CODE_OPERATION_FAIL;
	}
	
	updateActiveStamp();

	assert( buffer != NULL );
	assert( bufSize != 0 );
	if ( timeoutInterval <= 0 )
	{
		int iRet = ::send( mSock , buffer , (int) bufSize , 0 );
		if( iRet == SOCKET_ERROR )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke send, errorCode[%u/%s]"),
				WSAGetLastError() , getSocketErrorstring( WSAGetLastError() ).c_str()  );
			return ERROR_CODE_OPERATION_FAIL;
		}
		else
		{
			return iRet ;
		}
	}
	else
	{
		struct  timeval selectTime;	
		selectTime.tv_sec	=	timeoutInterval / 1000;
		selectTime.tv_usec	=	(timeoutInterval % 1000 ) * 1000;

		fd_set	writeSet;
		FD_ZERO( &writeSet );
		FD_SET(mSock,&writeSet);
		int iRet = ::select( 1 , NULL , &writeSet , NULL , &selectTime );
		if( iRet != 1)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke send, timed out") );
			return ERROR_CODE_OPERATION_TIMEOUT;
		}
		else
		{
			if( FD_ISSET(mSock,&writeSet))
			{
				iRet = ::send( mSock , buffer , (int)bufSize , 0 );
				if( iRet == SOCKET_ERROR )
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(write,"failed to invoke send, errorCode[%u/%s]"),
						WSAGetLastError() , getSocketErrorstring( WSAGetLastError() ).c_str()  );
					return ERROR_CODE_OPERATION_FAIL;
				}
				else
				{
					return iRet ;
				}
			}
			else
			{
				return ERROR_CODE_OPERATION_TIMEOUT;
			}
		}
	}
}

int32 ASocket::writeAsync( const int8* buffer , size_t bufSize )
{
	if( !isValid() )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(writeAsync,"invalid communicator, reject witing in async way") );
		return ERROR_CODE_OPERATION_FAIL;
	}
	
	updateActiveStamp();

	assert( buffer != NULL );
	assert( bufSize != 0 );
	mWriteBuffer.buf = const_cast<char*>(buffer);
	mWriteBuffer.len = static_cast<unsigned long>(bufSize);
	DWORD dwSent = 0;
	DWORD dwFlag = 0;

	memset(&mWriteOverlapped, 0, sizeof(mWriteOverlapped) );
	int iRet = WSASend( mSock , &mWriteBuffer , 1 , &dwSent , dwFlag , &mWriteOverlapped , NULL );
	if( iRet == SOCKET_ERROR )
	{
		if( WSAGetLastError() == WSA_IO_PENDING )
		{
			return ERROR_CODE_OPERATION_OK;
		}
		else
		{
			return ERROR_CODE_OPERATION_FAIL;
		}
	}
	else
	{
		return ERROR_CODE_OPERATION_OK ;
	}
}

void ASocket::clear()
{
	if( mDataDialog != NULL )
	{
		mDataDialog->onCommunicatorDestroyed(this);
		mDataDialog = NULL;
	}	
	if( mCompletionKey.dataCommunicator )
		mCompletionKey.dataCommunicator = NULL;	
}

void ASocket::close()
{
	if( mSock != INVALID_SOCKET )
	{
		SOCKET tmpSocket = mSock;
		mSock = INVALID_SOCKET;
		MLOG(ZQ::common::Log::L_INFO,COMMFMT(ASocket,"closed communicator"));
		if(mType != COMM_TYPE_UDP)
		{//do not close socket here if the communicator is UDP
			//because UDP use a dummy socket			
			::shutdown( tmpSocket , SD_BOTH  );
			::closesocket(tmpSocket);			
		}		
	}
}

int32 ASocket::onDataAsyncError()
{
	//MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncError,"communicator error[%u][%u]"), GetLastError() ,WSAGetLastError() );
	assert(mDataDialog != NULL );
	mDataDialog->onError();
	onCommunicatorClosed();
	return ERROR_CODE_OPERATION_OK;
}
void ASocket::onCommunicatorClosed( ) 
{	
	close( );//close socket and inform dialog the communicator is closed
	assert( mEnv.getDataDialogFactory() != NULL );	
	clear( );
	mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
}

int32 ASocket::onDataAsyncResult( int32  size , LPOVERLAPPED overlap )
{	
	//normal behavior
	assert( HasOverlappedIoCompleted(&mReadOverlapped) || HasOverlappedIoCompleted(&mWriteOverlapped) );	
#if defined _DEBUG || defined DEBUG
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(onDataAsyncResult,"received data with size[%d]"),size);
#endif
	if( overlap == &mReadOverlapped )
	{//read complete
		bool bRet = false;
		try
		{
			bRet = mDataDialog->onRead( mReadBuffer.buf , static_cast<size_t>(size) );
		}
		catch(...)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when call dialog's onRead()"));
			//should I return directly here ?
		}

		if(!isValid() )
		{//socket is closed
			onCommunicatorClosed( );
			return ERROR_CODE_OPERATION_OK;
		}
		if(!bRet )
		{
#pragma message(__MSGLOC__"Just return here ???")
			return ERROR_CODE_OPERATION_OK;
		}

		do 
		{
			int32 iRet = readAsync();
			if( iRet > 0 )
			{//never reach here
				try
				{
					mDataDialog->onRead( mReadBuffer.buf , static_cast<size_t>(iRet) );
				}
				catch(...)
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when call dialog's onRead()"));			
				}
			}
			else
			{
				if ( iRet == ERROR_CODE_OPERATION_PENDING  )
				{
					return ERROR_CODE_OPERATION_OK;
				}
				else
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"Failed to invoke readAsync"));
					onCommunicatorClosed();					
					return ERROR_CODE_OPERATION_FAIL;
				}
			}

		} while (true);
	}
	else
	{//write complete
		try
		{
			mDataDialog->onWritten( static_cast<size_t>(size) );
		}
		catch(...)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when invoke dialog's onWritten"));
			return ERROR_CODE_OPERATION_FAIL;
		}
		return ERROR_CODE_OPERATION_OK;
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

void getConnectionInfoFromAddr( const CommAddr& addr, std::string& ip, std::string& prt )
{
	char name[256]; name[0] = 0; name[sizeof(name)-1] = 0;
	char port[256]; port[0] = 0; port[sizeof(port)-1] = 0;
	int rc = getnameinfo( (struct sockaddr*)&addr.u.storage , addr.addrLen,
						name,sizeof(name)-1,
						port,sizeof(port)-1,
						NI_NUMERICHOST| NI_NUMERICSERV );
	if( rc != 0 )
	{
		return;
	}
	ip = name;
	prt = port;
}

void ASocket::getLocalAddress(std::string& strLocalIP, std::string& strLocalPort) const
{
	getConnectionInfoFromAddr( mAddrLocal , strLocalIP , strLocalPort );
}

void ASocket::getRemoteAddress(std::string& strRemoteIP, std::string& strRemotePort) const 
{
	getConnectionInfoFromAddr( mAddrRemote , strRemoteIP , strRemotePort );
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
									int32 maxBacklog /* = 100 */ )
{
	//step 1
	//create socket and bind it
	int family = (localIp.find(":") != std::string::npos) ? AF_INET6 :AF_INET;
	mAddrInfoHelper.init( family , SOCK_STREAM , IPPROTO_TCP);
	if( !mAddrInfoHelper.convert( localIp , localPort ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,
			COMMFMT(startServer,"can't get address information with peer[%s:%s] and errorCode[%u]"),
			localIp.c_str(), localPort.c_str() , WSAGetLastError() );
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );
	if (!createSocket( adInfo->ai_family, SOCK_STREAM , IPPROTO_TCP ) )
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"create server socket failed and errorCode [%u]"), WSAGetLastError() );
		return false;
	}
	
	if(!bind(localIp,localPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to bind local address[%s][%s] and errorCode[%u]"),
			localIp.c_str() , localPort.c_str() , WSAGetLastError() );
		return false;
	}

	if(!listen(maxBacklog))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to listen and errorCode[%u]"),WSAGetLastError() );
		return false;
	}
	mbRunning = true;
	mUserData = userData;	
	return start();
}
bool AServerSocketTcp::init()
{
	return true;
}

int AServerSocketTcp::run()
{
	while (mbRunning)
	{
		ASocket* pSocket = accept();
		if( pSocket == NULL )
		{
			MLOG(ZQ::common::Log::L_INFO,CLOGFMT(AServerSocketTcp,"failed to call accept(), error[%u]"),
				WSAGetLastError() );
			//detect if we close the listen socket
			continue;
		}
		else
		{
			IDataCommunicatorExPtr pComm = pSocket;
			pSocket->attachUserData(mUserData);
			IDataDialogPtr  dialog = mEnv.getDataDialogFactory()->createDataDialog(pComm);			
			if(dialog)
			{			
				pSocket->attchDialog( dialog );				
				pSocket->attachUserData(mUserData);
				dialog->onCommunicatorSetup( pComm );
				if(!mDak.addnewCommunicator( pComm ))
				{//failed to add communicator to Dak
					MLOG(ZQ::common::Log::L_ERROR,
						CLOGFMT(AServerSocketTcp,"failed to add communicator [%lld] to DAK , error is[%u]"),
						pComm->getCommunicatorId() , GetLastError() );
					pComm->onCommunicatorClosed();
					continue;
				}				
				int32 iRet = pSocket->readAsync( );
				if( iRet == ERROR_CODE_OPERATION_CLOSED )
				{
					MLOG(ZQ::common::Log::L_ERROR,CLOGFMT(AServerSocketTcp,
						"failed to call readAsync for communicator [%lld] and error [%u] , ERROR_CODE_OPERATION_CLOSED"),
						pComm->getCommunicatorId() , WSAGetLastError() );
					pComm->onCommunicatorClosed();					
				}
				else if (iRet < 0 )
				{
					if ( iRet != ERROR_CODE_OPERATION_PENDING )
					{
						MLOG(ZQ::common::Log::L_ERROR,CLOGFMT(AServerSocketTcp,
							"failed to call readAsync for communicator [%lld] and error [%u]"),
							pComm->getCommunicatorId() , WSAGetLastError() );
						pComm->onDataAsyncError();
					}					
				}
			}
			else
			{
				MLOG(ZQ::common::Log::L_ERROR,
					CLOGFMT(AServerSocketTcp,"failed to create dialog for communicator [%lld]"),
					pComm->getCommunicatorId()	);
				pComm->close();
			}
		}
	}

	return 1;
}

void AServerSocketTcp::final()
{
	//do nothing
}

void AServerSocketTcp::stop( )
{
	mbRunning = false;
	//how to close the server socket ?
	//just close it ?
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
	/*
	作为一个tcp客户端，首先需要创建一个socket并且连接到服务器，
	然后将自己添加到DataDak里面，最后需要调用read来触发数据通信
	*/
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

bool AServerSocketUdp::fixupUdpSocket()
{
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	DWORD status = 0;

	// disable  new behavior using
	// IOCTL: SIO_UDP_CONNRESET
	status = WSAIoctl(mSock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
						NULL, 0, &dwBytesReturned,	NULL, NULL);

	if (SOCKET_ERROR == status)
	{
		DWORD dwErr = WSAGetLastError();
		if (WSAEWOULDBLOCK == dwErr)
		{
			// nothing to do
			return false;
		}
		else
		{
			//printf("WSAIoctl(SIO_UDP_CONNRESET) Error: %d\n", dwErr);
			return false;
		}
	}
	return true;
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
			COMMFMT(startServer,"can't get address information with peer[%s:%s] and errorCode[%u]"),
			localIp.c_str(), localPort.c_str() , WSAGetLastError() );
		return false;
	}

	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert( adInfo != NULL );
	if (!createSocket(adInfo->ai_family, SOCK_DGRAM , IPPROTO_UDP ))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"create server socket failed and errorCode [%u]"), WSAGetLastError() );
		return false;
	}

	fixupUdpSocket();

	if(!bind(localIp,localPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer,"failed to bind local address[%s][%s] and errorCode[%u]"),
			localIp.c_str() , localPort.c_str() , WSAGetLastError() );
		return false;
	}

	mAddrLocal.addrLen = sizeof(mAddrLocal.u.storage);
	getsockname(mSock,(struct sockaddr*)&mAddrLocal.u.storage,&mAddrLocal.addrLen);

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
	if( mDataDialog != NULL )
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
		::closesocket(mSock);
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
	AUdpSocket* p =  new AUdpSocket(mEnv,mDak);
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
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AServerSocketUdp,"readAsync() communicator is closed, refuse to read data from it"));
		return ERROR_CODE_OPERATION_CLOSED;
	}
	while( true)

	{
		DWORD dwReadByte = 0;
		DWORD dwFlags = 0;

		mAddrRemote.addrLen = sizeof( mAddrRemote.u.storage );

		memset( &mReadOverlapped , 0 , sizeof(mReadOverlapped) );
		mReadBuffer.len = mEnv.getReadBufferSize();//is this a must ?
		int iRet =WSARecvFrom( mSock , &mReadBuffer , 1 ,
			&dwReadByte , &dwFlags , 
			(sockaddr*)&mAddrRemote.u.storage ,&mAddrRemote.addrLen ,
			&mReadOverlapped,NULL);

		MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(AServerSocketUdp,"readAsync() got return value %d"),iRet);

		if( iRet == SOCKET_ERROR || iRet < 0 )
		{		
			if ( WSAGetLastError() == WSA_IO_PENDING )
			{
				MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(AServerSocketUdp,"readAsync() successfully scheduled communicator in IOCP, current pending "));
				return ERROR_CODE_OPERATION_PENDING;
			}
			else if( WSAGetLastError() == WSAECONNRESET )
			{
				MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AServerSocketUdp,"readAsync() got error [%d], continue queueing the read request"),
					WSAGetLastError() );
				continue;
			}
			else
			{
				MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AServerSocketUdp,"readAsync() failed to read data, error[%d]"), WSAGetLastError() );
				return ERROR_CODE_OPERATION_FAIL;
			}
		}
		else if( iRet == 0 )
		{
			MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(AServerSocketUdp,"readAsync() successfully scheduled communicator in IOCP, current signalled "));
			return ERROR_CODE_OPERATION_PENDING;		
		}
		else
		{
			return iRet;
		}
	}
}

int32 AServerSocketUdp::onDataAsyncError( void )
{
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(AServerSocketUdp,"onDataAsyncError() got error[%d]"), WSAGetLastError() );
	onDataAsyncResult(0,0);
	return ERROR_CODE_OPERATION_OK;
}

int32 AServerSocketUdp::onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) 
{
	if( overlap == &mReadOverlapped && size > 0 )
	{		
		mServeCommunicator->mAddrRemote = mAddrRemote;
		IDataCommunicatorExPtr pComm = mServeCommunicator;

		try
		{
			mDialog->onData(mReadBuffer.buf, size , pComm );
		}
		catch( ... )
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when call dialog's onData()"));
		}
	}
	else
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AServerSocketUdp,"onDataAsyncResult() invalid overlap address passed in"));
	}

	do 
	{
		int32 iRet = readAsync();
		if( iRet > 0 )
		{//should never be here
			mServeCommunicator->mAddrRemote = mAddrRemote;
			IDataCommunicatorExPtr pComm = mServeCommunicator;
			try
			{
				mDialog->onData( mReadBuffer.buf, size , pComm );
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
				MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"Failed to invoke readAsync,err[%d]"), WSAGetLastError());
				//Should I close server udp socket here ?
				return ERROR_CODE_OPERATION_FAIL;
			}
		}

	} while (true);	
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
	if( rc == SOCKET_ERROR )
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AUdpSocket,"writeTo() failed with error[%u]"), WSAGetLastError() );
		return ERROR_CODE_OPERATION_FAIL;
	}
	return rc;
}

int32 AUdpSocket::write(const int8 *buffer, size_t bufSize, int32 timeout )
{
	int rc = sendto(mSock , buffer , (int)bufSize , 0 , 
						(struct sockaddr*)&mAddrRemote.u.storage, (int)mAddrRemote.addrLen);
	if( rc == SOCKET_ERROR )
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(AUdpSocket,"write() failed with error[%u]"), WSAGetLastError() );
		return ERROR_CODE_OPERATION_FAIL;
	}
	return rc;
}


//////////////////////////////////////////////////////////////////////////

TcpCommunicatorSettings::TcpCommunicatorSettings( IDataCommunicatorPtr comm /* = NULL */ )
:mSock(-1)
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
	//setsockopt( mSock , IPPROTO_TCP , TCP_CORK , (const char*)&value , sizeof(value));
	return true;
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
//SO_SNDTIMEO
bool TcpCommunicatorSettings::setSendTimeout( uint32 to )
{
	if(mSock < 0 )
		return false;
	int timeo = (int)to;
	return 0 == setsockopt(mSock, SOL_SOCKET, SO_SNDTIMEO,(const char*)&timeo,sizeof(timeo));
}

}}//namespace ZQ::DataPostHouse
