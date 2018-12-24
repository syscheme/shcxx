// ===========================================================================
// Copyright (c) 2005 by
// syscheme, Shanghai
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
// Name  : xsynch.h
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

#ifndef _XSYNCH_H_
#define _XSYNCH_H_

#include <pthread.h>
#include <sched.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#define TIMEO_INFINITE			0xffffffff
#define NSEC_MSEC_ZOODSEE		1000000
#define MSEC_SEC_ZOODSEE		1000

class XSynchException {
public:
	XSynchException(int nCode);
	virtual ~XSynchException();
	int GetCode();
protected:
	int		m_nCode;
};

class XSynch {
public:
	virtual ~XSynch();

	/*
	 * Name:	Lock
	 * Parameters:
	 *			nTimeout	timeout interval, in milliseconds
	 * Return:
	 *			ture, successful; false failed.
	 * Description:
	 *			wait synch object while state is signaled.
	 */
	virtual bool Lock(int nTimeout /* = TIMEO_INFINITE*/ ) = 0;

	/*
	 * Name:	Unlock
	 * Parameters:
	 *			None
	 * Return:
	 *			ture, successful; false failed.
	 * Description:
	 *			the owner release the synch object(make it nonsignaled).
	 */
	virtual bool Unlock() = 0;

	/*
	 * Name:	GetError
	 * Parameters:
	 *			None
	 * Return:
	 *			the last error code.
	 * Description:
	 *			retrieves the last error code.
	 */
	int	GetError();

protected:

	// check the error code, throw a exception in due course
	void CheckError(int nErr);
protected:
	int		m_nErr;
};

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XSYNCH_H_
