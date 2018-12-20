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
// Name  : xcond.cpp
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
#include <xcond.h>
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
XCond::XCond()
{
	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	pthread_cond_init(&m_pcond, &condattr);
	pthread_condattr_destroy(&condattr);
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutex_init(&m_pmutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
}

/* virtual */ XCond::~XCond()
{
	pthread_mutex_lock(&m_pmutex);
	pthread_cond_destroy(&m_pcond);
	pthread_mutex_destroy(&m_pmutex);
}

/* virtual */ bool XCond::Lock(int nTimeout /* = TIMEO_INFINITE */ )
{
	m_nErr = pthread_mutex_lock(&m_pmutex);
	if (m_nErr != 0)
		return false;
	if (nTimeout == TIMEO_INFINITE) {
		m_nErr = pthread_cond_wait(&m_pcond, &m_pmutex);
		if (m_nErr != 0) {
			pthread_mutex_unlock(&m_pmutex);
			return false;
		}

		
	} else {
		struct timespec t;
		t.tv_sec = time(NULL) + nTimeout / MSEC_SEC_ZOODSEE;
		t.tv_nsec = (nTimeout % MSEC_SEC_ZOODSEE) * NSEC_MSEC_ZOODSEE;
		m_nErr = pthread_cond_timedwait(&m_pcond, &m_pmutex, &t);
		if (m_nErr != 0)
			return false;
	}

	return true;
}

/* virtual */ bool XCond::Unlock()
{
	m_nErr = pthread_mutex_unlock(&m_pmutex);
	if (m_nErr != 0)
		return false;
	return true;
}

bool XCond::Signal()
{
	m_nErr = pthread_cond_signal(&m_pcond);
	if (m_nErr != 0)
		return false;
	
	m_nErr = sched_yield();
	if (m_nErr != 0)
		return false;
	return true;
}

bool XCond::Broadcast()
{
	m_nErr = pthread_cond_broadcast(&m_pcond);
	if (m_nErr != 0)
		return false;
	
	m_nErr = sched_yield();
	if (m_nErr != 0)
		return false;
	return true;
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
