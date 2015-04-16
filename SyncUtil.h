#pragma once

#include "rwlock.h"

namespace ZQ {
namespace common {

class AbstractLock {
public:
	virtual void lock() const = 0;
	virtual void unlock() const = 0;
};

class LightLock: public AbstractLock {

public:
	LightLock()
	{
		InitializeCriticalSection(&_cs);
	}

	virtual ~LightLock()
	{
		DeleteCriticalSection(&_cs);
	}

	virtual void lock() const
	{
		EnterCriticalSection(&_cs);
	}

	virtual void unlock() const
	{
		LeaveCriticalSection(&_cs);
	}

protected:
	mutable CRITICAL_SECTION	_cs;
};

template <class Lock>
class SimpleAutoLock {
public:
	SimpleAutoLock(const Lock& lock):
	  _lock(lock)
	{
		lock.lock();
	}

	~SimpleAutoLock()
	{
		_lock.unlock();
	}

protected:
	const Lock&		_lock;
};

typedef ZQ::common::SimpleAutoLock< AbstractLock > AbsAutoLock;

class XRWLockEx: public XRWLock {

public:
	XRWLockEx(const uint32 timeo = INFINITE):
	  _rlock(*this), _wlock(*this), _timeo(timeo)
	{

	}

	AbstractLock& rlock() const
	{
		return _rlock;
	}

	AbstractLock& wlock() const
	{
		return _wlock;
	}

protected:

	class RLock: public AbstractLock {
	public:
		RLock(XRWLockEx& rwlock):
		  _rwlock(rwlock)
		{

		}

		virtual void lock() const
		{
			_rwlock.lockRead(_rwlock._timeo);
		}

		virtual void unlock() const
		{
			_rwlock.unlockRead();
		}

	protected:
		XRWLockEx&	_rwlock;

	} mutable _rlock;

	class WLock: public AbstractLock {
	public:
		WLock(XRWLockEx& rwlock):
		  _rwlock(rwlock)
		{

		}

		virtual void lock() const
		{
			_rwlock.lockWrite(_rwlock._timeo);
		}

		virtual void unlock() const
		{
			_rwlock.unlockWrite();
		}

	protected:
		XRWLockEx&	_rwlock;

	} mutable _wlock;

	uint32		_timeo;

	friend class XRWLockEx::RLock;
	friend class XRWLockEx::WLock;

};

#define WQ_NONFULL_EVENT		0
#define WQ_QUEUE_MUTEX			1
#define WQ_NONEMPTY_EVENT		2
#define WQ_QUEUE_MUTEX2			3

template<class Type, class Container = std::deque<Type> >
	class queue_ex: public std::queue<Type, Container> {

public:
	bool exist(const Type& t) 
	{
		return std::find(c.begin(), c.end(), t) != c.end();
	}
};

template<typename T>
class WaitableQueue {
public:
	WaitableQueue(size_t size = 0, bool unique = false):
	  _unique(unique)
	{
		_waitableHandles[WQ_QUEUE_MUTEX] = CreateMutex(NULL, FALSE, NULL);

		if (size) {
			_waitableHandles[WQ_NONFULL_EVENT] = 
				CreateEvent(NULL, TRUE, TRUE, NULL);
		} else
			_waitableHandles[WQ_NONFULL_EVENT] = NULL;

		_waitableHandles[WQ_NONEMPTY_EVENT] = 
			CreateEvent(NULL, TRUE, FALSE, NULL);

		_waitableHandles[WQ_QUEUE_MUTEX2] = 
			_waitableHandles[WQ_QUEUE_MUTEX];

		_maxSize = size;
	}

	virtual ~WaitableQueue()
	{
		CloseHandle(_waitableHandles[WQ_NONFULL_EVENT]);
		CloseHandle(_waitableHandles[WQ_QUEUE_MUTEX]);		
		CloseHandle(_waitableHandles[WQ_NONEMPTY_EVENT]);
	}

	void lock() const
	{
		WaitForSingleObject(_waitableHandles[WQ_QUEUE_MUTEX], INFINITE);
	}

	void unlock() const
	{
		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
	}

	bool push(const T& item, DWORD timeo = INFINITE)
	{
		if (_maxSize) {

			if (WaitForMultipleObjects(2, &_waitableHandles[WQ_NONFULL_EVENT], 
				TRUE, timeo) != WAIT_OBJECT_0) {

				return false;
			}

		} else {

			if (WaitForSingleObject(_waitableHandles[WQ_QUEUE_MUTEX], 
				timeo) != WAIT_OBJECT_0) {

				return false;
			}
		}
	
		size_t size = _queue.size();

		if (_maxSize && size >= _maxSize)
			return false;

		if (_unique) {
			if (_queue.exist(item)) {
				ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
				return false;
			}
		}

		_queue.push(item);
		
		size ++;

		if (_maxSize && size >= _maxSize) {
            ResetEvent(_waitableHandles[WQ_NONFULL_EVENT]);
		}

		if (size == 1) {
			SetEvent(_waitableHandles[WQ_NONEMPTY_EVENT]);
		}
		
		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
		return true;
	}

	bool pop(T& item, DWORD timeo = INFINITE)
	{
		DWORD r;
		if ((r = WaitForMultipleObjects(2, &_waitableHandles[WQ_NONEMPTY_EVENT], 
				TRUE, timeo)) != WAIT_OBJECT_0) {

			return false;
		}
	
		size_t size = _queue.size();

		if (size <= 0) {			
			return false;
		}
		
		item = _queue.front();
        _queue.pop();

		size --;

		if (size == 0) {
			ResetEvent(_waitableHandles[WQ_NONEMPTY_EVENT]);
		}

		if (_maxSize && size == _maxSize - 1) {
			SetEvent(_waitableHandles[WQ_NONFULL_EVENT]);
		}

		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
		return true;
	}

	size_t size()
	{
		_AutoLock cs(*this);
		return _queue.size();
	}

	bool empty()
	{
		_AutoLock cs(*this);
		return _queue.empty();
	}

protected:
	typedef SimpleAutoLock<WaitableQueue> _AutoLock;
	HANDLE				_waitableHandles[4];
	size_t				_maxSize;
	queue_ex<T>			_queue;
	bool				_unique;
};

template<class _Ty, class _C = std::vector<_Ty>,
	class _Pr = std::less<_C::value_type> >
	class priority_queue_ex: public std::priority_queue<_Ty, _C, _Pr> {

public:
	bool exist(const _Ty& t) 
	{
		return std::find(c.begin(), c.end(), t) != c.end();
	}
};

template<typename T, typename Comp = std::less<T> >
class WaitablePriQueue {
public:
	WaitablePriQueue(size_t size = 0, bool unique = false):
	  _unique(unique)
	{
		_waitableHandles[WQ_QUEUE_MUTEX] = CreateMutex(NULL, FALSE, NULL);

		if (size) {
			_waitableHandles[WQ_NONFULL_EVENT] = 
				CreateEvent(NULL, TRUE, TRUE, NULL);
		} else
			_waitableHandles[WQ_NONFULL_EVENT] = NULL;

		_waitableHandles[WQ_NONEMPTY_EVENT] = 
			CreateEvent(NULL, TRUE, FALSE, NULL);

		_waitableHandles[WQ_QUEUE_MUTEX2] = 
			_waitableHandles[WQ_QUEUE_MUTEX];

		_maxSize = size;
	}

	virtual ~WaitablePriQueue()
	{
		CloseHandle(_waitableHandles[WQ_NONFULL_EVENT]);
		CloseHandle(_waitableHandles[WQ_QUEUE_MUTEX]);		
		CloseHandle(_waitableHandles[WQ_NONEMPTY_EVENT]);
	}

	void lock() const
	{
		WaitForSingleObject(_waitableHandles[WQ_QUEUE_MUTEX], INFINITE);
	}

	void unlock() const
	{
		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
	}

	bool push(const T& item, DWORD timeo = INFINITE)
	{
		if (_maxSize) {

			if (WaitForMultipleObjects(2, &_waitableHandles[WQ_NONFULL_EVENT], 
				TRUE, timeo) != WAIT_OBJECT_0) {

				return false;
			}

		} else {

			if (WaitForSingleObject(_waitableHandles[WQ_QUEUE_MUTEX], 
				timeo) != WAIT_OBJECT_0) {

				return false;
			}
		}
	
		size_t size = _queue.size();

		if (_maxSize && size >= _maxSize)
			return false;

		if (_unique) {
			if (_queue.exist(item)) {
				ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
				return false;
			}
		}

		_queue.push(item);
		
		size ++;

		if (_maxSize && size >= _maxSize) {
            ResetEvent(_waitableHandles[WQ_NONFULL_EVENT]);
		}

		if (size == 1) {
			SetEvent(_waitableHandles[WQ_NONEMPTY_EVENT]);
		}
		
		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
		
		return true;
	}

	bool pop(T& item, DWORD timeo = INFINITE)
	{	
		DWORD r;
		if ((r = WaitForMultipleObjects(2, &_waitableHandles[WQ_NONEMPTY_EVENT], 
				TRUE, timeo)) != WAIT_OBJECT_0) {

			return false;
		}
	
		size_t size = _queue.size();

		if (size <= 0) {			
			return false;
		}
		
		item = _queue.top();
        _queue.pop();

		size --;

		if (size == 0) {
			ResetEvent(_waitableHandles[WQ_NONEMPTY_EVENT]);
		}

		if (_maxSize && size == _maxSize - 1) {
			SetEvent(_waitableHandles[WQ_NONFULL_EVENT]);
		}

		ReleaseMutex(_waitableHandles[WQ_QUEUE_MUTEX]);
		return true;
	}

	size_t size() const
	{
		_AutoLock cs(*this);
		return _queue.size();
	}

	bool empty()
	{
		_AutoLock cs(*this);
		return _queue.empty();
	}

protected:

	typedef SimpleAutoLock<WaitablePriQueue> _AutoLock;
	HANDLE				_waitableHandles[4];
	size_t				_maxSize;
	priority_queue_ex<T, std::vector<T>, Comp>	_queue;
	bool				_unique;
};

template<typename T>
class RawQueue {
protected:
	void rawPush(const T& x)
	{
		_queue.push(x);
	}

	void rawPop(T& item)
	{
		item = _queue.front();
        _queue.pop();
	}

	size_t rawSize() const
	{
		return _queue.size();
	}

protected:
	std::queue<T>		_queue;
};

template<typename T, typename Comp = std::less<T> >
class RawPriQueue {
protected:
	void rawPush(const T& x)
	{
		_queue.push(x);
	}

	void rawPop(T& item)
	{
		item = _queue.top();
        _queue.pop();
	}

	size_t rawSize() const
	{
		return _queue.size();
	}

protected:
	std::priority_queue<T, std::vector<T>, Comp>	_queue;
};

template<typename T, typename Q = RawQueue<T> >
class WaitableQueue2: protected Q {

public:
	class QueueGroup;
	friend QueueGroup;

public:
	WaitableQueue2(size_t size = 0):
	  _maxSize(size)
	{
		assert(size);
		InitializeCriticalSection(&_cs);
		_usedSem = CreateSemaphore(NULL, 0, (LONG)size, NULL);
		assert(_usedSem);
		_unusedSem = CreateSemaphore(NULL, (LONG)size, (LONG)size, NULL);
		assert(_unusedSem);
	}

	virtual ~WaitableQueue2()
	{
		DeleteCriticalSection(&_cs);
		CloseHandle(_usedSem);
		CloseHandle(_unusedSem);
	}

	void lock() const
	{
		EnterCriticalSection(&_cs);
	}

	void unlock() const
	{
		LeaveCriticalSection(&_cs);
	}

	bool push(const T& item, DWORD timeo = INFINITE)
	{
		DWORD wr = WaitForSingleObject(_unusedSem, timeo);
		if (wr != WAIT_OBJECT_0) {
			
			// assert (wr != WAIT_FAILED);
			return false;
		}
		
		bool r = ipush(item, timeo);
		if (!r) {
			ReleaseSemaphore(_unusedSem, 1, NULL);
		}

		return r;
	}

	bool pop(T& item, DWORD timeo = INFINITE)
	{
		DWORD wr = WaitForSingleObject(_usedSem, timeo);
		if (wr != WAIT_OBJECT_0) {

			// assert (wr != WAIT_FAILED);
			return false;
		}

		bool r = ipop(item, timeo);
		if (!r) {
			ReleaseSemaphore(_unusedSem, 1, NULL);
		}

		return r;
	}

	size_t size() const
	{
		_AutoLock lock(*this);
		return rawSize();
	}

	bool empty()
	{
		_AutoLock lock(*this);
		return _queue.empty();
	}

public:

	class QueueGroup {
		friend WaitableQueue2;
	protected:
		QueueGroup(WaitableQueue2* queues[], size_t size, 
			bool forPop = true, bool forPush = true): 
		  _size(size)
		{
			assert(forPop | forPush);
			
			HANDLE* handles;

			if (forPop) {
				_usedHandles = (HANDLE* )malloc(sizeof(HANDLE) * size);
				handles = _usedHandles;

				for (size_t i = 0; i < size; i ++) {

					WaitableQueue2* queue = queues[i];
					queue->lock();
					handles[i] = queue->_usedSem;
					queue->unlock();
				}

			} else {

				_usedHandles = NULL;
			}

			if (forPush) {
				_unusedHandles = (HANDLE* )malloc(sizeof(HANDLE) * size);

				handles = _unusedHandles;

				for (size_t i = 0; i < size; i ++) {

					WaitableQueue2* queue = queues[i];
					queue->lock();
					handles[i] = queue->_unusedSem;
					queue->unlock();
				}
			} else {
				_unusedHandles = NULL;
			}

			_queues = (WaitableQueue2** )malloc(
				sizeof(WaitableQueue2* ) * size);

			memcpy(_queues, queues, sizeof(WaitableQueue2*) * size);
		}

		virtual ~QueueGroup()
		{
			if (_usedHandles) {
				free(_usedHandles);
			}

			if (_unusedHandles) {
				free(_unusedHandles);
			}

			free(_queues);
		}

	public:
		DWORD pop(T& item, DWORD timeo)
		{
			if (_usedHandles == NULL) {
				assert(false);
				return -1;
			}

			DWORD r = WaitForMultipleObjects(_size, _usedHandles, 
				FALSE, timeo);

			DWORD index;

			if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + _size) {

				index = r - WAIT_OBJECT_0;
			} else {
				return -1;
			}
			
			if (!_queues[index]->ipop(item, timeo)) {
				ReleaseSemaphore(_usedHandles[index], 1, NULL);
			}

			return index;
		}

		DWORD push(const T& item, DWORD timeo = INFINITE)
		{
			if (_unusedHandles == NULL) {
				assert(false);
				return -1;
			}

			DWORD r = WaitForMultipleObjects(_size, _unusedHandles, 
				FALSE, timeo);

			DWORD index;

			if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + _size) {

				index = r - WAIT_OBJECT_0;
			} else {
				return -1;
			}
			
			if (!_queues[index]->ipush(item, timeo)) {
				ReleaseSemaphore(_usedHandles[index], 1, NULL);
			}

			return index;
		}

		DWORD preparePush(DWORD timeo = INFINITE)
		{
			if (_unusedHandles == NULL) {
				assert(false);
				return -1;
			}

			DWORD r = WaitForMultipleObjects(_size, _unusedHandles, 
				FALSE, timeo);

			if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + _size) {

				return r - WAIT_OBJECT_0;
			} else {
				return r;
			}
		}

		bool cancelPush(DWORD index)
		{
			return ReleaseSemaphore(_unusedHandles[index], 1, NULL) == TRUE;
		}

		bool doPush(T& item, DWORD index)
		{
			if (!_queues[index]->ipush(item, INFINITE)) {
				ReleaseSemaphore(_usedHandles[index], 1, NULL);
				return false;
			}

			return true;
		}

		DWORD preparePop(DWORD timeo = INFINITE)
		{
			if (_usedHandles == NULL) {
				assert(false);
				return -1;
			}

			DWORD r = WaitForMultipleObjects(_size, _usedHandles, 
				FALSE, timeo);

			if (r >= WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + _size) {

				return r - WAIT_OBJECT_0;
			} else {
				return r;
			}
		}

		bool cancelPop(DWORD index)
		{
			return ReleaseSemaphore(_usedHandles[index], 1, NULL) == TRUE;
		}

		bool doPop(const T& item, DWORD index)
		{
			if (!_queues[index]->ipop(item, INFINITE)) {
				ReleaseSemaphore(_usedHandles[index], 1, NULL);
				return false;
			}

			return true;
		}

	protected:		
		HANDLE*				_usedHandles;
		HANDLE*				_unusedHandles;
		WaitableQueue2**	_queues;
		size_t				_size;
	};


	static QueueGroup* createQueueGroup(WaitableQueue2* queues[], 
		size_t queueNum, bool forPop = true, bool forPush = true)
	{

		QueueGroup* qg = new QueueGroup(queues, queueNum, forPop, 
			forPush);

		return qg;
	}

	static void closeQueueGroup(QueueGroup* queueGroup)
	{
		delete queueGroup;
	}
	

protected:

	bool ipop(T& item, DWORD timeo = INFINITE)
	{
		_AutoLock lock(*this);

		rawPop(item);

		BOOL res = ReleaseSemaphore(_unusedSem, 1, NULL);
		if (!res) {			
			assert(false);
			return false;
		}

		return true;
	}

	bool ipush(const T& item, DWORD timeo = INFINITE)
	{
		_AutoLock lock(*this);

		rawPush(item);

		BOOL res = ReleaseSemaphore(_usedSem, 1, NULL);
		if (!res) {			
			assert(false);
			return false;
		}

		return true;
	}

protected:
	typedef SimpleAutoLock<WaitableQueue2> _AutoLock;
	mutable CRITICAL_SECTION	_cs;
	HANDLE				_usedSem;
	HANDLE				_unusedSem;
	size_t				_maxSize;
	
};

} // namespace common {

} // namespace ZQ {
