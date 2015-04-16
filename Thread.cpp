// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
// Ident : $Id: Thread.cpp,v 1.13 2004/07/13 10:14:54 shao Exp $
// Branch: $Name:  $
// Author: Meng Wang
// Desc  : impl Thread by invoking ThreadPool.dll from SeaChange
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Thread.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 3     04-11-03 11:30 Jie.zhang
// 
// 2     04-11-03 10:26 Jie.zhang
// Revision 1.13  2004/07/13 10:14:54  shao
// load the lib
//
// Revision 1.12  2004/06/30 07:18:17  jshen
// remove resetThreadPool
//
// Revision 1.11  2004/06/17 04:07:48  jshen
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

#include "Log.h"
#include "Thread.h"
#include <stdio.h>
#include "scthreadpool.h"

#pragma comment (lib, "ScThreadPool" VODLIBEXT)

namespace ZQ{
namespace common {

ThreadPool gThreadPool;

///default constructor
ThreadPool::ThreadPool(DWORD dwNumThreads/*=10*/,DWORD dwGrowBy /*=10*/ ,DWORD dwMaxThreads /*=INFINITE*/, DWORD dwIdleTime/*=5000*/)
           :_handle(NULL)
{
	init(dwNumThreads, dwGrowBy, dwMaxThreads =INFINITE, dwIdleTime =5000);
}

/// Initialize the thread pool.
bool ThreadPool::init(DWORD dwNumThreads, DWORD dwGrowBy, DWORD dwMaxThreads /* =INFINITE*/ , DWORD dwIdleTime/*=5000*/)
{
	if (!clean())
		return false;

	_numThreads=dwNumThreads;
	_growBy=dwGrowBy;
	_maxThreads=dwMaxThreads;
	_idleTime=dwIdleTime;

	DWORD errorcode;
	//assert handle? throw exception???
	_handle=::InitializeThreadPool(_numThreads,_growBy,_maxThreads,_idleTime,&errorcode);
	if(_handle==NULL)
	{
		glog(LOGFMT("InitializeThreadPool error , error NT code=%u"),errorcode);
		return false;
	}

	return true;
}

/// destructor
ThreadPool::~ThreadPool()
{
	clean();
}

bool ThreadPool::clean()
{
	if (NULL == _handle)
		return true;

	ReleaseThreadPool(_handle, POOL_RELEASE_WAIT);
	_handle = NULL;
	return true;
}

void ThreadPool::close_thread_handle(HANDLE threadhandle)
{
	ScThreadCloseHandle(_handle,threadhandle);
}

bool Thread::start()
{
	DWORD errCode;
	_handle = ScBeginThreadPoolThread(_pool._handle,this->_execute,this, DEFAULT_THREAD_START_TIMEOUT,&errCode);

#ifdef _DEBUG
	if (NULL == _handle)
		glog(LOGFMT("thread start error!"));
#endif // _DEBUG

	return (NULL != _handle);
}

//when the thread is running ,you can't invoke this,
//can't invoke in thread
void Thread::close()
{
	ScThreadCloseHandle(_pool._handle, this->_handle);
	
	this->_handle = NULL;
}

//void Thread::ResetThreadPool(ThreadPool& new_pool)
//{
//	_pool = new_pool;
//}

bool Thread::wait(DWORD timeoutMillis)const
{
	DWORD result = ::WaitForSingleObject(_handle, timeoutMillis);

	if (result == WAIT_TIMEOUT)
		return false;
	
	if (result == WAIT_OBJECT_0)
		return true;

#ifdef _DEBUG
	//can happen?yes, when handle==NULL

	printf(LOGFMT("Error: %x,handle=%08x\n"),::GetLastError(), (DWORD)_handle);
#endif // _DEBUG
	return true;
}


void Thread::Sleep(timeout_t msec)
{
	::SleepEx(msec, false);
}

void Thread::setStatus(int sts)
{
	_status = (Status)sts;
}

} // namespace common
} // namespace ZQ
