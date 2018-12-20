// ============================================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai,,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of syscheme Possession, use, duplication or dissemination of the
// software and media is authorized only pursuant to a valid written license from ZQ Interactive,
// Inc.
// This source was copied from shcxx, shcxx's copyright is belong to Hui Shao
//
// This software is furnished under a  license  and  may  be used and copied only in accordance
// with the terms of  such license and with the inclusion of the above copyright notice.  This
// software or any other copies thereof may not be provided or otherwise made available to any
// other person.  No title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and should not be
// construed as a commitment by syscheme
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : define a pool for the native objected thread
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/NativeThreadPool.h 3     7/20/11 2:07p Hongquan.zhang $
// $Log: /ZQProjs/Common/NativeThreadPool.h $
// 
// 3     7/20/11 2:07p Hongquan.zhang
// 
// 2     10-11-24 15:55 Hui.shao
// for the stuck on TcpWatchDog's quit
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 21    10-08-16 15:32 Fei.huang
// * fix deadlock by the lock to modify activeCount
// 
// 20    09-03-19 12:30 Hongquan.zhang
// 
// 19    09-03-09 15:31 Hongquan.zhang
// 
// 18    09-03-09 14:49 Yixin.tian
// 
// 17    09-03-06 16:24 Hongquan.zhang
// 
// 16    09-03-06 16:21 Hongquan.zhang
// 
// 15    09-03-06 11:53 Hongquan.zhang
// use HighPerfPriorityQueue instead of std::priority_queue due to bug and
// low efficiency
// 
// 14    08-12-09 14:36 Yixin.tian
// 
// 13    07-12-21 16:25 Jie.zhang
// 
// 12    07-11-29 11:51 Hui.shao
// 
// 12    07-11-29 11:49 Hui.shao
// 
// 11    07-11-29 11:44 Hui.shao
// 
// 10    07-09-03 14:39 Hongquan.zhang
// ============================================================================================

#ifndef __ZQ_Common_NativeThreadPool_h__
#define __ZQ_Common_NativeThreadPool_h__

#include "ZQ_common_conf.h"
#include "NativeThread.h"
#include "Locks.h"
#include "TimeUtil.h"

#include <vector>
#include <queue>
#include <list>
#include <map>

namespace ZQ {
namespace common {

class ZQ_COMMON_API ThreadRequest;
class ZQ_COMMON_API NativeThreadPool;

#define DEFAULT_THRPOOL_SZ		(10)
#define MAX_THRPOOL_SZ			(500)
#define DEFAULT_REQUEST_PRIO	(128)

// -----------------------------
// class SlaveThread
// -----------------------------
class SlaveThread : public NativeThread
{
	friend class NativeThreadPool;
	friend class StopRequest;
	friend class CleanRequest;

	SlaveThread(NativeThreadPool& Pool);
	
	void quit();
	const bool isActive() const { return _bActive; }
	
	NativeThreadPool& _pool; 
	virtual int run(void);
	virtual void final();
	
	ThreadRequest* _pReq;
	bool _bQuit;
	bool _bActive;
#ifndef ZQ_OS_MSWIN
	pthread_mutex_t _pthmutex;
#endif
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
template<typename T ,class _Pr = std::less<uint8>  >
class HighPerfPriority_Queue
{
public:
	HighPerfPriority_Queue( )
	{
		mObjCount = 0;		
	}

	virtual ~HighPerfPriority_Queue( )
	{

	}

public:
	void		push( const  T& obj )
	{
		uint8 level = (uint8)obj->getPriority();
		typename OBJMAP::iterator itMap = mObjMap.find(level);
		if( itMap != mObjMap.end() )
		{
			OBJLIST& l = itMap->second;
			l.push_back(obj);
		}
		else
		{
			OBJLIST l;
			l.push_back(obj);
			mObjMap.insert(typename OBJMAP::value_type(level,l) );
		}
		++ mObjCount ;		
	}
	const T&			top( ) const
	{
		typename OBJMAP::const_iterator it = mObjMap.begin();
		return it->second.front();
		
	}
	void		pop( )
	{
		typename OBJMAP::iterator it = mObjMap.begin();		
		it->second.pop_front();
		--mObjCount;
		if( it->second.size() <= 0 )
			mObjMap.erase(it);
	}
	size_t		size( ) const
	{
		return	 mObjCount;
	}
	bool		empty( ) const
	{
		return mObjCount <= 0;
	}
protected:
	
	typedef std::list<T>	OBJLIST;
	

private:

	typedef std::map< uint8 , OBJLIST , _Pr > OBJMAP;
	OBJMAP					mObjMap;
	size_t					mObjCount;	
};

// -----------------------------
// class ThreadRequest
// -----------------------------
/// The base Thread class supports encapsulation of the generic threading methods implemented
/// on various target operating systems. This includes the ability to start and stop threads
/// in a synchronized and controllable manner, the ability to specify thread execution
/// priority, and thread specific "system call" wrappers, such as for sleep and yield. A
/// thread exception is thrown if the thread cannot be created. Derived from the class Thread,
/// threads can be as objects. 1) At minimum the "Run" method must be implemented, and this
/// method essentially is the "thread", for it is executed within the execution context of
/// the thread, and when the Run method terminates the thread is assumed to have terminated.
/// 2) Threads can be "suspended" and "resumed". As this behavior is not defined in the Posix 
/// pthread specification, it is often emulated through signals.
class ThreadRequest : protected Mutex
{
	friend class SlaveThread;
	friend class NativeThreadPool;

public:
	
	///constructor
	///@param Pool the pool this request apply to
	ThreadRequest(NativeThreadPool& Pool);
	
	///destructor
	virtual ~ThreadRequest();

protected:
	
	/// The initial method is called by a thread when it execute the request. This
	/// method is ran with deferred cancellation disabled by default. The Initial method is
	/// given a separate handler so that it can create temporary objects on it's own stack
	/// frame, rather than having objects created on run() that are only needed by startup and
	/// yet continue to consume stack space.
	///@return true if pass the initialization steps
	virtual bool init(void)	{ return true; };
	
	/// All thread request execute by deriving the Run method. This method is called after
	/// initial to begin normal operation of the request. If the method terminates then the
	/// thread will notify the request with return code of this method in final() method.
	///@return the return value will also be passed to final()
	virtual int run(void) {return 0; }
	
	/// A thread that is self terminating, either by invoking exit() or leaving its run(),
	/// will have this method called. It can be used to self delete the current object assuming
	/// the object was created with new on the heap rather than stack local. You can safe
	/// delete thread via "delete this"in final, but should exit ASAP
	///@param retcode the return value of run()
	///@param bTerminated  true if the request was cancelled by the execution thread
	virtual void final(int retcode =0, bool bCancelled =false) {}
	
public:
	
	/// Get priority of the request. Mostly be evaluted by the ThreadPool when
	/// it queues this request. The requst will be insert before the first item
	/// that has lower priority, Child class may override this method to define
	/// their own priority evaluation
	/// @return priority value, 0 is the highest while 255 is the lowest
	const uint8 getPriority(void) const { return _priority; }

	/// When a new request is created, it does not begin immediate put into the request queue of
	/// the threadpool, because the derived class virtual tables are not properly loaded at the
	/// time the C++ object is created within the constructor itself, at least in some
	/// compiler/system combinations. The request can either be told to wait for an external
	/// semaphore, or it can be queued directly after the constructor completes by calling the
	/// start() method.
	/// @return error code if execution fails.
	virtual bool start();
	
	/// Gets the status code of the current request object.
	/// @return a status code
	inline NativeThread::status_t getStatus(void)	const { return _status; }
	
	/// Gets the id of the host thread object.
	/// @return  thread id
	uint32 threadId(void) const;
	
	/// Verifies if the thread is still running or has already been terminated but not yet
	/// deleted.
	/// @return true if the thread is still executing.
	bool isRunning(void) { return _status == NativeThread::stRunning; }
	
	void setPriority(uint8 priority) { _priority = priority;}
	

protected:
	
	NativeThread::status_t _status;
	SlaveThread* _pThr;
	NativeThreadPool& _pool;
	uint8 _priority;	

// 	struct greater : std::binary_function<ThreadRequest*, ThreadRequest*, bool>
// 	{
// 		bool operator()(const ThreadRequest* _X, const ThreadRequest* _Y) const
// 		{
// // 			return (NULL != _X && NULL != _Y && 
// // 				_X->getPriority() > _Y->getPriority() );
// 			if( _X->getPriority() > _Y->getPriority())
// 				return true;
// 			else if( _X->getPriority() == _Y->getPriority() )
// 			{
// 				return _X->getReqId() > _Y->getReqId();
// 			}
// 			else
// 			{
// 				return false;
// 			}
// 		}
// 	};
};

// -----------------------------
// class NativeThreadPool
// -----------------------------
class NativeThreadPool : protected Mutex, Semaphore
{
	friend class SlaveThread;
	friend class ThreadRequest;
	friend class CleanRequest;

public:
	
	NativeThreadPool(const unsigned int size =DEFAULT_THRPOOL_SZ);
	virtual ~NativeThreadPool();
	
	/// get the total number of the threads in the pool
	///@return the number of total threads in the pool
	int size() const {	return (int) _SThrPool.size(); }
	
	/// change the total number of the threads in the pool
	void resize(int newSize);

	/// get the number of the active threads in the pool
	///@return the number of active threads in the pool
	int activeCount() const { return (int) _activeCount; }

	// clear all pending requests
	void clearRequests();

	/// disable the threads in the pool
	void stop();

	//get current pending request size
	const int pendingRequestSize();

	int64 getBusyTime( );

protected:
	
	// only for ThreadRequest
	virtual void pushRequest(ThreadRequest& request);
	
	typedef std::vector<SlaveThread* > sthr_vector;
	sthr_vector _SThrPool;
	
    //typedef std::priority_queue <ThreadRequest*, std::vector<ThreadRequest*>, ThreadRequest::greater > req_queue;
	typedef HighPerfPriority_Queue<ThreadRequest*> req_queue;

	req_queue  _requests;
	size_t	   _resizeCount;
	volatile long	   _activeCount;

	bool		_running;	
	Mutex _countMutex;
	
	int64 _lastIdle;

};

} // namespace common
} // namespace ZQ

#endif // __ZQ_Common_NativeThreadPool_h__
