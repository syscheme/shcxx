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
// Name  : xthread.cpp
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
#include <xthread.h>
#include "./platform/platform.h"
#ifndef __VOID
#define __VOID(p)		((void* )(p))
#endif
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
//////////////////////////////////////////////////////////////////////////
// Implementations of class XThread
XThread::XThread()
{
	m_status = STATUS_INITIAL;
	m_exit_status = __VOID(0);
	m_err = 0;
}

/* virtual */ XThread::~XThread()
{
	Terminate(0);
}

int	XThread::_ErrNo()
{
	return m_err;
}

/* static */ void* XThread::_Thread_Routine(void* arg)
{
	if (arg == NULL) {
		assert(false);
		return __VOID(0);
	}

	
	XThread* thread = (XThread* )arg;
	try {
		if (!thread->Init())
			return __VOID(0);
		thread->m_status = STATUS_RUNNING;
		thread->m_exit_status = thread->Run();
		thread->m_status = STATUS_FINISHED;
	}

	catch(...) {
		// no body
	}

	thread->Final();
	return __VOID(thread->m_exit_status);
}

	
/* virtual */ bool XThread::Init(void)
{
	return true;
}

/* virtual */ void XThread::Final(void)
{
	// no body
}

void XThread::Exit(Exit_Status status)
{
	pthread_exit(status);
}

bool XThread::Start()
{
	m_err = pthread_create(&m_pthread, NULL, _Thread_Routine, (void* )this);
	return (0 == m_err);
}

bool XThread::Suspend(void)
{
	if (pthread_equal(pthread_self(), m_pthread)) {
		assert(false);
		return false;
	}

	/// unimplemented
	assert(false);
	return false;
}

bool XThread::Resume(void)
{
	if (pthread_equal(pthread_self(), m_pthread)) {
		assert(false);
		return false;
	}

	// unimplemented
	assert(false);
	return false;
}

bool XThread::Terminate(Exit_Status status)
{
	if (!_pthread_terminate(&m_pthread, status))
		return false;
	m_exit_status = status;
	m_status = STATUS_FINISHED;
	return true;
}

inline XThread::Thread_Status XThread::GetStatus(void)
{
	return m_status;
}

bool XThread::IsRunning(void)
{
	return STATUS_RUNNING == m_status;
}

XThread::Exit_Status XThread::GetExitStatus()
{
	if (m_status != STATUS_FINISHED) {
		assert(false); // invalid status
		return 0;
	}

	return m_exit_status;
}

pthread_t& XThread::_GetThread()
{
	return m_pthread;
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
