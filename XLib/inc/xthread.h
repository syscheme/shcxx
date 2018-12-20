// ===========================================================================
// Copyright (c) 2005 by
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
// ----------------------------
//
// Name  : xthread.h
// Author: Cary (cary.xiao@i-zq.com)
// Date  : 07/06/2005
// Desc  : C++ thread library for multi-platform, it based on pthreads.
//
// Revision History:
//
//
// ----------------------------
//
//
// ===========================================================================

#ifndef _XTHREAD_H_
#define _XTHREAD_H_

#include <assert.h>
#include <pthread.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
	
class XThread
{
public:

	typedef void* exit_status_t;

	enum status_t {
		STATUS_INVALID		= 0,
		STATUS_INITIAL		= STATUS_INVALID,
		STATUS_RUNNING,
		STATUS_SUSPEND,
		STATUS_FINISHED
	};

	XThread();
	virtual ~XThread();

	int	_errno();
protected:

	// the thread_start_routine of pthreads
	static void* _thread_routine(void* arg);
	
	pthread_t		m_pthread;
	// current status_t of thread
	status_t		m_status;
	// the termination status_t of the specified thread
	exit_status_t	m_exit_status;

	int				m_err;

protected:

	/// The initial method is called by a newly created thread when it starts execution. This
	/// method is ran with deferred cancellation disabled by default. The Initial method is
	/// given a separate handler so that it can create temporary objects on it's own stack
	/// frame, rather than having objects created on run() that are only needed by startup and
	/// yet continue to consume stack space.
	///@return true if pass the initialization steps
	virtual bool init(void);

	/// All threads execute by deriving the Run method of Thread. This method is called after
	/// Initial to begin normal operation of the thread. If the method terminates, then the
	/// thread will also terminate after notifying it's parent and calling it's final() method.
	///@return the return value will also be passed as the thread exit code
	virtual exit_status_t run(void) = 0;

	/// A thread that is self terminating, either by invoking exit() or leaving its run(),
	/// will have this method called. It can be used to self delete the current object assuming
	/// the object was created with new on the heap rather than stack local. You can safe
	/// delete thread via "delete this"in final, but should exit ASAP
	virtual void final(void);

	/// Used to properly exit from a Thread derived run() or init() method. Terminates
	/// execution of the current thread and calls the derived classes final() method.
	void exit(exit_status_t status);

public:

	/// When a new thread is created, it does not begin immediate execution, because the
	/// derived class virtual tables are not properly loaded at the time the C++ object is
	/// created within the constructor itself, at least in some compiler/system combinations.
	/// The thread can either be told to wait for an external semaphore, or it can be started
	/// directly after the constructor completes by calling the start() method.
	/// @return error code if execution fails.
	bool start();

	/// Suspends execution of the selected thread.
	bool suspend(void);

	// Resumes execution of the selected thread.
	bool resume(void);

	/// Used by another thread to terminate the current thread. Termination actually occurs
	/// based on the current setCancel() mode. When the current thread does terminate, control
	/// is returned to the requesting thread. terminate() should always be called at the start
	/// of any destructor of a class derived from Thread to assure the remaining part of the
	/// destructor is called without the thread still executing.
	bool terminate(exit_status_t status);


	/// Gets the status_t code of the current thread object.
	/// @return a status_t code
	inline status_t getStatus(void);

	/// Verifies if the thread is still running or has already been terminated but not yet
	/// deleted.
	/// @return true if the thread is still executing.
	bool isRunning(void);

	// retrieves the termination status_t of the specified thread
	// @return a status_t code
	exit_status_t getExitStatus();

	// this routine will return the thread objects of pthreads library
	// WARNING: This function may make the code depend on the platform, 
	// so it may be abolished in future
	pthread_t& _getThread();

};

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XTHREAD_H_
