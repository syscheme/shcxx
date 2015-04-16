
#ifndef _zq_data_posthouse_data_communicator_implement_win_header_file_h__
#define _zq_data_posthouse_data_communicator_implement_win_header_file_h__

#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <NativeThread.h>
#include "DataPostHouseEnv.h"
#include "DataCommunicatorEx.h"
#include "DataPostHouseWin.h"


namespace ZQ
{
namespace DataPostHouse
{

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
	void			init( int family , int type , int protocol )
	{
		mAddrHint.ai_family		=	family;
		mAddrHint.ai_socktype	=	type;
		mAddrHint.ai_protocol	=	protocol;
		mAddrHint.ai_flags		=	AI_NUMERICHOST;
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
		else
		{
			return true;
		}
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
	ASocket( DataPostHouseEnv& env );	
	ASocket( DataPostHouseEnv& env , const SOCKET& s , const CommunicatorType& type = COMM_TYPE_NULL , SharedObjectPtr userData = NULL );
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
	
	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) ;
	virtual		int32				onDataAsyncError( );
	virtual		void				onCommunicatorClosed( ) ;
	virtual		SOCKET				getCommunicatorDescriptor( ) ;
	virtual		void				attchDialog( IDataDialogPtr dialog ) ;
	virtual		void				attachUserData( SharedObjectPtr userData );
	virtual		DataCompletionKey*	getCompletionKey( ) ;
public:

	bool							createSocket( int family, int type , int protocol );

	bool							bind( const std::string& localIp , const std::string& localPort );	

	bool							listen( int backLog = 100 ); 

	ASocket*						accept( );
	
	bool							connect( const std::string& remoteIp , const std::string& remotePort );
	
protected:

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

	WSAOVERLAPPED					mReadOverlapped;
	WSABUF							mReadBuffer;

	WSAOVERLAPPED					mWriteOverlapped;
	WSABUF							mWriteBuffer;
	
	DataPostHouseEnv&				mEnv;
	
	DataCompletionKey				mCompletionKey;
	CommAddr						mAddrLocal;
	CommAddr						mAddrRemote;
};

typedef ObjectHandle<ASocket>	ASocketPtr;


class AServerSocketTcp : public ASocket , public ZQ::common::NativeThread
{
public:

	AServerSocketTcp( DataPostDak& dak , DataPostHouseEnv& env );

	virtual ~AServerSocketTcp( );

public:
	
	bool		startServer(  const std::string& localIp  , const std::string& localPort ,  SharedObjectPtr userData = NULL , int32 maxBacklog = 100 );	

	void		stop( );

protected:

	bool		init(void);
	int			run(void);
	void		final(void);

private:
	DataPostDak&		mDak;	
};

typedef ObjectHandle<AServerSocketTcp>	AServerSocketTcpPtr;


class AServerSocketUdp : public ASocket
{
public:
	AServerSocketUdp(  DataPostDak& dak , DataPostHouseEnv& env  );
	~AServerSocketUdp( );
public:
	
	bool		startServer(  const std::string& localIp  , const std::string& localPort ,SharedObjectPtr userData =NULL );	

	void		stop( );

public:

	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) ;

	virtual		void				onCommunicatorClosed( );

	virtual		void				close( );

	virtual		int32				readAsync( );

protected:

	IDataCommunicatorExPtr			createCommunicator( );

private:

	DataPostDak&					mDak;	
};

typedef ObjectHandle<AServerSocketUdp>	AServerSocketUdpPtr;


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class APipe : public IDataCommunicatorEx
{
public:
	APipe( DataPostHouseEnv& env );
	~APipe( );

public:

	virtual		CommunicatorType	getCommunicatorType( ) const ;
	///detect current communicator is valid or not
	virtual		bool				isValid( ) const;

	///get the  unique communicator Id
	virtual		int64				getCommunicatorId( ) const;

	virtual		bool				getCommunicatorAddrInfo( CommAddr& local , CommAddr& remote ) const;

	///read data from communicator
	///@return the actually read buffer size in byte
	/// <0 means failed to perform read operation
	///@param buffer data buffer to hold the data
	///@param bufSize data buffer size in byte 
	///@param timeout timed out interval in milliseconds
	/// if this value < 0 mean no timeout
	virtual		int32				read( int8* buffer , size_t bufSize ,int32 timeout = -1 );

	///read data in asynchronous way
	///and communicator afford read buffer for you
	virtual		int32				readAsync(  );

	virtual		int32				write( const int8* buffer , size_t bufSize , int32 timeout = -1 );

	virtual		int32				writeAsync( const int8* buffer , size_t bufSize );

	///close communicator
	virtual		void				close( ) ;

	virtual		IDataDialogPtr		getDataDialog( ) ;

	virtual		SharedObjectPtr		getUserData( );

	virtual		int32				onDataAsyncResult( int32  size , LPOVERLAPPED overlap );

	virtual		int32				onDataAsyncError( LPOVERLAPPED overlap );

	virtual		void				onCommunicatorClosed( );

	virtual		SOCKET				getCommunicatorDescriptor( );

	virtual		void				attchDialog( IDataDialogPtr dialog );

	virtual		void				attachUserData( SharedObjectPtr userData ) ;

	virtual		DataCompletionKey*	getCompletionKey( ) ;

protected:

	void		clear( );

public:

	///create named pipe	
	bool		createPipe( const std::string& pipeName , int32 maxInstance ,
								int32 defaultTimeout = 5000, 
								int32 inBufSize = 4 * 1024 , int32 outBufSize = 4 * 1024 );

	///connect named pipe 
	///timeout == 0 mean use pipe's default timeout value
	bool		connectPipe( const std::string& pipeName , int32 timeout = 0 );


protected:	
	
	HANDLE							mPipe;
	int64							mId;	
	CommunicatorType				mType;
	SharedObjectPtr					mUserData;
	IDataDialogPtr					mDataDialog;

	OVERLAPPED						mReadOverlapped;
	//WSABUF						mReadBuffer;
	LPCVOID							mReadBuffer;
	DWORD							mReadSize;


	OVERLAPPED						mWriteOverlapped;
	//WSABUF						mWriteBuffer;
	LPCVOID							mWriteBuffer;
	DWORD							mWriteSize;

	DataPostHouseEnv&				mEnv;

	DataCompletionKey				mCompletionKey;
	CommAddr						mAddress;	
};

// class APipeServer : public APipe
// {
// public:
// 	APipeServer( DataPostHouseEnv& env );
// 	~APipeServer( );
// 
// public:
// 
// 	bool							startServer( const std::string& pipeName , int32 maxInstance , 
// 													int32 defaultTimeout = 5000, 
// 													int32 inBufSize = 4 * 1024 , int32 outBufSize = 4 * 1024 );
// 
// private:
// 	OVERLAPPED						mListenOverlapped;
// };
// 
}}//namespace ZQ::DataPostHouse

#endif//_zq_data_posthouse_data_communicator_implement_win_header_file_h__
