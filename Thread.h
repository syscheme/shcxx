// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: Thread.h,v 1.14 2004/06/30 07:19:00 jshen Exp $
// Branch: $Name:  $
// Author: Meng.Wang
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Thread.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 6     08-03-06 16:45 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 4     05-04-11 16:21 Hongye.gu
// 
// 3     04-11-04 10:38 Jie.zhang
// 
// 2     04-11-03 11:30 Jie.zhang
// Revision 1.14  2004/06/30 07:19:00  jshen
// remove resetThreadPool
//
// Revision 1.13  2004/06/27 06:28:10  wli
// Add ndefine verify for WIN32_LEAN_AND_MEAN
//
// Revision 1.12  2004/06/17 05:26:44  jshen
// no message
//
// Revision 1.11  2004/06/17 04:07:44  jshen
// no message
//
// Revision 1.10  2004/06/16 14:14:29  mwang
// add const restrict
//
// Revision 1.9  2004/06/16 02:44:56  shao
// simplized ThreadPool
//
// Revision 1.8  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.7  2004/05/14 04:14:30  shao
// not to export detail operations from ThreadPool
//
// Revision 1.6  2004/05/14 03:51:44  shao
// code reviewed, added init() routine, moved pool here, selectable pool to adapt
//
// ===========================================================================

#ifndef __ZQ_COMMON_THREAD_H__
#define __ZQ_COMMON_THREAD_H__
#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>
#endif

namespace ZQ{
namespace common{

class ZQ_COMMON_API Thread;
class ZQ_COMMON_API ThreadPool;

extern ThreadPool gThreadPool;

#define POOL_RELEASE_WAIT 100 // in msec
#define DEFAULT_THREAD_START_TIMEOUT 1000 // in msec

// -----------------------------
// class Thread
// -----------------------------
/// represents a thread of execution within an application
class Thread
{
public:
	enum Status	
	{
		///the thread is initial
		stInitial		=1,  
		///the thread is running
		stRunning,	
		///the thread is disabled
		stDisabled,	
	};
	
	/// default constructor
	/// @param pool    where the thread pooled, each application can
	///                maintain its own pool or use the global one
	Thread(ThreadPool& pool = gThreadPool)
		: _pool(pool), _handle(NULL), _status(stInitial)
		{}
	
	///default destuctor
	virtual ~Thread()
		{ if(_handle!=NULL) close(); }

	/// get the pool
	ThreadPool* pool() { return &_pool; }

	/// reset the thread pool
//	void ResetThreadPool(ThreadPool& new_pool); 

	///return the thread handle, use for waitformultiobjects()
	HANDLE handle(){ return _handle;}

	///Starts execution!
	bool start();

	static void Sleep(timeout_t msec);

	///retrieve the current thread status
	Status status(void)const{ return _status; }

	///set the status of the current thread
	void setStatus(int sts);

	///query if the thread is still running.
	bool isRunning(void)const{ return _status==stRunning;}

	///wait until the thread is ended or timeout
	///@param timeoutMillis timeout
	///@return return false only if timeout, other return true means thread have quit.
	bool wait(DWORD timeoutMillis=INFINITE)const;

	///close the job handle.
	void close();

protected:
	
	/// the initial steps can be put here after start() is called
	virtual bool init(void)	{ return true; };

	/// should be Overrided! main steps for thread
	virtual int run() =0;

	/// you can do some clean work here, this functions is run in the thread.
	virtual void final() {}

private:

	static DWORD WINAPI _execute(void *pVoid)
	{
		DWORD result=0;
		
		Thread* pThis = (Thread*)pVoid;

		try
		{
			if (pThis ==NULL || !pThis->init())
				return result;
			
			pThis->_status=stRunning;
			result=pThis->run();

			pThis->_status=stDisabled;
		}
		catch (...) {}
		
		try
		{
			pThis->final();
		}
		catch (...){}

     // if User call delete this in final() this will cause 
	 // access violation exception
		//pThis->_status=stDisabled;

		return result;
	}

	HANDLE _handle; ///thread handle
	ThreadPool& _pool;

	Status _status; ///thread status

	/// define the following two constructor WITHOUT implementation to prevent
	/// thread copying!
	Thread(const Thread &);
	Thread &operator=(const Thread &);
};

// -----------------------------
// class ThreadPool
// -----------------------------
/// This class provides a pool of worker threads that process some works..
class ThreadPool
{
public:

	friend class Thread;

	/// constructor
	/// @param needinit --Initialize the thread pool if true.
	/// @param dwNumThreads --The number of threads the thread pool should maintain minimally.
	/// @param dwGrowBy --The number of threads to grow by when the thread pool is exhausted.
	/// @param dwMaxThreads --The maximum number of threads the thread pool can have at one time
	///						This may be INFINITE which will indicate that the system's resource limit
	///						is the limit.  I.e. NT will grind to a halt before I stop creating threads.(INFINITE, Nt can handle it, right?)
	/// @param dwIdleTime --When the current number of threads in the pool exceed dwNumThreads, this is 
	///					how many milliseconds a thread can be idle before it is,released to the system.
	ThreadPool(DWORD dwNumThreads =10, DWORD dwGrowBy =10, DWORD dwMaxThreads =INFINITE, DWORD dwIdleTime =5000);

	/// destructor
	~ThreadPool();

	/// Initialize the thread pool.
	/// @param dwNumThreads --The number of threads the thread pool should maintain minimally.
	/// @param dwGrowBy --The number of threads to grow by when the thread pool is exhausted.
	/// @param dwMaxThreads --The maximum number of threads the thread pool can have at one time
	///						This may be INFINITE which will indicate that the system's resource limit
	///						is the limit.  I.e. NT will grind to a halt before I stop creating threads.(INFINITE, Nt can handle it, right?)
	/// @param dwIdleTime --When the current number of threads in the pool exceed dwNumThreads, this is 
	///					how many milliseconds a thread can be idle before it is,released to the system.
	bool init(DWORD dwNumThreads, DWORD dwGrowBy, DWORD dwMaxThreads =INFINITE, DWORD dwIdleTime =5000);

	/// clean up the current thread pool.
	bool clean();

	///Close the given job handle, users must never call the ZQ_OS_MSWIN ::CloseHandle()
	///@param threadhandle --the job handle to close
	///@note you can only invoke this function when the thread is ended.
	void close_thread_handle(HANDLE threadhandle);

private:

	///the session handle
	HANDLE _handle;
	///The number of threads the thread pool should maintain minimally
	DWORD _numThreads;
	///The number of threads to grow by when the thread pool is exhausted
	DWORD _growBy;
	///The maximum number of threads the thread pool can have at one time
	DWORD _maxThreads;
	///When the current number of threads in the pool exceed dwNumThreads
	DWORD _idleTime;
};


} // namespace common
} // namespace ZQ

#endif __ZQ_COMMON_THREAD_H__