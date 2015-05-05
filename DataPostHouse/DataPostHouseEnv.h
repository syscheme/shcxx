
#ifndef _zq_data_post_house_environment_header_file_h__
#define _zq_data_post_house_environment_header_file_h__

#include <ZQ_common_conf.h>
#include <Log.h>
#include <Locks.h>
#include <assert.h>
#include "DataCommunicator.h"

namespace ZQ
{
namespace DataPostHouse
{

class DataPostHouseEnv
{
public:
	DataPostHouseEnv()
	{
		mConnectionId		=	0;
		mReadBufferSize		=	4096;
		mEncryptBufferSize  =   4096;
	}
	virtual ~DataPostHouseEnv( )
	{

	}
	inline int32	getReadBufferSize() const
	{
		return mReadBufferSize;
	}
	
	inline ZQ::common::Log& getLogger( )
	{
		return glog;
	//	assert( mLogger != NULL );
	//	return (*mLogger);
	}

	inline int64 generateConnectionId( )
	{
		ZQ::common::MutexGuard gd(mMutex);
		return ++mConnectionId;
	}
	inline IDataDialogFactoryPtr getDataDialogFactory( ) 
	{
		return dataFactory;
	}

public:
	
	int32					mReadBufferSize;
	ZQ::common::Log*		mLogger;
	ZQ::common::Mutex		mMutex;
	int64					mConnectionId;
	IDataDialogFactoryPtr	dataFactory;
	int32                   mEncryptBufferSize;
};



}} //namespace ZQ::DataPostHouse

#endif//_zq_data_post_house_environment_header_file_h__
