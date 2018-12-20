
#include "DataPostHouseLinux.h"
#include <assert.h>
#include <string>
#include <errno.h>

#define MLOG _env.getLogger()

using namespace std;
namespace ZQ
{
namespace DataPostHouse
{
///////////////// clase DataPostDak /////////////////
DataPostDak::DataPostDak( DataPostHouseEnv& env , IDataDialogFactoryPtr factory )
:_env(env), _dialogFactory(factory)
{
	assert(factory != 0);
	_epollfd = -1;
	_bQuit = false;
	_bInit = false;
}

DataPostDak::~DataPostDak()
{
	_dialogFactory = NULL;
}

bool DataPostDak::initDak(int maxHandleCount)
{
	if(_bInit)
	{
		MLOG(ZQ::common::Log::L_DEBUG,"DataPostDak::initDak() have inited");
		return true;
	}
	int re = sem_init(&_eventsem,1,0);
	if(re != 0)
	{
		MLOG(ZQ::common::Log::L_ERROR,"DataPostDak::initDak() init semaphore failed,error string[%s]",strerror(errno));
		return false;
	}

	assert(maxHandleCount >= 1);
	int nmaxsize = MAX_EPOLL_SIZE;
	_epollfd = epoll_create(nmaxsize);
	if(_epollfd == -1)
	{
		MLOG(ZQ::common::Log::L_ERROR,"DataPostDak::initDak() epoll create failed,error string[%s]",strerror(errno));
		return false;
	}
	for(int i = 0; i < maxHandleCount; i++)
	{
		HandleRequest* pHandle = new HandleRequest(*this, _env);
		assert(pHandle != NULL);
		_handles.push_back(pHandle);
		pHandle->start();
	}
	_bInit = true;
	return true;
}

bool DataPostDak::startDak(int maxHandleCount)
{
	if( !initDak(maxHandleCount) )
	{
		MLOG(ZQ::common::Log::L_ERROR,"DataPostDak::startDak() init dak failed");
		return false;
	}

	MLOG(ZQ::common::Log::L_INFO,"DataPostDak::startDak() start dak and set handle count [%d]",maxHandleCount);
	return start();	
}

void DataPostDak::stopDak()
{
	_bQuit = true;
	if ( _epollfd != -1 )
	{
		close(_epollfd);
		_epollfd = -1;
	}

	_dialogFactory->close();
	HandleVector::iterator it = _handles.begin();
	for( ; it != _handles.end() ; it ++ )
	{
		(*it)->stop();
	}

	it = _handles.begin();
	for( ; it != _handles.end() ; it ++ )
	{
		sem_post(&_eventsem);
	}

	it = _handles.begin();
	for( ; it != _handles.end() ; it ++ )
	{
		(*it)->waitHandle(3000);
		delete (*it);
	}

	try{
		sem_destroy(&_eventsem);
	}
	catch(...){}

	_handles.clear();
	_bInit = false;
	MLOG(ZQ::common::Log::L_INFO,"DataPostDak::stopDak() DataPostDak stopped");
}

bool DataPostDak::init(void)
{

	return true;
}

bool DataPostDak::addnewCommunicator( IDataCommunicatorExPtr comm ,bool bChange)
{
	if( comm == 0 )
		return false;

	assert( _epollfd != -1);

	DataCompletionKey* key = comm->getCompletionKey();
	int fd = comm->getCommunicatorDescriptor();
	if( fd <0 )
		return false;

	if(!bChange)
		MLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(DataPostDak,"add communicator [%lld] with key[%p], socket[%d]"),
					comm->getCommunicatorId(),key,fd );	
	
	struct ::epoll_event ev;
	ev.data.ptr = reinterpret_cast<void*>(key);
	ev.events = EPOLLIN | EPOLLONESHOT;//| EPOLLOUT 
	
	int rc = 0;
	if(bChange)
		rc = ::epoll_ctl(_epollfd,EPOLL_CTL_MOD,fd,&ev );
	else
		rc = ::epoll_ctl(_epollfd,EPOLL_CTL_ADD,fd,&ev );
	if(rc != 0)
	{
		MLOG(ZQ::common::Log::L_ERROR,"AddnewCommunicator [%lld] failed string[%s]", 
			comm->getCommunicatorId() , strerror(errno));
		return false;
	}
	else
	{
		if( !bChange)
		{
			ZQ::common::MutexGuard gd(_mutex);
			_availKeys.insert( std::map<void*,IDataCommunicatorPtr>::value_type(key, comm) );
			MLOG(ZQ::common::Log::L_DEBUG,CLOGFMT(DataPostDak,"AddnewCommunicator() COMM[%lld] current key count [%u]"),
					comm->getCommunicatorId(), _availKeys.size());
		}
	}
	
	return true;
}

IDataCommunicatorPtr DataPostDak::findAvailKey( void* ptr )
{
	ZQ::common::MutexGuard gd(_mutex);
	std::map<void*,IDataCommunicatorPtr>::iterator it = _availKeys.find(ptr);
	if( it == _availKeys.end())
		return 0;
	else
		return it->second;
}

bool DataPostDak::removeOutEpoll(int64& comID,int& fd, void *key)
{
	{
		ZQ::common::MutexGuard gd(_mutex);
		_availKeys.erase(key);
		MLOG(ZQ::common::Log::L_DEBUG,CLOGFMT(DataPostDak,"COMM[%lld] removeOutEpoll() current key count [%u]"),
				comID, (uint)_availKeys.size());
	}
	struct ::epoll_event event;
	int rc = epoll_ctl(_epollfd,EPOLL_CTL_DEL,fd,&event);
	if(rc != 0)
	{
		MLOG(ZQ::common::Log::L_DEBUG,"DataPostDak::removeOutEpoll() epoll_ctl delete communicator [%lld], socket [%d] failed [%s]", 
			comID , fd, strerror(errno));
		return false;	
	}
	else
	{
		MLOG(ZQ::common::Log::L_DEBUG,"DataPostDak::removeOutEpoll() remove communicator [%lld] socket[%d] out epoll",
			comID, fd);
		return true;
	}	
	
}

int DataPostDak::run(void)
{
	struct ::epoll_event	events[MAX_EPOLL_SIZE + 1];
	int nwaitfd = 0;
	while(!_bQuit)
	{
		nwaitfd = epoll_wait(_epollfd,events,MAX_EPOLL_SIZE, 1000);
		if(_bQuit)
		{
			MLOG(ZQ::common::Log::L_DEBUG,"DataPostDak::run() _bQuit is true,this thread exit");
			break;
		}
		if(nwaitfd == -1)
		{
			if(errno == EINTR)
				continue;

			MLOG(ZQ::common::Log::L_ERROR,"DataPostDak::run() epoll_wait have a error, string code[%s]",strerror(errno));
			return -1;
		}
		size_t eventCount = 0;
		for(int i = 0; i < nwaitfd; i++)
		{
			{
				ZQ::common::MutexGuard gd(_mutex);
				_epollevents.push_back(events[i]);
				eventCount = _epollevents.size();
			}
			sem_post(&_eventsem);
		}
		if( eventCount > 50 )
		{
			MLOG(ZQ::common::Log::L_DEBUG,CLOGFMT(DataPostDak,"run() too many pending events %u"), eventCount);
		}
	}

	return 0;
}

void DataPostDak::final(void)
{

}

//////////////////// class HandleRequest ////////////////////
HandleRequest::HandleRequest(DataPostDak& dak, DataPostHouseEnv&	env)
:_dak(dak), _env(env), _bQuit(false)
{
}

HandleRequest::~HandleRequest()
{
}

void HandleRequest::stop()
{
	_bQuit = true;
}

bool HandleRequest::init(void)
{
	return true;
}

int HandleRequest::run(void)
{
	
	struct ::epoll_event epollevent;
	DataCompletionKey* key = NULL;	
	while(!_bQuit)
	{
		int re = sem_wait(&_dak._eventsem);
		if(_bQuit)
		{
			MLOG(ZQ::common::Log::L_DEBUG,"HandleRequest::run() _bQuit is true, thread should stop");
			break;
		}
		if(re != 0)
		{
			if(errno == EINVAL && _bQuit)
				MLOG(ZQ::common::Log::L_INFO,"HandleRequest::run() should be quit");
			else if(errno == EINTR)
				continue;
			else
				MLOG(ZQ::common::Log::L_ERROR,"HandleRequest::run() sem_wait have a error code[%d] string[%s]",
						errno,strerror(errno));
			break;
		}
		bool bContinue = true;
		while( bContinue)	
		{
			{
				ZQ::common::MutexGuard gd(_dak._mutex);
				if(_dak._epollevents.size() < 1)
				{
					bContinue = false;
					break;
				}
				struct ::epoll_event& epollev = _dak._epollevents.front();
				epollevent.events = epollev.events;
				epollevent.data.ptr = epollev.data.ptr;
				_dak._epollevents.pop_front();

			}
			key = reinterpret_cast<DataCompletionKey*>(epollevent.data.ptr);
			IDataCommunicatorExPtr communicator = IDataCommunicatorExPtr::dynamicCast( _dak.findAvailKey(key) );
			if(!communicator)
				continue;
			if(key != NULL)
			{	
				try
				{
					//MLOG(ZQ::common::Log::L_DEBUG,"Communicator[%lld] have date to handle",	key->dataCommunicator->getCommunicatorId());
					//MLOG.flush();
					re = communicator->onDataAsyncResult(epollevent);
					if(re == ERROR_CODE_OPERATION_OK)
					{
						if( !_dak.addnewCommunicator(communicator,true) )
							communicator->onDataAsyncError();
					}
				}
				catch(...){}			
			}
			else
			{
				MLOG(ZQ::common::Log::L_ERROR,	"HandleRequest::run() Have a event but  the key is NULL");
			}
		}
	}
	return 0;
}

void HandleRequest::final(void)
{

}

}}

