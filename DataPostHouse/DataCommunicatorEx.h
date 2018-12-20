
#ifndef _zq_data_post_house_communicator_ex_header_file_h__
#define _zq_data_post_house_communicator_ex_header_file_h__


#include "DataCommunicator.h"

#ifdef ZQ_OS_LINUX
extern "C"
{
#include <sys/epoll.h>
}
#endif

namespace ZQ
{
namespace DataPostHouse
{

class IDataCommunicatorEx;
typedef ObjectHandle<IDataCommunicatorEx> IDataCommunicatorExPtr;
class DataCompletionKey 
{
public:
	DataCompletionKey()
	{
		dataCommunicator = NULL;
	}
	virtual ~DataCompletionKey( )
	{
		if( dataCommunicator )
			dataCommunicator = NULL;		
	}

public:

	IDataCommunicatorExPtr		dataCommunicator;

	bool						mStatus;//TRUE for running, FALSE for closing

};
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//for internal use
class IDataCommunicatorEx :public IDataCommunicator
{
public:

#ifdef ZQ_OS_MSWIN
	virtual			int32		onDataAsyncResult( int32  size , LPOVERLAPPED overlap ) = 0;

	virtual			SOCKET		getCommunicatorDescriptor( ) = 0;
#else
	virtual			int32		onDataAsyncResult( struct ::epoll_event& epollevent) = 0;

	virtual			int			getCommunicatorDescriptor( ) = 0;
#endif

	virtual			int32		onDataAsyncError(void ) = 0;

	virtual			void		onCommunicatorClosed( ) = 0;


	virtual			void		attchDialog( IDataDialogPtr dialog ) = 0;

	virtual			void		attachUserData( SharedObjectPtr userData ) = 0;

	virtual			DataCompletionKey* getCompletionKey( ) = 0;
};


}}////namespace ZQ::DataPostHouse
#endif//_zq_data_post_house_communicator_ex_header_file_h__
