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
// Name  : xevent.cpp
// Author: Cary (cary.xiao@i-zq.com)
// Date  : 07/07/2005
// Desc  : C++ thread library for multi-platform, it based on pthreads.
//
// Revision History:
//
//
// ----------------------------
//
//
// ===========================================================================
#include <xevent.h>
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
XEvent::XEvent(bool bManual /* = false */) : m_bManual(bManual)
{
	SetSignal(false);
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init(&attr);
	pthread_rwlock_init(&m_prwlock, &attr);
	pthread_rwlockattr_destroy(&attr);
}

/* virtual */ XEvent::~XEvent()
{
}

bool XEvent::GetSignal()
{
	pthread_rwlock_rdlock(&m_prwlock);
	bool bRet = m_bSignal;
	pthread_rwlock_unlock(&m_prwlock);
	return bRet;
}

bool XEvent::SetSignal(bool bSignal)
{
	m_nErr = pthread_rwlock_wrlock(&m_prwlock);
	if (m_nErr != 0)
		return false;
	m_bSignal = bSignal;
	pthread_rwlock_unlock(&m_prwlock);
	return true;
}

/* virtual */ bool XEvent::Lock(int nTimeout /* = TIMEO_INFINITE */)
{
	if (GetSignal())
		return true;
	if (!XCond::Lock(nTimeout))
		return false;
	return Unlock();
}

///* virtual */ bool XEvent::Unlock()
//{
//	return XCond::Unlock();
//}

bool XEvent::PulseEvent()
{
	if (!SetEvent())
		return false;
	return ResetEvent();
}

bool XEvent::SetEvent()
{
	bool bRet;
	if (m_bManual) {
		bRet = Broadcast();
	}

	else {
		bRet = Signal();
	}

	if (!bRet)
		return false;
	if (m_bManual) {
		SetSignal(true);
	} else {
		SetSignal(false);
	}

	return true;
}

bool XEvent::ResetEvent()
{
	return SetSignal(false);
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
