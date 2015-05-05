#ifndef _DATAPOSTHOUSELINUX_H_
#define _DATAPOSTHOUSELINUX_H_

#include "DataPostHouseEnv.h"
#include "DataCommunicatorEx.h"
#include <ZQ_common_conf.h>
#include <NativeThread.h>
#include <NativeThreadPool.h>
#include <vector>
#include <list>


namespace ZQ
{
namespace DataPostHouse
{
#define MAX_EPOLL_SIZE 20000

class HandleRequest;
class DataPostDak : public ZQ::common::NativeThread
{
public:
	DataPostDak( DataPostHouseEnv& env , IDataDialogFactoryPtr factory );
	virtual ~DataPostDak();

	bool startDak(int maxhandleCount = 10);
	void stopDak(void);
	bool addnewCommunicator( IDataCommunicatorExPtr comm, bool bChange=false);
	bool removeOutEpoll(int64& comID,int& fd, void* );
	IDataCommunicatorPtr findAvailKey( void* key );
protected:	
	bool	initDak(int maxHandleCount);
	bool	init(void);
	int		run(void);
	void	final(void);

public:
	typedef std::list<struct epoll_event>	EventList;
	EventList						_epollevents;

	ZQ::common::Mutex				_mutex;
	sem_t							_eventsem;
private:
	typedef std::vector<HandleRequest*>		HandleVector;
	HandleVector					_handles;
	bool							_bQuit;
	bool							_bInit;
	int								_epollfd;
	DataPostHouseEnv&				_env;
	IDataDialogFactoryPtr			_dialogFactory;
	std::map<void*,IDataCommunicatorPtr>	_availKeys;
};


class HandleRequest : public ZQ::common::NativeThread
{
public:
	HandleRequest(DataPostDak& dak, DataPostHouseEnv&	env);
	virtual ~HandleRequest();
	void stop(void);	
protected:	
	bool	init(void);
	int		run(void);
	void	final(void);

private:
	DataPostDak&		_dak;
	DataPostHouseEnv&	_env;
	bool				_bQuit;
};







}}
#endif //_DATAPOSTHOUSELINUX_H_

