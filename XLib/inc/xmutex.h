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
// Name  : xmutex.h
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

#ifndef _XMUTEX_H_
#define _XMUTEX_H_

#include <xsynch.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

class XMutex : public XSynch {
public:
	XMutex(const char* pszName = NULL, bool bInitiallyOwn = false);
	virtual ~XMutex();
	virtual bool Lock(int nTimeout = TIMEO_INFINITE);
	virtual bool Unlock();

protected:
	pthread_mutex_t		m_pmutex;
};

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XMUTEX_H_
