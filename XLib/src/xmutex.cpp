// ===========================================================================
// Copyright (c) 2005 by
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
//
// ----------------------------
//
// Name  : xmutex.cpp
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
#include <time.h>
#include <errno.h>
#include <xmutex.h>
#include "./platform/platform.h"
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
XMutex::XMutex(const char* pszName /* = NULL*/, bool bInitiallyOwn /*= FALSE*/)
{
	pthread_mutexattr_t attr;
	m_nErr = pthread_mutexattr_init(&attr);
	CheckError(EINVAL);
	if (pszName != NULL) {
		m_nErr = _pthread_mutexattr_setname_np(&attr, pszName);
		CheckError(m_nErr);
	}

	if (bInitiallyOwn) {
#ifdef PTHREAD_MUTEX_OWNERTERM_NP
		m_nErr = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_OWNERTERM_NP);
#else
		m_nErr = EINVAL;
#endif
		CheckError(m_nErr);
	}

	m_nErr = pthread_mutex_init(&m_pmutex, &attr);
	pthread_mutexattr_destroy(&attr);
	CheckError(m_nErr);
}

XMutex::~XMutex()
{
	Unlock();
	pthread_mutex_destroy(&m_pmutex);
}

/* virtual */ bool XMutex::Lock(int nTimeout /* = TIMEO_INFINITE*/ )
{
	if (TIMEO_INFINITE == nTimeout) {
		m_nErr = pthread_mutex_lock(&m_pmutex);
		if (m_nErr != 0)
			return false;
	} else {
		struct timespec t;
		t.tv_sec = time(NULL) + nTimeout / MSEC_SEC_ZOODSEE;
		t.tv_nsec = (nTimeout % MSEC_SEC_ZOODSEE) * NSEC_MSEC_ZOODSEE;
		m_nErr = pthread_mutex_timedlock(&m_pmutex, &t);
		if (m_nErr != 0)
			return false;
	}

	return true;
}

/* virtual */ bool XMutex::Unlock()
{
	m_nErr = pthread_mutex_unlock(&m_pmutex);
	if (m_nErr != 0)
		return false;
	return true;
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
