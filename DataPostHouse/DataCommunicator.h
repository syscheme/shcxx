
#ifndef _zq_data_post_house_communicator_header_file_h__
#define _zq_data_post_house_communicator_header_file_h__

#include <ZQ_common_conf.h>
#include <Locks.h>
#include <set>
#include "common_define.h"

#ifdef ZQ_OS_LINUX
extern "C"
{
#include <sys/socket.h>
}
#endif

namespace ZQ
{
namespace DataPostHouse
{

//fail
#define ERROR_CODE_OPERATION_FAIL		-10
#define ERROR_CODE_OPERATION_TIMEOUT	-11
#define ERROR_CODE_OPERATION_PENDING	-20
#define ERROR_CODE_OPERATION_CLOSED		-1
//success
#define ERROR_CODE_OPERATION_OK			1




enum CommunicatorType
{
	COMM_TYPE_NULL,
	COMM_TYPE_TCP,
	COMM_TYPE_UDP,
	COMM_TYPE_PIPE,
	COMM_TYPE_SSL
};

typedef struct _CommAddr
{
	union
	{
		char						addr[256];//256 is enough ?
		struct sockaddr_storage 	storage;	
	}u;
	int32				addrLen;
	_CommAddr()
	{
		addrLen	= sizeof(u);
	}
}CommAddr;

typedef ObjectHandle<SharedObject>	SharedObjectPtr;

class IDataDialog ;

typedef ObjectHandle<IDataDialog>	IDataDialogPtr;

class IDataCommunicator : public SharedObject
{
public:
	virtual		CommunicatorType	getCommunicatorType( ) const = 0;
	///detect current communicator is valid or not
	virtual		bool				isValid( ) const = 0;

	///get the  unique communicator Id
	virtual		int64				getCommunicatorId( ) const = 0 ;

	virtual		bool				getCommunicatorAddrInfo( CommAddr& local , CommAddr& remote ) const = 0;
	
	///read data from communicator
	///@return the actually read buffer size in byte
	/// <0 means failed to perform read operation
	///@param buffer data buffer to hold the data
	///@param bufSize data buffer size in byte 
	///@param timeout timed out interval in milliseconds
	/// if this value < 0 mean no timeout
	virtual		int32				read( int8* buffer , size_t bufSize ,int32 timeout = -1 ) = 0;

	///read data in asynchronous way
	///and communicator afford read buffer for you
	virtual		int32				readAsync(  ) = 0;	

	virtual		int32				write( const int8* buffer , size_t bufSize , int32 timeout = -1 ) = 0;

	virtual		int32				writeAsync( const int8* buffer , size_t bufSize ) = 0;

	///close communicator
	virtual		void				close( ) = 0;
		
	virtual		IDataDialogPtr		getDataDialog( ) =0;

	virtual		SharedObjectPtr		getUserData( ) = 0;

	virtual     void                getLocalAddress(std::string& localIP, std::string& localPort)const = 0;

	virtual     void                getRemoteAddress(std::string& remoteIP, std::string& remotePort) const = 0;

	virtual		uint32				getIdleTime() = 0;
};

typedef ObjectHandle<IDataCommunicator> IDataCommunicatorPtr;
class IDataDialog : public SharedObject
{
public:
	virtual ~IDataDialog(){}

	virtual		void		onCommunicatorSetup( IDataCommunicatorPtr communicator ) = 0;

	virtual		void		onCommunicatorDestroyed( IDataCommunicatorPtr communicator ) = 0;

	///new data arrived
	///return true if you want to DataPostHouse receive more data automatically
	/// or else , just return false and it's your responsibility to receive data
	/// by using communicator::read()
	virtual		bool		onRead( const int8* buffer , size_t bufSize ) = 0;

	virtual		void		onWritten( size_t bufSize ) = 0;
	
	virtual		void		onError( ) = 0;
};

enum
{
	APP_TYPE_RTSP,
	APP_TYPE_LSCP
};

class IDgramDialog : public IDataDialog
{
public:
	// For data gram dialog
	// IDataDialog::onRead IDataDialog::onWritten will never be invoked, instead of calling onData
	virtual		void		onData( const int8* buffer , size_t bufSize , IDataCommunicatorPtr comm  ) = 0;
};
typedef ObjectHandle<IDgramDialog> IDgramDialogPtr;

class IDataDialogFactory : public SharedObject
{
protected:

	typedef std::set<IDataCommunicatorPtr>	CommunicatorS;

public:
	///it's dialog factory's responsibility to close all communicator if it's close is invoked
	virtual void					close( ) ;

	virtual IDataDialogPtr			createDataDialog( IDataCommunicatorPtr communicator ) ;

	virtual void					releaseDataDialog( IDataDialogPtr dialog , IDataCommunicatorPtr communicator) ;

public:

	virtual	void					onClose( CommunicatorS& comms ) = 0;

	virtual IDataDialogPtr			onCreateDataDialog( IDataCommunicatorPtr communicator ) = 0;

	virtual void					onReleaseDataDialog( IDataDialogPtr dialog , IDataCommunicatorPtr communicator ) = 0;


protected:

	CommunicatorS					mComms;
	ZQ::common::Mutex				mCommLocker;
};

typedef ObjectHandle<IDataDialogFactory> IDataDialogFactoryPtr;


class TcpCommunicatorSettings
{
public:
	TcpCommunicatorSettings( IDataCommunicatorPtr comm = NULL );
	virtual ~TcpCommunicatorSettings();

public:

	void	attachCommunicator( IDataCommunicatorPtr comm  );

	bool	nodelay( bool bNodelay );

	bool	holdon( bool bHoldOn );

	bool	setWriteBufSize( size_t sz );

	bool	setReadBufSize( size_t sz );

	bool	setSendTimeout( uint32 to );

private:
#ifdef ZQ_OS_LINUX
	int		mSock;
#elif defined ZQ_OS_MSWIN
	SOCKET	mSock;
#endif
};

}}//namespace ZQ::DataPostHouse

#endif //_zq_data_post_house_communicator_header_file_h__
