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
// Desc  : impl a native objected thread
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/NativeThread.cpp 3     3/11/16 9:52a Dejian.fei $
// $Log: /ZQProjs/Common/NativeThread.cpp $
// 
// 3     3/11/16 9:52a Dejian.fei
// NDK  android
// 
// 2     9/25/15 2:23p Ketao.zhang
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 35    10-11-04 14:17 Hongquan.zhang
// 
// 34    10-09-16 18:17 Xiaohui.chai
// 
// 33    10-03-22 15:53 Fei.huang
// * fix: use status instead of thread ID to tell if start() has been
// called
// 
// 32    09-12-29 11:35 Fei.huang
// 
// 31    09-12-29 11:33 Fei.huang
// + message not shown if allocated locally
// 
// 30    09-12-21 17:36 Fei.huang
// * fix: can't not rely on thread id, use status to tell if thread is
// running
// * fix: waitHandle should do nothing if thread is not running at all
// 
// 29    09-12-16 18:42 Fei.huang
// 
// 29    09-12-10 17:20 Fei.huang
// * do not join any thread who is not running
// 
// 28    09-12-08 14:22 Fei.huang
// + disable thread cancel 
// + throw exception if thread creation failed
// * do not set thread id to 0, unless dtor called
// * disable thread after final completed
// 
// 
// 27    09-11-25 19:03 Fei.huang
// * accept (-1) for waitHandle to wait INFINITE
// 
// 26    09-09-24 17:14 Fei.huang
// * fix start() might be invoked twice or above that cause waitHandle
// misbehaved
// 
// 25    09-08-07 14:38 Yixin.tian
// set thread stacksize of Linux default value 2M
// 
// 24    09-06-23 15:22 Fei.huang
// * don't cancel thread when exit
// 
// 22    09-05-18 16:19 Fei.huang
// * remove pthread_join after cancel 
// * re-start sem_wait in case of interrupt in execute
// 
// 21    09-05-18 11:04 Fei.huang
// * fix waitHandle
// 
// 20    09-05-15 16:44 Fei.huang
// * replace cond_timedwait back with sem_timedwait
// * fix waithandle block issue
// 
// 19    09-05-13 15:26 Fei.huang
// 
// 18    09-05-13 15:21 Fei.huang
// 
// 17    09-05-12 11:01 Build
// 
// 16    09-05-11 18:30 Fei.huang
// * remove "suspend" "resume"
// * rewrite waitHandle
// * use pthread_cond_wait to replace sem_wait
// * fix some issue
// 
// 15    09-04-22 13:46 Fei.huang
// * remove dup pthread_cancel, might cause crash
// 
// 14    08-12-25 18:14 Fei.huang
// * fix pthread_cancel cause fatal error under NPTL
// 
// 13    08-03-20 10:07 Yixin.tian
// modify sem_trywait to sem_timedwait in linux os
// 
// 12    08-03-06 15:12 Yixin.tian
// merrge for linux
// 
// 11    08-03-03 17:54 Yixin.tian
// merged changes for linux
// 
// 10    07-07-20 17:30 Hui.shao
// yield for other thread when start()
// 
// 9     06-09-07 18:02 Hui.shao
// 
// 8     06-02-27 13:44 Cary.xiao
// 
// 1     06-02-23 18:17 Cary.xiao
// 
// 7     05-08-25 23:02 Bernie.zhao
// 
// 6     05-06-24 16:30 Bernie.zhao
// added try--catch in thread body
// 
// 5     05-06-15 20:17 Bernie.zhao
// fixed resource leak bug -> always call ::CloseHandle()
// 
// 4     05-04-22 12:12 Hongye.gu
// 
// 3     05-04-21 11:13 Daniel.wang
// 
// 1     4/14/05 6:50p Hui.shao
// ============================================================================================

#include "NativeThread.h"
#include "Log.h" // from macro SCRTRACE, DON'T log to file in base thread

//xiaobai
#ifndef SCRTRACE
#define SCRTRACE
#endif

extern "C" {

#ifdef WIN32
#  include <process.h>
#  include <stdio.h>
#else
#  include <unistd.h>
#  include <errno.h>
#endif

#include <time.h>
}


namespace ZQ {
namespace common {

// typedef	unsigned (__stdcall *exec_t)(void *);
#ifdef WIN32
NativeThread::NativeThread(int stacksize)
       :_thrdID(0), _status(stDeferred),_bCancel(false)
{
	_flags.B = 0;
	
//	_hThread = (HANDLE)CreateThread(NULL, stacksize, _execute, (void *)this, CREATE_SUSPENDED, (unsigned long *)&_thrdID);
	_hThread = (HANDLE)_beginthreadex(NULL, stacksize, _execute, (void *)this, CREATE_SUSPENDED, (unsigned*) &_thrdID);

	if(_hThread >0)
		return;	

	setStatus(stInitial);
}

NativeThread::~NativeThread()
{
	terminate();
	try
	{
		CloseHandle(_hThread);
	}
	catch(...){}

	_hThread = INVALID_HANDLE_VALUE;
}

void NativeThread::exit()
{
	if (isThread())
	{
		setStatus(stDisabled);
		ExitThread(0);
	}
}

bool NativeThread::isRunning(void)
{
	return (_status == stRunning) ? true : false;
}

bool NativeThread::isThread(void)
{
	return ((_thrdID == GetCurrentThreadId())) ? true : false;
}

void NativeThread::terminate(int code /* = 0 */)
{
#ifdef _DEBUG
//	printf(LOGFMT("tid=%d\n"), _thrdID);
#endif // _DEBUG

	if(!_thrdID)
	{
		return;
	}

	SCRTRACE;

	if(_thrdID == GetCurrentThreadId())
		return;

	SCRTRACE;

	bool terminated = false;

	try
	{
		if(!_flags.b.active)
			ResumeThread(_hThread);
	}
	catch(...){}

	SCRTRACE;

	try
	{
		TerminateThread(_hThread, code);
		terminated = true;
	}
	catch(...){}

	WaitForSingleObject(_hThread, INFINITE);

	if (terminated)
	{
		SCRTRACE;
		this->final();
	}

	_thrdID = 0;
}

// unsigned long __stdcall NativeThread::_execute(void *thread)
unsigned __stdcall NativeThread::_execute(void *thread)
{
	NativeThread *th = (NativeThread *)thread;

	int ret = -1;
	if( th->_bCancel )
	{
		th->_thrdID = 0 ;
		th->setStatus(stDisabled);
		return 0;
	}

	try
	{
		// initialize the rand seed as the srand()/rand() is per-thread
		::srand((uint32) (::time(NULL)<<6 ^ ((uint32) thread & 0xffffffff)) ^ 0x93da8fae);

		if (th->init())
		{
			th->setStatus(stRunning);
			ret = th->run();
		}

	} catch(...) {}

	th->setStatus(stDisabled);

	try
	{
		
		if (th->_thrdID > 0)
		{
			th->_thrdID = 0;

			SCRTRACE;
			th->final();
		}
		else ret = -2; // this thread was terminated

	} catch(...) {}

	return ret;
}

void NativeThread::setStatus(const status_t st)
{
	_status = st;
}

void NativeThread::sleep(timeout_t msec)
{
	::SleepEx(msec, false);
}

bool NativeThread::start()
{
	DWORD ret = ResumeThread(_hThread);
	::Sleep(1); // yield for other threads
	return (ret!=-1);
}

void NativeThread::final(void)
{ 
	SCRTRACE;
	return; 
}

void NativeThread::suspend(void)
{
	SuspendThread(_hThread);
}

void NativeThread::resume(void)
{
	ResumeThread(_hThread);
}

uint32 NativeThread::waitHandle(timeout_t timeout)
{
	if( _status == stDeferred)
	{
		_bCancel = true;
		resume();
	}
	return WaitForSingleObject(_hThread,timeout);
}

#else//non-win32

#define panic(str) \
    char* error = new char[128]; \
    memset(error, '\0', sizeof(error)); \
    sprintf(error, str": [%d][%s]", errno, strerror(errno)); \
    throw error; 


__thread unsigned int tid = 0; 

void settid()
{
#ifndef ZQ_COMMON_ANDROID
	tid = syscall(SYS_gettid);
#else
	tid = pthread_self();
#endif
}

unsigned int getthreadid()
{
	return tid;
}

NativeThread::NativeThread(int stacksize):_thrdID(0), _status(stDeferred){
	_flags.B = 0;

	if(sem_init(&_thsem, 0, 0)) {
        panic("failed to initialize thread semaphore");
	}
	if(sem_init(&_suspend, 0, 0)) {
        panic("failed to initialize suspend semaphore");
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stacksize);
	if(pthread_create(&_thrdID, &attr, _execute,(void*)this)) {
		sem_destroy(&_thsem);
		sem_destroy(&_suspend);
		pthread_attr_destroy(&attr);

        panic("failed to create thread");
	}

	setStatus(stInitial);
}

NativeThread::~NativeThread() {
	terminate();

    sem_destroy(&_thsem);
    sem_destroy(&_suspend);
}

void NativeThread::exit() {
	if(isThread()) {
		setStatus(stDisabled);
		pthread_exit(EXIT_SUCCESS);
	}
}

bool NativeThread::isRunning() {
	return (_status == stRunning) ? true : false;
}

bool NativeThread::isThread() {
    return pthread_equal(_thrdID, pthread_self());
}

void NativeThread::terminate(int code /* = 0 */) {
    /* called from the same thread */
	if(isThread()) {
		return;
    }
    
    if(_status == stRunning) {
//      pthread_cancel(_thrdID);

        void* res;
        pthread_join(_thrdID, &res);
    
/*
        if(res == PTHREAD_CANCELED) {
            this->final();
        }
*/

        _thrdID = 0;
    }
}

void* NativeThread::_execute(void *thread) {

	settid();
#ifndef ZQ_COMMON_ANDROID
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif

	NativeThread *th = (NativeThread *)thread;

	/* suspend */
sus:
	int res =sem_wait(&th->_suspend);
	if(res == (-1) && errno == EINTR) {
		goto sus;
	}

	//try {
		if (th->init()) {
			th->setStatus(stRunning);
			th->run();
		}
	//} catch(...) {}

	try {
        th->final();
    } catch(...) {}

	sem_post(&th->_thsem);
    
	th->setStatus(stDisabled);

    return (void*)(0);
}

void NativeThread::setStatus(const status_t st) {
	_status = st;
}

void NativeThread::sleep(timeout_t msec) {
    struct timespec ts;
    ts.tv_sec = msec/1000L;
    ts.tv_nsec = (msec%1000L) * 1000000L;
    nanosleep(&ts, 0);
}

bool NativeThread::start() {
	return (sem_post(&_suspend) == 0);
}

void NativeThread::final(void) { 
}

void NativeThread::suspend(void) {
}

void NativeThread::resume(void) {
}

uint32 NativeThread::waitHandle(timeout_t timeout) {
    if(!isRunning()) {
        return (0);
    }
	int res = 0;
	if(timeout <= 0 || timeout == (timeout_t)(-1)) {
		void* val;
        res = pthread_join(_thrdID, &val);
	}
	else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);

		long long nsec = ts.tv_nsec + timeout*1000000LL;
		ts.tv_sec += nsec/1000000000L;
		ts.tv_nsec = nsec%1000000000L;

con:
        if(sem_timedwait(&_thsem, &ts) == (-1)) {
            if(errno == EINTR) {
                goto con;
            }
            res = errno;
        }
	}
	return res;
}

#endif

/// Gets the status code of the current thread object.
/// @return a status code
NativeThread::status_t NativeThread::getStatus(void) const {
    return _status;
}

/// Gets the id of the current thread object.
/// @return  thread id
uint32 NativeThread::id(void) const {
    return _thrdID;
}

} // namespace common
} // namespace ZQ

