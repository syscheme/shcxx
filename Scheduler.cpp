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
// Name  : Schduler.cpp
// Author : Bernie Zhao (bernie.zhao@i-zq.com  Tianbin Zhao)
// Date  : 2006-4-20
// Desc  : Impl for scheduler class
//
// Revision History:
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Scheduler.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 13    08-12-09 14:36 Yixin.tian
// 
// 12    08-03-20 10:07 Yixin.tian
// modify sem_trywait to sem_timedwait in linux os
// 
// 11    08-03-06 16:28 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 10    08-03-03 18:03 Yixin.tian
// merged changes for linux
// 
// 9     07-08-16 10:40 Fei.huang
// 
// 9     07-08-06 14:10 Fei.huang
// 
// 8     07-05-22 17:32 Hui.shao
// 
// 7     06-10-10 13:58 Hongquan.zhang
// 
// 6     06-09-12 17:16 Bernie.zhao
// now only int64 version for PollingTimer under ZQ_OS_MSWIN
// 
// 5     06-05-23 18:29 Hui.shao
// re-org the class definition
// 
// 4     06-04-25 20:29 Bernie.zhao
// changed ScheduleTask start function prototype
// 
// 3     06-04-25 18:50 Bernie.zhao
// comment detailed
// 
// 2     06-04-25 18:36 Bernie.zhao
// 
// 1     06-04-24 14:40 Bernie.zhao
// ===========================================================================

#include "Scheduler.h"

extern "C" {
#include <time.h>
#ifndef ZQ_OS_MSWIN
#  include <sys/timeb.h>
#endif
}

namespace ZQ {
namespace common {

#define	MAX_SCHD_POLLTIME		10000	// 10 seconds, the longest wait time for each schedule monitoring loop
#define MAX_TASK_PRESTARTTIME	10		// task will be chosen 10 msec before actual start time
#define MIN_TASK_UPDATETIME		1000	// task can only be updated 1 second early than the original start time


#ifdef ZQ_OS_MSWIN
SchdPollerThread::SchdPollerThread(Scheduler& schd)
	: _schdler(schd), ThreadRequest(schd._thrpool), _bQuit(false), _hWakeupEvent(NULL)
{	
	_hWakeupEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

SchdPollerThread::~SchdPollerThread()
{
	_bQuit = true;
	wakup();
	
	::Sleep(1);

	if(_hWakeupEvent)
		::CloseHandle(_hWakeupEvent);
}

void SchdPollerThread::quit()
{
	_bQuit = true;
	wakup();
}

void SchdPollerThread::wakup()
{
	::SetEvent(_hWakeupEvent);
}

bool SchdPollerThread::init(void)
{
	if(NULL == _hWakeupEvent)
		return false;

	return !_bQuit;
}

int SchdPollerThread::run()
{
	//////////////////////////////////////////////////////////////////////////
	// main loop
	// This loop uses a wait function to wait for some events:
	// if quit event is signaled, the loop should be break and return;
	// if wakeup event is signaled, the loop should start over, with the updated sleep time
	// if timeout, then we must post some task.  Scan tasks to choose one.
	//////////////////////////////////////////////////////////////////////////
	while(!_bQuit)
	{
		long sleepTime	= MAX_SCHD_POLLTIME;

		// scan the pending task and prepare the min sleep time
		{
			MutexGuard tmpGd(_schdler._taskMutex);	// lock the task list
			
			for(std::vector<ScheduleTask*>::iterator iter = _schdler._taskList.begin(); iter < _schdler._taskList.end(); )
			{
				ScheduleTask* pTask = *iter;
				if(pTask==NULL)
				{
					// invalid task, remove it. Do not increase iterator
					_schdler._taskList.erase(iter);
					continue;
				}
				
				if(_bQuit)
					break;	// should quit polling
				
				timeout64_t timeout = pTask->getTimer64();
				if(timeout <= MAX_TASK_PRESTARTTIME)	
				{
					// timer is met, now we push it into the real thread request pool
					// Do not increase iterator
					_schdler._taskList.erase(iter);
					pTask->start();
				}
				else
				{
					// not yet to start, figure out how long shall we wait.
					// Of coz we wait for the smallest period among all the tasks

					sleepTime = (long) (timeout<sleepTime ? timeout : sleepTime);

					iter++;	// increase iterator
				}
				
			}	// end for
		}
		
		if(_bQuit)
			break;	// should quit polling

		// now goto sleep
		if (sleepTime >MAX_TASK_PRESTARTTIME)
			sleepTime -= MAX_TASK_PRESTARTTIME;

		switch(::WaitForSingleObject(_hWakeupEvent, sleepTime))
		{
		case WAIT_OBJECT_0:	// wakeup by outside
		case WAIT_TIMEOUT:	// timeout
			break;
		}

		if(_bQuit)
			break;	// should quit polling

	} // while

	return 0;
}

#else //non-win32
SchdPollerThread::SchdPollerThread(Scheduler& schd)
	:ThreadRequest(schd._thrpool), _schdler(schd), _bQuit(false)
{
	sem_init(&_wakeup,0,0);
}

SchdPollerThread::~SchdPollerThread()
{
	_bQuit = true;
	wakup();
	usleep(1000);

	try{
		sem_destroy(&_wakeup);
	}
	catch(...){}
}

void SchdPollerThread::quit()
{
	_bQuit = true;
	wakup();
}

void SchdPollerThread::wakup()
{
	sem_post(&_wakeup);
}

bool SchdPollerThread::init(void)
{
//	#pragma message ( __MSGLOC__ "TODO: impl here for non-Win32")
	return !_bQuit;
}

int SchdPollerThread::run()
{
	//////////////////////////////////////////////////////////////////////////
	// main loop
	// This loop uses a wait function to wait for some events:
	// if quit event is signaled, the loop should be break and return;
	// if wakeup event is signaled, the loop should start over, with the updated sleep time
	// if timeout, then we must post some task.  Scan tasks to choose one.
	//////////////////////////////////////////////////////////////////////////
	while(!_bQuit)
	{
		long sleepTime	= MAX_SCHD_POLLTIME;

		// scan the pending task and prepare the min sleep time
		{
			MutexGuard tmpGd(_schdler._taskMutex);	// lock the task list
			
			for(std::vector<ScheduleTask*>::iterator iter = _schdler._taskList.begin(); iter < _schdler._taskList.end(); )
			{
				ScheduleTask* pTask = *iter;
				if(pTask==NULL)
				{
					// invalid task, remove it. Do not increase iterator
					_schdler._taskList.erase(iter);
					continue;
				}
				
				if(_bQuit)
					break;	// should quit polling

				timeout64_t timeout = pTask->getTimer();
				if(timeout <= MAX_TASK_PRESTARTTIME)	
				{
					// timer is met, now we push it into the real thread request pool
					// Do not increase iterator
					_schdler._taskList.erase(iter);
					pTask->start();
				}
				else
				{
					// not yet to start, figure out how long shall we wait.
					// Of coz we wait for the smallest period among all the tasks

					sleepTime = (long) ((long)timeout<sleepTime ? timeout : sleepTime);

					iter++;	// increase iterator
				}
				
			}	// end for
		}
		
		if(_bQuit)
			break;	// should quit polling

		// now goto sleep
		if (sleepTime >MAX_TASK_PRESTARTTIME)
			sleepTime -= MAX_TASK_PRESTARTTIME;

		struct timespec ts;
		struct timeb tb;
		
		ftime(&tb);
		int sec = sleepTime/1000;
		int msec = sleepTime%1000;
		
		tb.time += sec;	
		tb.millitm += msec;
		if(tb.millitm > 999)
		{
			tb.millitm -= 1000;
			tb.time += 1;
		}
		ts.tv_sec = tb.time;
		ts.tv_nsec = tb.millitm * 1000000;
		sem_timedwait(&_wakeup,&ts);

		if(_bQuit)
			break;	// should quit polling

	} // while

	return 0;
}
#endif

void SchdPollerThread::final(int retcode, bool bCancelled)
{
	_bQuit = true;
}

/*
void SchdPollerThread::poll()
{
	DWORD nextSleep = MAX_SCHD_POLLTIME;

	MutexGuard tmpGd(_schdler._taskMutex);	// lock the task list

	std::vector<ScheduleTask*>::iterator	iter = 0;
	for(iter = _schdler._taskList.begin(); iter < _schdler._taskList.end(); )
	{
		if(*iter==NULL)
		{
			// invalid task, remove it. Do not increase iterator
			_schdler._taskList.erase(iter);
			continue;
		}

		if(_bQuit)
		{
			break;	// should quit polling
		}

		timeout64_t timeout = (*iter)->getTimer64();
		if(timeout<=MAX_TASK_PRESTARTTIME)	
		{
			// timer is met, now we push it into the real thread request pool
			// Do not increase iterator
			(*iter)->start();
			// (*iter)->_timer.endTimer64();
			_schdler._taskList.erase(iter);
		}
		else
		{
			// not yet to start, figure out how long shall we wait.
			// Of coz we wait for the smallest period among all the tasks
			if(timeout-MAX_TASK_PRESTARTTIME < nextSleep)
			{
				nextSleep = timeout-MAX_TASK_PRESTARTTIME;
			}

			iter++;	// increase iterator
		}

	}	// end for

	// update the next sleep interval
	updateSleeptime(nextSleep);
}
*/

//////////////////////////////////////////////////////////////////////////
// class Scheduler
//////////////////////////////////////////////////////////////////////////
Scheduler::Scheduler(NativeThreadPool& pool)
: _pPoller(NULL),_thrpool(pool) 
{
#ifdef ZQ_OS_MSWIN
	::Sleep(1); // yield for other thread
	_pPoller = new SchdPollerThread(*this);
	::Sleep(10);	// give a chance for the poller thread to start quickly
#else
	usleep(1000); // yield for other thread
	_pPoller = new SchdPollerThread(*this);
	usleep(10*1000);	// give a chance for the poller thread to start quickly
#endif
	if (NULL == _pPoller)
		throw Exception("Scheduler::Scheduler() failed to allocate memory for SchdPollerThread()");

	_pPoller->start();
}

Scheduler::~Scheduler()
{
	MutexGuard tmpGd(_taskMutex);
	_taskList.clear();

	if (NULL != _pPoller)
		_pPoller->quit();
#ifdef ZQ_OS_MSWIN	
	::Sleep(10); // yield for other thread
#else
	usleep(10*1000);
#endif
}

bool Scheduler::bookTask(ScheduleTask& task)
{
	bool bFound = false;
	
	MutexGuard	tmpGd(_taskMutex);
	std::vector<ScheduleTask*>::iterator	iter ;
	for(iter = _taskList.begin(); iter < _taskList.end(); iter++)
	{
		if(*iter == &task)
		{
			// this task already exists
			bFound = true;
			break;
		}
	}

	if(!bFound)
	{
		_taskList.push_back(&task);
	}

	// we had added a task, so force poller to begin a new parse
	_pPoller->wakup();

	return true;
}

bool Scheduler::clearTask(ScheduleTask& task)
{
	bool bFound = false;

	MutexGuard	tmpGd(_taskMutex);
	std::vector<ScheduleTask*>::iterator	iter ;
	for(iter = _taskList.begin(); iter < _taskList.end(); iter++)
	{
		if(*iter == &task)
		{
			// just reset it, not remove it.  We will let SchdPollerThread to remove it
			*iter = NULL;	
			bFound = true;
			break;
		}
	}

	return bFound;
}

//////////////////////////////////////////////////////////////////////////
// class Schedule Task
//////////////////////////////////////////////////////////////////////////
#ifdef ZQ_OS_MSWIN
ScheduleTask::ScheduleTask(Scheduler& schd)
: ThreadRequest(schd._thrpool),_schdler(schd)
{
	endTimer64();
}

ScheduleTask::~ScheduleTask()
{
	endTimer64();
	_schdler.clearTask(*this);
}

bool ScheduleTask::startWait(timeout64_t waittime)
{
	timeout64_t	timeout = getTimer64();
	if(timeout!=_UI64_MAX)	// has timer already
	{
		if(timeout<MIN_TASK_UPDATETIME)	
		{
			// we have less than MIN_TASK_UPDATETIME second(s) before task begin, so we can not update it
			return false;
		}
	}	

	setTimer64(waittime);

	// insert into pending list
	return _schdler.bookTask(*this);
}

bool ScheduleTask::startAt(SYSTEMTIME&	starttime)
{
	SYSTEMTIME currtime;
	
	// calculate the time difference between now and starttime
	::GetSystemTime(&currtime);

	timeout64_t	timediff;
	
	tm	tb1,tb2;
	time_t	tval1,tval2;
	
	tb1.tm_year = starttime.wYear -1900;
	tb2.tm_year	= currtime.wYear -1900;
	tb1.tm_mon	= starttime.wMonth -1;
	tb2.tm_mon	= currtime.wMonth -1;
	tb1.tm_mday	= starttime.wDay;
	tb2.tm_mday	= currtime.wDay;
	tb1.tm_hour	= starttime.wHour;
	tb2.tm_hour	= currtime.wHour;
	tb1.tm_min	= starttime.wMinute;
	tb2.tm_min	= currtime.wMinute;
	tb1.tm_sec	= starttime.wSecond;
	tb2.tm_sec	= currtime.wSecond;
	
	tval1 = mktime(&tb1);
	tval2 = mktime(&tb2);
	
	timediff = (tval1>tval2)? (tval1 - tval2) : 0;	// this is in seconds
	timediff = timediff*1000 + (starttime.wMilliseconds - currtime.wMilliseconds);	// now in msec
	
//	if(!timediff) 
//		timediff+=1;

	return startWait(timediff);
}

#else
ScheduleTask::ScheduleTask(Scheduler& schd)
:ThreadRequest(schd._thrpool), _schdler(schd)
{
	endTimer();
}

ScheduleTask::~ScheduleTask()
{
	endTimer();
	_schdler.clearTask(*this);
}

bool ScheduleTask::startWait(timeout64_t waittime)
{
	timeout64_t	timeout = getTimer();
	if(timeout != (uint64)(-1))//0xffffffffffffffff)
	{
		if(timeout<MIN_TASK_UPDATETIME)	
		{
			// we have less than MIN_TASK_UPDATETIME second(s) before task begin, so we can not update it
			return false;
		}
	}
	setTimer(waittime);

	// insert into pending list
	return _schdler.bookTask(*this);
}

bool ScheduleTask::startAt(struct tm&	starttime)
{
	time_t curt;
	time(&curt);

	struct tm* utctm; 
	utctm = gmtime(&curt);
	
	time_t tstar,tcur;
	tcur = mktime(utctm);
	tstar = mktime(&starttime);
	
	timeout64_t	timediff = 0;
	timediff = (tstar>tcur) ? (tstar-tcur) : 0;
	timediff = timediff * 1000;//to millisec
	
	return startWait(timediff);
}
#endif

bool ScheduleTask::start()
{
	return ThreadRequest::start();
}

}	// namespace ZQ
}	// namespace common
