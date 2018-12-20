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
// Name  : xevent.h
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

#ifndef _XEVENT_H_
#define _XEVENT_H_

#include <xcond.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

class XEvent : protected XCond {
public:
	XEvent(bool bManual = false);
	virtual ~XEvent();
	virtual bool Lock(int nTimeout = TIMEO_INFINITE);
	// virtual bool Unlock();

	/*
	 * Sets the state of the event to signaled (available), 
	 * releases any waiting threads, and resets it to nonsignaled (unavailable) automatically.
	 */
	bool PulseEvent();

	/*
	 * Sets the event to available (signaled) and releases any waiting threads.
	 */
	bool SetEvent();

	/*
	 * Sets the event to unavailable (nonsignaled).
	 */
	bool ResetEvent();

protected:
	bool GetSignal();
	bool SetSignal(bool bSignal);

protected:
	const bool			m_bManual;
	bool				m_bSignal;
	pthread_rwlock_t	m_prwlock;
};

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XEVENT_H_
