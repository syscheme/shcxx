// ============================================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai
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
// Desc  : define a native objected thread
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/NativeThread.h 1     10-11-12 15:56 Admin $
// $Log: /ZQProjs/Common/NativeThread.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 18    10-11-04 14:17 Hongquan.zhang
// 
// 17    10-09-16 18:17 Xiaohui.chai
// 
// 16    09-12-16 15:21 Fei.huang
// 
// 16    09-12-08 14:16 Fei.huang
// * use one thread id only, not to confused with Windows version
// 
// 15    09-09-24 17:14 Fei.huang
// * fix start() might be invoked twice or above that cause waitHandle
// misbehaved
// 
// 14    09-08-07 14:36 Yixin.tian
// 
// 13    09-05-15 16:44 Fei.huang
// 
// 12    09-05-11 18:30 Fei.huang
// * remove "suspend" "resume"
// * rewrite waitHandle
// * use pthread_cond_wait to replace sem_wait
// * fix some issue
// 
// 11    08-03-06 16:24 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 10    08-03-03 17:54 Yixin.tian
// merged changes for linux
// 
// 4     05-06-15 20:26 Bernie.zhao
// made waitHandle() public
// 
// 3     05-04-22 12:12 Hongye.gu
// 
// 2     05-04-21 10:44 Hongye.gu
// 
// 1     4/14/05 6:50p Hui.shao
// ============================================================================================

#ifndef __ZQ_Common_NativeThread_h__
#define __ZQ_Common_NativeThread_h__

#include "ZQ_common_conf.h"

extern "C" {

#ifndef ZQ_OS_MSWIN
#  include <pthread.h>
#  include <semaphore.h>
#endif

#ifdef _DEBUG
#  include <stdio.h>
#endif

}


namespace ZQ {
namespace common {

class ZQ_COMMON_API NativeThread;

// -----------------------------
// class NativeThread
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
class NativeThread
{
public:

	typedef enum ThreadStatus
	{
		stDeferred	=0,
		stInitial		=1,	
		stRunning,
		stDisabled,
		stDefault     =stDeferred
	} status_t;

#ifdef ZQ_OS_MSWIN
	typedef  unsigned ThreadId_t;
#else
	typedef  pthread_t ThreadId_t;
#endif

#ifdef ZQ_OS_MSWIN
	NativeThread(int stacksize =0);
#else
//@stacksize set the thread stack size(in bytes)
	NativeThread(int stacksize =2*1024000);
#endif

	virtual ~NativeThread();
	
private:

	ThreadId_t _thrdID;
	status_t   _status;

	// log information
	int _msgpos;
	char _msgbuf[128];
 

	union _flag
	{
		struct
		{
			bool active :1;
		} b;

		uint32 B;
	} _flags;

	bool	_bCancel;

#ifdef ZQ_OS_MSWIN
//	static unsigned long __stdcall _execute(void *th);
	static unsigned __stdcall _execute(void *th);

	HANDLE	_hThread;	
#else
	static void* _execute(void* th);

    sem_t _suspend;
	sem_t _thsem;

#endif

protected:

	/// The initial method is called by a newly created thread when it starts execution. This
	/// method is ran with deferred cancellation disabled by default. The Initial method is
	/// given a separate handler so that it can create temporary objects on it's own stack
	/// frame, rather than having objects created on run() that are only needed by startup and
	/// yet continue to consume stack space.
	///@return true if pass the initialization steps
	virtual bool init(void)	{ return true; };

	/// All threads execute by deriving the Run method of Thread. This method is called after
	/// Initial to begin normal operation of the thread. If the method terminates, then the
	/// thread will also terminate after notifying it's parent and calling it's final() method.
	///@return the return value will also be passed as the thread exit code
	virtual int run(void) = 0;

	/// A thread that is self terminating, either by invoking exit() or leaving its run(),
	/// will have this method called. It can be used to self delete the current object assuming
	/// the object was created with new on the heap rather than stack local. You can safe
	/// delete thread via "delete this"in final, but should exit ASAP
	virtual void final(void);

	/// Used by another thread to terminate the current thread. Termination actually occurs
	/// based on the current setCancel() mode. When the current thread does terminate, control
	/// is returned to the requesting thread. terminate() should always be called at the start
	/// of any destructor of a class derived from Thread to assure the remaining part of the
	/// destructor is called without the thread still executing.
	void terminate(int code = 0);

	/// Used to properly exit from a Thread derived run() or init() method. Terminates
	/// execution of the current thread and calls the derived classes final() method.
	void exit(void);

	/// Used to change the value of the status of the running thread
	void setStatus(const status_t st);

public:
	
	/// When a new thread is created, it does not begin immediate execution, because the
	/// derived class virtual tables are not properly loaded at the time the C++ object is
	/// created within the constructor itself, at least in some compiler/system combinations.
	/// The thread can either be told to wait for an external semaphore, or it can be started
	/// directly after the constructor completes by calling the start() method.
	/// if the inherited class re-implement start(), invoke base start function by NativeThread::start() is required.
	/// @return error code if execution fails.
	virtual bool start();

	/// Suspends execution of the selected thread.
	void suspend(void);

	// Resumes execution of the selected thread.
	void resume(void);

	/// Gets the status code of the current thread object.
	/// @return a status code
	status_t getStatus(void) const;

	/// Gets the id of the current thread object.
	/// @return  thread id
	uint32 id(void) const;

	static void sleep(timeout_t msec);//timeout_t is a value of millisecond

	/// Verifies if the thread is still running or has already been terminated but not yet
	/// deleted.
	/// @return true if the thread is still executing.
	bool isRunning(void);

	/// wait for thread to exit
	///@param		timeout		the time to wait, in msec
	///@return		0 if wait success, otherwise failed
	uint32 waitHandle(timeout_t timeout);	
	
	/// Tests to see if the current execution context is the same as the specified thread
	/// object.
	/// @return true if the current context is this object.
	bool isThread(void);

	/// A thread affinity mask is a bit vector in which each bit represents a logical processor that a thread is allowed to run on.
	/// If the function succeeds, the return value is the thread's previous affinity mask.
	/// If the function fails, the return value is zero. To get extended error information, call GetLastError.
	bool setCPUAffinity(uint cpuId);

	/// set the affinity of the current caller thread
#ifdef ZQ_OS_MSWIN
	static bool setAffinityOfThread(uint cpuId, HANDLE hThread =NULL);
#else
	static bool setAffinityOfThread(uint cpuId, ThreadId_t hThread =0);
#endif
};


} // namespace common
} // namespace ZQ

#endif // __ZQ_Common_NativeThread_h__
