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
// Name  : xsynch.cpp
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
#include <xsynch.h>
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
XSynchException::XSynchException(int nCode)
{
	m_nCode = nCode;
}

/* virtual */ XSynchException::~XSynchException()
{
}

int XSynchException::GetCode()
{
	return m_nCode;
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ XSynch::~XSynch()
{
}

int	XSynch::GetError()
{
	return m_nErr;
}

void XSynch::CheckError(int nErr)
{
	if (m_nErr != 0)
		throw XSynchException(m_nErr);
}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
