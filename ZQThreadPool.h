// ZQThreadPool.h: interface for the ZQThreadPool class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREADPOOL_H__B72F4746_2265_407F_8FB1_88EEBEDD4A01__INCLUDED_)
#define AFX_THREADPOOL_H__B72F4746_2265_407F_8FB1_88EEBEDD4A01__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <queue>
#include <vector>
#include <set>
#include <functional>
#include "SyncUtil.h"

namespace ZQ {
namespace common {

class ZQ_COMMON_API ZQThread;
class ZQ_COMMON_API ZQThreadPool;
class ZQ_COMMON_API ZQWorkItem;

class ZQThread {
public:
	ZQThread();
	virtual ~ZQThread();

	bool start();
	void stop();

	bool isStopped() const
	{
		return (bool )_stopped;
	}

	void setDefStack(size_t defStatck)
	{
		_defStatck = defStatck;
	}

	size_t getDefStack()
	{
		return _defStatck;
	}

	void join();

	void suspend();
	void resume();

protected:
	virtual bool init() 
	{
		return true;
	}

	virtual int run() = 0;

	virtual void final(int exitCode) 
	{

	}
	
	void applyStop()
	{
		SetEvent(_stopEvent);
	}

	virtual void onStop()
	{
		applyStop();
	}

protected:
	static unsigned long __stdcall win32ThreadProc(void* param);

protected:
	
	volatile bool					_stopped;
	HANDLE							_threadHandle;
	unsigned long					_threadId;
	HANDLE							_stopEvent;
	size_t							_defStatck;
};

class ZQThreadPool;

#define WORKITEM_PRIORITY_LOW			50
#define WORKITEM_PRIORITY_NORMAL		100
#define WORKITEM_PRIORITY_HIGH			200

class ZQWorkItem {
	
	friend class ZQWorkThread;
	friend class ZQThreadPool;

public:
	ZQWorkItem(ZQThreadPool& pool, 
		unsigned char priority = WORKITEM_PRIORITY_NORMAL): 
		_pool(pool), _priority(priority)
	{

	}

	bool start();

	unsigned char getPriority() const
	{
		return _priority;
	}

protected:

	virtual bool init() 
	{
		return true;
	}

	virtual int run() = 0;

	virtual void final(int exitCode) 
	{

	}

	ZQWorkThread* getThread()
	{
		return _thread;
	}

	void setThread(ZQWorkThread* thread)
	{
		_thread = thread;
	}

protected:
	ZQThreadPool&		_pool;
	unsigned char		_priority;
	ZQWorkThread*		_thread;
};

#if !defined(_ZQThreadPool_LIGHTLOCK)

typedef XRWLockEx ZQThreadPoolLock;

#else

class ZQThreadPoolLock: public LightLock {
public:
	ZQThreadPoolLock(const uint32 timeo = INFINITE)
	LightLock& wlock()
	{
		return *this;
	}

	LightLock& rlock()
	{
		return *this;
	}

	void lockWrite(unsigned long = INFINITE)
	{
		lock();
	}

	void unlockWrite()
	{
		unlock();
	}

	void lockRead(unsigned long = INFINITE)
	{
		lock();
	}

	void unlockRead()
	{
		unlock();
	}
};

#endif

class ZQWorkThread;

#ifdef _DEBUG
#define ZQTHREADPOOL_TIMEO			INFINITE
#else
#define ZQTHREADPOOL_TIMEO			INFINITE
#endif

class ZQThreadPool: private ZQThreadPoolLock {

	friend class ZQWorkItem;
	friend class ZQWorkThread;

public:
	ZQThreadPool(size_t initSize = 3, size_t maxSize = 20, 
		size_t increment = 3, unsigned long idleTimeout = 30000);

	virtual ~ZQThreadPool();

	bool start();
	void stop();

	size_t activeCount();
	size_t size();

	void setDefThreadStack(size_t defThreadStack);
	size_t getDefThreadStack();

protected:
	bool enter(ZQWorkItem* item);
	ZQWorkItem* leave();

	virtual bool needGrow();

	void createThread(int threadCount);

	void addThread(ZQWorkThread* thread);
	bool removeThread(ZQWorkThread* thread);

	void onThreadActivate(ZQWorkThread* thread);
	void onThreadDeactivate(ZQWorkThread* thread);

protected:

	struct WorkItemLess: std::binary_function<ZQWorkItem*, ZQWorkItem*, bool> {

		bool operator()(const ZQWorkItem* item1, const ZQWorkItem* item2) const
		{
			return (NULL != item1 && NULL != item1 && 
				item1->getPriority() < item2->getPriority());
		}
	};

	friend class ScPoolBgThread;
	
	typedef WaitableQueue2<ZQWorkItem* , 
		RawPriQueue<ZQWorkItem*, WorkItemLess> > WorkItemQueue;
	
	// typedef WaitableQueue2<ZQWorkItem* > WorkItemQueue;
	// typedef WaitablePriQueue<ZQWorkItem*, WorkItemGreat> WorkItemQueue;
	// typedef WaitableQueue<ZQWorkItem* > WorkItemQueue;

	typedef std::set<ZQThread*> ThreadSet;
	
protected:
	ThreadSet		_threads;
	unsigned long	_idleTimeout;	// 线程空闲超时(ms)
	size_t			_initSize;		// 初始线程数
	size_t			_maxSize;		// 最大线程数, 0 为无限
	size_t			_increment;		// 线程增量, 0 时代表不增长	
	size_t			_activeCount;	// 活动线程计数器
	bool			_incFlag;
	HANDLE			_stopEvent;
	size_t			_defThreadStack;
	WorkItemQueue	_itemQueue;
	volatile bool	_stopped;
};

} // namespace ZQ {
} // namespace common {


#endif // !defined(AFX_THREADPOOL_H__B72F4746_2265_407F_8FB1_88EEBEDD4A01__INCLUDED_)
