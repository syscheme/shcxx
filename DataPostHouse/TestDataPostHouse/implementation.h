
#ifndef _TEST_DATA_POSTHOUSE_IMPLEMENTATION_HEADER_FILE_H__
#define _TEST_DATA_POSTHOUSE_IMPLEMENTATION_HEADER_FILE_H__

#include <DataCommunicator.h>
#include <map>
#include <Locks.h>

using namespace ZQ::DataPostHouse;

class DialogImpl : public ZQ::DataPostHouse::IDataDialog
{
public:
	DialogImpl( );
	virtual ~DialogImpl( );
public:
	virtual		void		onCommunicatorSetup( IDataCommunicatorPtr communicator ) ;

	virtual		void		onCommunicatorDestroyed( IDataCommunicatorPtr communicator ) ;

	virtual		bool		onRead( const int8* buffer ,size_t bufSize ) ;

	virtual		void		onWritten( size_t bufSize ) ;

	virtual		void		onError( ) ;

private:
	IDataCommunicatorPtr	mComm;
	char szBuf[4096];

};
class DialogFactoryImpl : public ZQ::DataPostHouse::IDataDialogFactory
{
public:
	DialogFactoryImpl( );
	virtual ~DialogFactoryImpl( );
public:
	
	virtual void					onClose( CommunicatorS& comms ) ;

	virtual IDataDialogPtr			onCreateDataDialog( IDataCommunicatorPtr communicator ) ;

	virtual void					onReleaseDataDialog( IDataDialogPtr idalog , IDataCommunicatorPtr communicator) ;

};
#endif//_TEST_DATA_POSTHOUSE_IMPLEMENTATION_HEADER_FILE_H__
