// ============================================================================================
// Copyright (c) 1997, 1998 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of ZQ Interactive, Inc. Possession, use, duplication or dissemination of the
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
// construed as a commitment by ZQ Interactive, Inc.
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : impl a pool for the native objected thread
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/NativeThreadPool.cpp 7     10/17/12 5:45p Hui.shao $
// $Log: /ZQProjs/Common/NativeThreadPool.cpp $
// 
// 7     10/17/12 5:45p Hui.shao
// merged from V1.16
// 
// 6     10/16/12 2:39p Hui.shao
// ticket#12283 to avoid block after linux threads now calls join() prior
// to delete
// 
// 5     7/22/11 11:37a Hui.shao
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
// 23    10-08-16 15:32 Fei.huang
// * fix deadlock by the lock to modify activeCount
// 
// 22    10-07-06 14:54 Hongquan.zhang
// 
// 21    09-12-02 10:28 Fei.huang
// * fix: some threads may not quit properly.
// * fix: do not stop again in dtor
// * fix: wait threads in stop()
// 
// 20    09-05-22 10:18 Fei.huang
// * wait thread before delete 
// 
// 19    09-05-20 18:20 Fei.huang
// * wait for slave threads to terminate
// 
// 18    09-03-06 11:53 Hongquan.zhang
// use HighPerfPriorityQueue instead of std::priority_queue due to bug and
// low efficiency
// 
// 17    08-12-09 14:36 Yixin.tian
// 
// 16    08-03-06 16:24 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 15    08-03-03 17:55 Yixin.tian
// merged changes for linux
// 
// 14    08-02-19 15:59 Hui.shao
// 
// 13    07-12-21 16:25 Jie.zhang
// 
// 12    07-11-29 11:45 Hui.shao
// 
// 11    07-09-03 14:39 Hongquan.zhang
// 
// 9     06-09-07 18:02 Hui.shao
// 
// 8     06-06-14 11:28 Hui.shao
// 
// 7     06-04-25 17:13 Bernie.zhao
// fixed bug when ThreadPool stop() while pushRequest()
// 
// 6     06-03-15 10:40 Cary.xiao
// 
// 5     1/05/06 8:25p Hui.shao
// 
// 4     1/05/06 8:23p Hui.shao
// added pool::resize()
// 
// 3     11/21/05 4:47p Hui.shao
// added priority process
// 
// 2     11/04/05 10:20a Hui.shao
// 
// 111/03/05 9:04p Hui.shao
// ============================================================================================

#include "NativeThreadPool.h"
#include "SystemUtils.h"

namespace ZQ {
namespace common {


SlaveThread::SlaveThread(NativeThreadPool& Pool) 
: _pool(Pool),_pReq(NULL),_bQuit(false),_bActive(false)
{ 
	start();
#ifndef ZQ_OS_MSWIN
	pthread_mutex_init(&_pthmutex,NULL);
#endif
}

void SlaveThread::quit()
{
	_bQuit = true;
}

int SlaveThread::run(void)
{
	while(!_bQuit)
	{
		{

			while(_pool._requests.empty() && !_bQuit)
				_pool.wait();
			
			if(_bQuit)
				return 0;
			
			// dequeu next request
			MutexGuard guard(_pool);

#ifndef ZQ_OS_MSWIN
			int delta = _pool._requests.size();
			delta -= _pool.getValue();
			if( delta > 0 )
			{
				for( int i = 0; i < delta ;i++)
					_pool.post();
			}
#endif	// ZQ_OS_MSWIN

			if (!_pool._requests.empty())
			{
				_pReq = _pool._requests.top();
				_pool._requests.pop();
			} else
				_pReq = NULL;
			_pool._lastIdle = ZQ::common::now();
			
		}
		
		if (NULL == _pReq || _bQuit)
			continue;
		
		// run the request
		int ret = -1;
		_pReq->_pThr = this;
		
		_bActive = true;
#ifdef ZQ_OS_MSWIN
		InterlockedIncrement(&_pool._activeCount);
#else
		{
			ZQ::common::MutexGuard gggggd(_pool._countMutex);
			++_pool._activeCount ;
		}
#endif

		//try {
			if (_pReq->init() && !_bQuit)
			{
				MutexGuard guard(*_pReq);
				_pReq->_status = stRunning;
				ret = _pReq->run();
			}
			
			_pReq->_status = stDisabled;
			_pReq->final(ret);
		//}
		//catch(...) {}

		_bActive = false;
#ifdef ZQ_OS_MSWIN
		InterlockedDecrement(&_pool._activeCount);	
#else
		{
			ZQ::common::MutexGuard gggggd(_pool._countMutex);
			--_pool._activeCount ;
		}
		
#endif

		_pReq = NULL;
	}
	
	return 0;
}

void SlaveThread::final()
{
	if (NULL != _pReq)
	{
		_pReq->_status = stDisabled;
		_pReq->final(-1, true);
	}
	
	_bActive = false;
	_pReq = NULL;
}

// -----------------------------
// class ThreadRequest
// -----------------------------

ThreadRequest::ThreadRequest(NativeThreadPool& Pool)
:_status(NativeThread::stDeferred),_pThr(NULL),_pool(Pool),
_priority(DEFAULT_REQUEST_PRIO)
{
}

ThreadRequest::~ThreadRequest()
{
	MutexGuard guard(*this);
}


uint32 ThreadRequest::threadId(void) const
{
	if (_status == NativeThread::stRunning && NULL != _pThr)
		return _pThr->id();
	return uint32(-1);
}

bool ThreadRequest::start()
{
	_pool.pushRequest(*this);
	return true;
}

// -----------------------------
// class CleanRequest
// -----------------------------
class CleanRequest : public ThreadRequest
{
public:
	CleanRequest(SlaveThread* pThrToClean, NativeThreadPool& Pool)
		: ThreadRequest(Pool), _pThrToClean(pThrToClean)
	{
		ThreadRequest::setPriority(0);
	}

protected:
	int run()
	{
		if (NULL == _pThrToClean)
			return 0;

		// wait til the stop request finishs
		while (NULL != _pThrToClean->_pReq)
#ifdef ZQ_OS_MSWIN
			Sleep(1);
#else
			usleep(1000);
#endif

		delete _pThrToClean;

		MutexGuard guard(_pool);
		for (NativeThreadPool::sthr_vector::iterator it = _pool._SThrPool.begin(); it != _pool._SThrPool.end(); ++it)
		{
			if ((*it) == _pThrToClean)
			{
				_pool._SThrPool.erase(it);
				break;
			}
		}
		return 0;
	}

	void final(int retcode, bool bCancelled)
	{
		delete this;
	}

	SlaveThread* _pThrToClean;
};

// -----------------------------
// class StopRequest
// -----------------------------
class StopRequest : public ThreadRequest
{
public:	
	StopRequest(NativeThreadPool& Pool)
		: ThreadRequest(Pool)
	{
	}
protected:
	int run()
	{
		if (NULL == _pThr)
			return 0;
		
		_pThr->quit();

		ThreadRequest * p = new CleanRequest(_pThr, _pool);
		p->start();
		return 0;
	}

	void final(int retcode, bool bCancelled)
	{
		delete this;
	}
};

// -----------------------------
// class NativeThreadPool
// -----------------------------

#define SMALL_POOL_SZ (2)
NativeThreadPool::NativeThreadPool(const unsigned int size)
 : _resizeCount(size), _activeCount(0), _running(false),_lastIdle(0)
{  
	// allocate slave threads
	int sz = (size<1 ? SMALL_POOL_SZ : size);
	if (sz > MAX_THRPOOL_SZ)
		sz = MAX_THRPOOL_SZ;
	
	for (size_t i = 0; i< (size_t) sz; i++)
	{
		SlaveThread* sthr = new SlaveThread(*this);
		if (NULL == sthr)
			continue;
		
		_SThrPool.push_back(sthr);
	}

	_running = true;
}  

NativeThreadPool::~NativeThreadPool(void)   
{
	// in case the calling thread invoked stop() already
	if(_running)
		stop();

	_SThrPool.clear();
} 

void NativeThreadPool::clearRequests()
{
	MutexGuard guard(*this); // no more new requests...

	//dequeue the pending requests
	while(!_requests.empty())
	{
		ThreadRequest* pReq = _requests.top();
		_requests.pop();

		if (NULL != pReq)
			pReq->final(-2, true);
	}
}

void NativeThreadPool::stop()
{
	_running = false;
	clearRequests();

	sthr_vector tmpThrlList;
	sthr_vector::iterator it;

	{
		MutexGuard tempGd(*this);

		for ( it = _SThrPool.begin(); it < _SThrPool.end(); it++)
		{
			if (NULL != (*it))
			{
				(*it)->quit();
			}
		}

		for (it = _SThrPool.begin(); it < _SThrPool.end(); it++)
		{
			post();
		}

		// duplicate the slave thread list and clear the original list
		tmpThrlList = _SThrPool;
		_SThrPool.clear();
	}

	for (it = tmpThrlList.begin(); it < tmpThrlList.end(); it++)
	{
        if (NULL != (*it)) 
        {
			(*it)->waitHandle(-1);
			delete (*it);
        }
    }
}

void NativeThreadPool::resize(int newSize)
{
	if (!_running)
		return;

	_resizeCount = newSize > MAX_THRPOOL_SZ ? MAX_THRPOOL_SZ : newSize;
	_resizeCount = _resizeCount < SMALL_POOL_SZ ? SMALL_POOL_SZ : _resizeCount;

	while (_running && NativeThreadPool::size() < (int) _resizeCount)
	{
		SlaveThread* sthr = new SlaveThread(*this);
		if (NULL != sthr)
		{
			MutexGuard guard(*this);
			if (_running)
			{
				_SThrPool.push_back(sthr);
				post();
			}
			else delete sthr;
		}
	}

	for (size_t sz = NativeThreadPool::size(); _running && sz > _resizeCount; sz--)
	{
		ThreadRequest* p = new StopRequest(*this);
		p->start();
	}
}

void NativeThreadPool::pushRequest(ThreadRequest& request)  
{  
	if(!_running)	// if pool is stopped, do not permit post
		return;

	// reset request status
	request._status = NativeThread::stInitial;
	
	{
		MutexGuard guard(*this);
		if(!_running)	// if pool is stopped, do not permit post
			return;

		_requests.push(&request);
	}

	post(); // increase the Semaphore
}  

const int NativeThreadPool::pendingRequestSize()
{
	if (!_running)
		return 0;

	MutexGuard guard(*this);
	return (int) _requests.size();
}

int64 NativeThreadPool::getBusyTime( )
{
	if(!_running || activeCount() != _SThrPool.size() || _lastIdle<= 0 )
		return 0;

	return ZQ::common::now() - _lastIdle;
}

} // namespace common
} // namespace ZQ
