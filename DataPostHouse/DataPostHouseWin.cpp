
#include "DataPostHouseWin.h"

#define MLOG mEnv.getLogger()

namespace ZQ
{
namespace DataPostHouse
{

DataPostMan::DataPostMan( DataPostDak& dak , DataPostHouseEnv& env , HANDLE completionPort )
:mCompletionPort(completionPort),mDak(dak),mEnv(env)
{
	mQuit = false;
}

DataPostMan::~DataPostMan( )
{

}

bool DataPostMan::init()
{
	mQuit = false;
	return true;
}

void DataPostMan::final()
{
	//clear all allocated resource here
}

int DataPostMan::run( )
{
	assert( INVALID_HANDLE_VALUE != mCompletionPort );

	DWORD					dwOpByte		= 0;
	LONG_PTR				completionKey	= NULL;
	LPOVERLAPPED			overLapped		= NULL;
	DataCompletionKey*		dataKey			= NULL;

	while( !mQuit )
	{	
		completionKey = NULL;
		overLapped = NULL;
		dataKey = NULL;
		dwOpByte = 0;

		bool bRet = false;
		bRet =  GetQueuedCompletionStatus( mCompletionPort , &dwOpByte ,(PULONG_PTR) &completionKey , &overLapped , 60 * 1000 ) ;
		if( bRet )		
		{
			assert( completionKey != NULL && overLapped != NULL );
			dataKey = (DataCompletionKey*)completionKey;
			assert( dataKey != NULL );
			if( !dataKey->mStatus )
			{				
				//quit
				//log here
				delete dataKey;
				delete overLapped;
				break;
			}
			if( dwOpByte == 0 )
			{//communicator is closed
				//MLOG(ZQ::common::Log::L_DEBUG,CLOGFMT(DataPostMan,"key [%x][%x] is coming, received byte is 0"),dataKey);
				try
				{
					dataKey->dataCommunicator->onCommunicatorClosed();
				}
				catch(...)
				{
					//log here
				}
			}
			else
			{
				try
				{
					dataKey->dataCommunicator->onDataAsyncResult( static_cast<int32>( dwOpByte ) , overLapped );
				}
				catch( ... )
				{
					//log here
				}	
			}		
		}
		else
		{
			if( WAIT_TIMEOUT == GetLastError () )
				continue;

			if( overLapped == NULL )
			{
				//something wrong
#pragma message(__MSGLOC__"should I quit thread here?")
			}
			else
			{
#ifdef _DEBUG
				printf("error\n");
#endif
#pragma message(__MSGLOC__"how about ignoring the error ? ")
				if( completionKey != NULL )
				{
					//communicator closed
					assert( completionKey != NULL && overLapped != NULL );
					dataKey = reinterpret_cast<DataCompletionKey*>(completionKey);
					assert( dataKey != NULL );
					try
					{					
						dataKey->dataCommunicator->onDataAsyncError();
					}
					catch(...)
					{
					}
				}
			}
		}
	}
	return 1;
}

void DataPostMan::stop( )
{	
	DataCompletionKey* key = new DataCompletionKey;
	assert( key != NULL );
	key->dataCommunicator	= NULL;
	key->mStatus			= false;
	LPOVERLAPPED overlapped = new OVERLAPPED;
	memset( overlapped , 0 ,sizeof(OVERLAPPED) );	
	
	PostQueuedCompletionStatus( mCompletionPort , sizeof(DataCompletionKey) , (ULONG_PTR)key , overlapped );
}

//////////////////////////////////////////////////////////////////////////
DataPostDak::DataPostDak( DataPostHouseEnv& env , IDataDialogFactoryPtr factory )
:mEnv(env),
mCompletionPort(INVALID_HANDLE_VALUE)
{
	mDialogFactory = factory;
	assert( factory != NULL );
	mbStart = false;
}

DataPostDak::~DataPostDak( )
{
	stopDak();
	mDialogFactory = NULL;
}

bool DataPostDak::addnewCommunicator( IDataCommunicatorExPtr comm  )
{
	assert( comm != NULL );
	assert( mCompletionPort != INVALID_HANDLE_VALUE );

 	DataCompletionKey* key = comm->getCompletionKey();
	//MLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(DataPostDak,"add communicator [%lld] with key[%x]"), comm->getCommunicatorId(),key );
	HANDLE newCompletionPort = CreateIoCompletionPort( (HANDLE)comm->getCommunicatorDescriptor() , mCompletionPort , (ULONG_PTR)key , 0);	
	if( newCompletionPort == 0 )
	{
		MLOG(ZQ::common::Log::L_ERROR,CLOGFMT(DataPostDak,"addnewCommunicator() failed to add communicator[%lld] into dak, err[%u]"),
			comm->getCommunicatorId(), WSAGetLastError() );
	}
	return newCompletionPort != 0;
}

bool DataPostDak::startDak( int32 maxPostmen   )
{
	assert( maxPostmen >= 1 );
	stopDak();
	mCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE , NULL , NULL , maxPostmen );
	if( mCompletionPort == NULL )
	{
		//log error
		return false;
	}

	for ( int32 i = 0 ; i < maxPostmen ; ++i )
	{
		DataPostMan* postman = new DataPostMan( *this ,mEnv, mCompletionPort );
		assert( postman != NULL );
		mPostMen.push_back( postman );
		postman->start();
	}
	mbStart = true;
	//MLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(startDak, "DataPost has [%d] post men is running"), maxPostmen);
	return true;
}

void DataPostDak::stopDak()
{
	if(!mbStart)
		return;
	mbStart = false;

	DataPostMen::iterator it = mPostMen.begin();
	for( ; it != mPostMen.end() ; it ++ )
	{
		(*it)->stop();
	}

	it = mPostMen.begin();
	for( ; it != mPostMen.end() ; it ++ )
	{
		(*it)->waitHandle(5000);
	}

	mDialogFactory->close();

	if ( mCompletionPort != NULL )
	{
		CloseHandle(mCompletionPort);
		mCompletionPort = NULL;
	}

	for (size_t i = 0; i < mPostMen.size(); i++)
	{
		delete mPostMen[i];
		mPostMen[i] = NULL;
	}
	mPostMen.clear();
}



}}//namespace ZQ::DataPostHouse
