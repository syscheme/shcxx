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
// Name  : Scheduler.h
// Author : Bernie Zhao (bernie.zhao@i-zq.com  Tianbin Zhao)
// Date  : 2006-4-20
// Desc  : Definition of common scheduler class
//
// Revision History:
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Scheduler.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 12    08-12-09 14:36 Yixin.tian
// 
// 11    08-03-06 16:28 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 10    08-03-03 18:03 Yixin.tian
// merged changes for linux
// ===========================================================================

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

// local headers
#include "ZQ_common_conf.h"
#include "NativeThreadPool.h"
#include "PollingTimer.h"

// stl headers
//#pragma warning (disable : 4786)
#include <vector>

namespace ZQ {
namespace common {
		
class ZQ_COMMON_API Scheduler;
class ZQ_COMMON_API ScheduleTask;

//////////////////////////////////////////////////////////////////////////
// class SchdPollerThread
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// class scheduler polling timer thread, not exported
class SchdPollerThread : public ThreadRequest
{
public:
	/// constructor
	SchdPollerThread(Scheduler& schd);

	/// destructor
	~SchdPollerThread();
	
public:
	/// signal the poller to exit
	void quit();

	/// signal the poller to wakeup from the sleeping
	void wakup();

	/// force the poller to update the sleep interval for next time
	void updateSleeptime(unsigned long sleeptime);

protected:
	// implemetations of ThreadRequest
	bool init(void);
	int  run(void);
	void final(int retcode =0, bool bCancelled =false);

//private:
	/// actual poll process
//	void poll();
	
private:
	
	Scheduler&	_schdler;
	bool		_bQuit;
#ifdef ZQ_OS_MSWIN
	HANDLE		_hWakeupEvent;
#else
	sem_t		_wakeup;
#endif
};

//////////////////////////////////////////////////////////////////////////
/// class Scheduler
class Scheduler
{
	friend class SchdPollerThread;
	friend class ScheduleTask;
	
public:
	/// constructor
	///@param[in]	pool		the thread pool to run on
	Scheduler(NativeThreadPool& pool);
	~Scheduler();

	NativeThreadPool& theThreadPool() { return _thrpool; }

public:
	/// This function is too remove task from the pending list. 
	///@return		true if success, false else
	bool clearTask(ScheduleTask& task);

protected:
	/// This function is only for ScheduleTask to call, to add task into pending list
	///@return		true if success, false else
	bool bookTask(ScheduleTask& task);

	
protected:
	
	std::vector<ScheduleTask*>	_taskList;		///< task list, containing all task that has not been executed
	Mutex	_taskMutex;		///< lock for task list

private:

	SchdPollerThread* _pPoller;	///< polling thread object

	NativeThreadPool&	_thrpool; ///< the thread pool to run on
};
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
/// class ScheduleTask
class ScheduleTask : public ThreadRequest, public PollingTimer
{
	friend class SchdPollerThread;
	friend class Scheduler;

public:
	/// constructor
	///@param[in]		schd		the reference of the Scheduler object that owns this task
	ScheduleTask(Scheduler& schd);

	/// destructor
	~ScheduleTask();

protected:
	
	/// Inherited from ThreadRequest.
	/// The initial method is called by a thread when it execute the task. This
	/// method is ran with deferred cancellation disabled by default. The Initial method is
	/// given a separate handler so that it can create temporary objects on it's own stack
	/// frame, rather than having objects created on run() that are only needed by startup and
	/// yet continue to consume stack space.
	///@return true if pass the initialization steps
	virtual bool init(void)	{ return true; };
	
	/// Inherited from ThreadRequest.
	/// All task request execute by deriving the Run method. This method is called after
	/// initial to begin normal operation of the request. If the method terminates then the
	/// thread will notify the request with return code of this method in final() method.
	///@return the return value will also be passed to final()
	virtual int run(void) {return 0; }
	
	/// Inherited from ThreadRequest.
	/// A task that is self terminating, either by invoking exit() or leaving its run(),
	/// will have this method called. It can be used to self delete the current object assuming
	/// the object was created with new on the heap rather than stack local. You can safe
	/// delete thread via "delete this"in final, but should exit ASAP
	///@param retcode the return value of run()
	///@param bTerminated  true if the request was cancelled by the execution thread
	virtual void final(int retcode =0, bool bCancelled =false) {}

public:
	///Post the task to scheduler, with the scheduler time specified.
	///@param[in]	waittime	time to wait before start, in msec
	///@return				true if success, false else.
	///@remarks				If the task has already been posted, calling this function will update 
	///the schedule time.
	///If the task is already posted into the scheduler and is about to starting within 1 second,
	///the task can not been updated.  Thus calling this will return false.
	bool startWait(timeout64_t	waittime);

#ifdef ZQ_OS_MSWIN
	///Post the task to scheduler, with the scheduler time specified.
	///@param[in]	starttime	time point to start, UTC time
	///@return				true if success, false else.
	///@remarks				If the task has already been posted, calling this function will update 
	///the schedule time.
	///If the task is already posted into the scheduler and is about to starting within 1 second,
	///the task can not been updated.  Thus calling this will return false.
	bool startAt(SYSTEMTIME&	starttime);
#else
	bool startAt(struct tm&	starttime);
#endif
private:
	// move start() to private, so can only be called from Scheduler
	virtual bool start();

protected:

	Scheduler&		_schdler;		///< scheduler object
};
//////////////////////////////////////////////////////////////////////////


}	// namespace ZQ
}	// namespace common


#endif	// #ifndef _SCHEDULER_H_
