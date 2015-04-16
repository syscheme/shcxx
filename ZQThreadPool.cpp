// ZQThreadPool.cpp: implementation of the ZQThreadPool class.
//
//////////////////////////////////////////////////////////////////////

#include "ZQThreadPool.h"

#ifdef _DEBUG
#include <atlbase.h>
#define _TRACE		ATLTRACE
#else
#define _TRACE		
#endif

namespace ZQ {
namespace common {

//////////////////////////////////////////////////////////////////////////

ZQThread::ZQThread()
{
	_stopped = true;
	_threadHandle = NULL;
	_threadId = 0;
	_stopEvent = NULL;
	_defStatck = 0;
}

ZQThread::~ZQThread()
{
	_TRACE("ZQThread::~ZQThread():\tthis = %p\n", this);

	if (_stopped == false)
		stop();

	if (_threadHandle) {
		CloseHandle(_threadHandle);
#ifdef _DEBUG
		_threadHandle = NULL;
#endif
	}
}

bool ZQThread::start()
{
	assert(_threadHandle == NULL);

	if (!init()) {
		return false;
	}

	_stopped = false;

	if (_stopEvent) {
		_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	} else {
		ResetEvent(_stopEvent);
	}

	_threadHandle = CreateThread(NULL, _defStatck, win32ThreadProc, 
		this, 0, &_threadId);

	return (_threadHandle != NULL);
}

void ZQThread::stop()
{
	_stopped = true;

	onStop();

	WaitForSingleObject(_stopEvent, INFINITE);
}

unsigned long __stdcall ZQThread::win32ThreadProc(void* param)
{
	ZQThread* thisPtr = (ZQThread* )param;
	unsigned long result = -1;

	try {
	
		result = thisPtr->run();
	} catch(...) {
		assert(false);
		_TRACE("%s:%d\tScThread::win32ThreadProc()\t"
			"warning: occurred a unknown error.\n", 
			__FILE__, __LINE__);
	}

	thisPtr->_stopped = true;
	thisPtr->_threadHandle = NULL;
	thisPtr->final(result);	
	
	return result;
}

void ZQThread::suspend()
{
	assert(_threadHandle);
	SuspendThread(_threadHandle);
}

void ZQThread::resume()
{
	assert(_threadHandle);
	ResumeThread(_threadHandle);
}

//////////////////////////////////////////////////////////////////////////

bool ZQWorkItem::start()
{
	if (!this->init())
		return false;

	return _pool.enter(this);	
}

//////////////////////////////////////////////////////////////////////////

class ZQWorkThread : public ZQThread {
public:
	ZQWorkThread(ZQThreadPool& pool);
	virtual ~ZQWorkThread();
	virtual int run();
	virtual void final(int exitCode);

protected:
	ZQThreadPool&	_pool;
};

ZQWorkThread::ZQWorkThread(ZQThreadPool& pool):
	_pool(pool)
{
	
}

ZQWorkThread::~ZQWorkThread()
{
	
}

int ZQWorkThread::run()
{
	ZQWorkItem* item;
	
	while (!_stopped) {
		item = _pool.leave();
		
		if (item == NULL && _pool.removeThread(this))
			return 0;

		if (item) {

			item->setThread(this);

			int result;
				
			try {
				_pool.onThreadActivate(this);
				result = item->run();
			} catch(...) {
				assert(false);
				_TRACE("%s:%d\tZQWorkThread::run()\t"
					"warning: occurred a unknown error.\n", 
					__FILE__, __LINE__);
			}

			item->final(result);
			_pool.onThreadDeactivate(this);
		}

	}

	_pool.removeThread(this);

	return 0;
}

void ZQWorkThread::final(int exitCode)
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////

class StopWorkItem: public ZQWorkItem {
public:
	StopWorkItem(ZQThreadPool& pool):
	  ZQWorkItem(pool, 255)
	{

	}

	virtual int run()
	{
		ZQWorkThread* thread = getThread();
		assert(thread);
		thread->stop();
		return 0;
	}

	virtual void final(int exitCode)
	{
		delete this;
	}
};

//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define _ITEMQUEUE_UNQUE		true
#else
#define _ITEMQUEUE_UNQUE		false
#endif

ZQThreadPool::ZQThreadPool(size_t initSize /* = 3 */, 
						   size_t maxSize /* = 20 */, 
						   size_t increment /* = 3 */, 
						   unsigned long idleTimeout /* = 30000 */):
							ZQThreadPoolLock(ZQTHREADPOOL_TIMEO), 
							// : _itemQueue(0, _ITEMQUEUE_UNQUE)
							_itemQueue(0x7fffffff)	   
{
	if (initSize == 0) {
		assert(false);
		initSize = 1;
	}

	_initSize = initSize;

	if (maxSize != 0 && maxSize < initSize) {
		assert(false);
		maxSize = initSize;
	}

	_maxSize = maxSize;
	_increment = increment;
	_idleTimeout = idleTimeout;
	_incFlag = true;
	_activeCount = 0;
	_stopEvent = NULL;
	_defThreadStack = 0;
	_stopped = true;
}

ZQThreadPool::~ZQThreadPool()
{
	if (_stopEvent)
		CloseHandle(_stopEvent);
}

bool ZQThreadPool::start()
{
	if (_stopEvent == NULL) {

		_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	} else {
		ResetEvent(_stopEvent);
	}

	_stopped = false;
	createThread(_initSize);
	
	return true;
}

void ZQThreadPool::stop()
{
	size_t threadCount;

	{
		AbsAutoLock autoLock(wlock());

		_incFlag = false;
		_stopped = true;
		threadCount = _threads.size();
	}

	if (threadCount) {
		
		for (size_t i = 0; i < threadCount; i ++) {
			StopWorkItem* stopItem = new StopWorkItem(*this);
			enter(stopItem);
		}
		
		WaitForSingleObject(_stopEvent, INFINITE);		
	}

	{
		AbsAutoLock autoLock(wlock());
		// clear the queue.
		ZQWorkItem* item;
		while(_itemQueue.pop(item, 0)) {
			item->init();
			item->final(-1);
		}

		_incFlag = true;
	}
}

size_t ZQThreadPool::activeCount()
{
	AbsAutoLock autoLock(rlock());
	return _activeCount;
}

size_t ZQThreadPool::size()
{
	AbsAutoLock autoLock(rlock());
	return _threads.size();
}

bool ZQThreadPool::enter(ZQWorkItem* item)
{
	/*
	if (size() == 0) {
		_TRACE("Warning: the threadpool is stopped\n");
		return false;
	}
	*/

	_itemQueue.push(item);

	return true;
}

ZQWorkItem* ZQThreadPool::leave()
{
	ZQWorkItem* item = NULL;
	if (!_itemQueue.pop(item, _idleTimeout))
		return NULL;
	return item;
}

void ZQThreadPool::createThread(int threadCount)
{
	ZQWorkThread* thread;
	for (int i = 0; !_stopped && i < threadCount; i ++) {

		thread = new ZQWorkThread(*this);
		thread->setDefStack(_defThreadStack);
		addThread(thread);

		if (!thread->start()) {
			_threads.erase(thread);
			_TRACE("_threads.erase(): %d\n", _threads.size());
			break;
		}
	}
}

void ZQThreadPool::addThread(ZQWorkThread* thread)
{	
	_threads.insert(thread);
	_TRACE("addThread(): %d\n", _threads.size());
}

bool ZQThreadPool::removeThread(ZQWorkThread* thread)
{
	AbsAutoLock autoLock(wlock());

	if (!_stopped && _threads.size() <= _initSize) {
		return false;
	}

	_threads.erase(thread);
	size_t threadCount = _threads.size();
	if (threadCount == 0)		
		SetEvent(_stopEvent);

	_TRACE("removeThread(): %d\n", threadCount);

	return true;
}

inline bool ZQThreadPool::needGrow()
{
	bool result = _incFlag && _threads.size() < _activeCount + _increment;
	_TRACE("needGrow(): _threads.size() = %d, _activeCount = %d, "
		"_increment = %d, _incFlag = %d, result = %d\n", 
		_threads.size(), _activeCount, _increment, _incFlag, result);

	return result;
}

void ZQThreadPool::onThreadActivate(ZQWorkThread* thread)
{
	AbsAutoLock autoLock(wlock());

	_activeCount ++;
	_TRACE("onThreadActivate(): %d\n", _activeCount);

	size_t size;
	size = _threads.size();
	
	if (needGrow()) {
		size_t inc;

		inc = _maxSize != 0 && size + _increment > _maxSize ? 
			_maxSize - size : _increment;
		
		if (inc > 0) {
			
			createThread(inc);
			return;
		}
	}
}

void ZQThreadPool::onThreadDeactivate(ZQWorkThread* thread)
{
	AbsAutoLock autoLock(wlock());
	_activeCount --;
	_TRACE("onThreadDeactivate(): %d\n", _activeCount);
}

inline void ZQThreadPool::setDefThreadStack(size_t defThreadStack)
{
	_defThreadStack = defThreadStack;
}

inline size_t ZQThreadPool::getDefThreadStack()
{
	return _defThreadStack;
}

} // namespace ZQ {
} // namespace common {
