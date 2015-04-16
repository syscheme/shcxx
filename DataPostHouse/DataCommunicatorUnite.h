
#ifndef _zq_data_posthouse_data_communicator_implement_unite_header_file_h__
#define _zq_data_posthouse_data_communicator_implement_unite_header_file_h__

#include "DataPostHouseEnv.h"
#include "DataCommunicatorEx.h"
#include <NativeThread.h>
#include <SystemUtils.h>

#ifdef ZQ_OS_MSWIN
#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include "DataPostHouseWin.h"
#include <Ws2tcpip.h>
#else
#include "DataPostHouseLinux.h"
extern "C"
{
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>
}
#endif


namespace ZQ
{
namespace DataPostHouse
{
#ifdef ZQ_OS_LINUX
	#ifndef SOCKET
	#	define SOCKET int
	#endif
	#ifndef INVALID_SOCKET
	#	define INVALID_SOCKET -1
	#endif//INVALID_SOCKET
#endif//ZQ_OS_LINUX

class addrInfoHelper
{
public:
	addrInfoHelper( )
	{
		memset( &mAddrHint , 0 ,sizeof(mAddrHint) );
		mpAddrInfo = NULL;
	}
	~addrInfoHelper( )
	{
		freeResource( );
	}
	void			init( int family , int type , int protocol, int flag = AI_CANONNAME|AI_PASSIVE )
	{
		mAddrHint.ai_family		=	family;
		mAddrHint.ai_socktype	=	type;
		mAddrHint.ai_protocol	=	protocol;
		mAddrHint.ai_flags		=	flag;
	}
public:
	
	bool			convert( const std::string& ip , const std::string& port  )
	{
		freeResource();
		int rc = getaddrinfo( ip.c_str() , port.c_str() , &mAddrHint , &mpAddrInfo);
		if ( rc != 0 ) 
		{
			mpAddrInfo = NULL;
			return false;
		}

		return true;			
	}
	
	addrinfo*		getAddrInfo( ) const
	{
		return mpAddrInfo;
	}

	void			freeResource( )
	{
		if( mpAddrInfo )
		{
			freeaddrinfo( mpAddrInfo );
			mpAddrInfo = NULL;
		}
	}	

private:
	addrinfo*	mpAddrInfo;
	addrinfo	mAddrHint;
};



class ASocket :public IDataCommunicatorEx
{
	friend class AServerSocketUdp;
public:
	ASocket( DataPostHouseEnv& env , DataPostDak& dak);	
	ASocket( DataPostHouseEnv& env , DataPostDak& dak, const SOCKET& s , const CommunicatorType& type = COMM_TYPE_NULL , SharedObjectPtr userData = NULL );
	virtual ~ASocket( );
public:
	virtual		CommunicatorType	getCommunicatorType( ) const ;
	virtual		bool				isValid( ) const ;
	virtual		int64				getCommunicatorId( ) const ;
	virtual		bool				getCommunicatorAddrInfo( CommAddr& local , CommAddr& remote ) const;
	virtual		int32				read( int8* buffer , size_t bufSize ,int32 timeout = -1 ) ;
	virtual		int32				readAsync(  ) ;
	virtual		int32				write( const int8* buffer , size_t bufSize , int32 timeout = -1 ) ;
	virtual		int32				writeAsync( const int8* buffer , size_t bufSize ) ;
	virtual		void				close( ) ;
	virtual		IDataDialogPtr		getDataDialog( ) ;
	virtual		SharedObjectPtr		getUserData( ) ;
	virtual     void                getLocalAddress(std::string& localIP, std::string& localPort) const;
	virtual     void                getRemoteAddress(std::string& remoteIP, std::string& remotePort) const;


#ifdef ZQ_OS_MSWIN	
	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) ;
	virtual		SOCKET				getCommunicatorDescriptor( ) ;
#else
	virtual		int32				onDataAsyncResult(struct ::epoll_event& epollevent);
	virtual		int					getCommunicatorDescriptor( ) ;
	virtual		int					setNonBlock(int sockfd);
#endif

	virtual		int32				onDataAsyncError( void );
	virtual		DataCompletionKey*	getCompletionKey( ) ;
	virtual		void				onCommunicatorClosed( ) ;
	virtual		void				attchDialog( IDataDialogPtr dialog ) ;
	virtual		void				attachUserData( SharedObjectPtr userData );

public:

	bool							createSocket( int family, int type , int protocol );

	bool							bind( const std::string& localIp , const std::string& localPort );	

	bool							listen( int backLog = 2000 ); 

	ASocket*						accept( );
	
	bool							connect( const std::string& remoteIp , const std::string& remotePort , uint32 timeout = (uint32)-1);
	
	// return the milliseconds
	virtual uint32					getIdleTime();

protected:
	void							updateActiveStamp();

	void							clear();

	void							internalInitialize( );
	
	void							initializeSockName( );

protected:

	SOCKET							mSock;
	int64							mId;	
	CommunicatorType				mType;
	SharedObjectPtr					mUserData;
	IDataDialogPtr					mDataDialog;	
	addrInfoHelper					mAddrInfoHelper;

#ifdef ZQ_OS_MSWIN
	WSAOVERLAPPED					mReadOverlapped;
	WSABUF							mReadBuffer;

	WSAOVERLAPPED					mWriteOverlapped;
	WSABUF							mWriteBuffer;
	
#else
	char*							mReadBuf;
	size_t							mReadBufLen;
	std::string						mWriteBuf;
#endif

	int64							_lastActiveTime;

	DataCompletionKey				mCompletionKey;
	DataPostHouseEnv&				mEnv;
	DataPostDak&					mDak;
	
	CommAddr						mAddrLocal;
	CommAddr						mAddrRemote;
	ZQ::common::Mutex				mLocker;
};

typedef ObjectHandle<ASocket>	ASocketPtr;


class AServerSocketTcp : public ASocket , public ZQ::common::NativeThread
{
public:

	AServerSocketTcp( DataPostDak& dak , DataPostHouseEnv& env );

	virtual ~AServerSocketTcp( );

public:
	
	bool		startServer(  const std::string& localIp  , const std::string& localPort ,  SharedObjectPtr userData = NULL , int32 maxBacklog = 2000 );	
#ifdef ZQ_OS_LINUX
	virtual		int32				onDataAsyncResult(struct ::epoll_event& epollevent);
#endif
	void		stop( );

protected:

	bool		init(void);
	int			run(void);
	void		final(void);

private:
	DataPostDak&		mDak;	
	bool				mbRunning;
};
typedef ObjectHandle<AServerSocketTcp>	AServerSocketTcpPtr;

class AClientSocketTcp : public ASocket
{
public:
	AClientSocketTcp( DataPostDak& dak , DataPostHouseEnv& env , IDataDialogFactoryPtr fac , SharedObjectPtr userData = NULL );
	virtual ~AClientSocketTcp();

public:

	bool		connectTo( const std::string& remoteIp , const std::string& remotePort , uint32 timeout = (uint32)-1 , const std::string& localIp ="", const std::string& localPort = "" );

	bool		addToDak();
private:
	DataPostDak&		mDak;	
	IDataDialogFactoryPtr mFac;
	SharedObjectPtr		mUserData;
};

typedef ObjectHandle<AClientSocketTcp> AClientSocketTcpPtr;


class AUdpSocket : public ASocket
{
public:
	AUdpSocket(DataPostHouseEnv& env, DataPostDak& dak);
	void							setRemoteAddr( const CommAddr& remoteAddr )
	{
		mAddrRemote = remoteAddr;
	}
	virtual		int32				read( int8* buffer , size_t bufSize ,int32 timeout = -1 )
	{//read from udp socket is not permitted, just waiting for event 
		return ERROR_CODE_OPERATION_FAIL;
	}
	virtual		int32				readAsync(  )
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
	virtual		int32				write( const int8* buffer , size_t bufSize , int32 timeout = -1 ) ;
	
	virtual		int32				writeTo( const int8* buffer, size_t bufSize , const std::string& peerIp , const std::string& peerPort );

	virtual		int32				writeAsync( const int8* buffer , size_t bufSize )
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
	virtual		void				close( ) 
	{//do nothing		
	}

#ifdef ZQ_OS_MSWIN	
	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) 
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
#else
	virtual		int32				onDataAsyncResult(struct ::epoll_event& epollevent)
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
#endif
	virtual		int32				onDataAsyncError( void )
	{
		return ERROR_CODE_OPERATION_FAIL;
	}	
	virtual		void				onCommunicatorClosed( )
	{
	}
};
typedef ObjectHandle<AUdpSocket> AUdpSocketPtr;

class AServerSocketUdp : public AUdpSocket
{
public:
	AServerSocketUdp(  DataPostDak& dak , DataPostHouseEnv& env  );
	~AServerSocketUdp( );
public:
	
	bool		startServer(  const std::string& localIp  , const std::string& localPort ,SharedObjectPtr userData = 0 );	

	void		stop( );
protected:

#ifdef ZQ_OS_MSWIN
	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) ;
#else
	virtual		int32				onDataAsyncResult(struct ::epoll_event& epollevent);
#endif
	virtual		void				onCommunicatorClosed( );

	virtual		int32				onDataAsyncError( void );

	virtual		void				close( );

	virtual		int32				readAsync( );

	void							clear( );

	bool							fixupUdpSocket();

protected:
	AUdpSocketPtr			createCommunicator();
private:
	DataPostDak&		mDak;
	IDgramDialogPtr		mDialog;
	AUdpSocketPtr		mServeCommunicator;
};

typedef ObjectHandle<AServerSocketUdp>	AServerSocketUdpPtr;


}}//namespace ZQ::DataPostHouse

#endif//_zq_data_posthouse_data_communicator_implement_unite_header_file_h__
