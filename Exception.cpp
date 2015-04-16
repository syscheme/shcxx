// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: Exception.cpp,v 1.6 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl common exceptions
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Exception.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// Revision 1.6  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.5  2004/04/28 06:25:02  shao
// doxy comment format
//
// Revision 1.4  2004/04/27 08:52:12  shao
// no message
//
// Revision 1.3  2004/04/27 02:28:06  shao
// addjusted file header format
//
// Revision 1.2  2004/04/21 03:25:50  shao
// included zq_common_conf.h
//
// ===========================================================================

#include "Exception.h"

namespace ZQ {
namespace common {

/// -----------------------------
/// class Exception
/// -----------------------------
Exception::Exception(const std::string& what_arg) throw()
          : _what(what_arg) 
{
}

Exception::~Exception() throw()
{
}

const char* Exception::getString() const 
{
	return _what.c_str();
}

/// -----------------------------
/// class IOException
/// -----------------------------
IOException::IOException(const std::string &what_arg) throw()
            :Exception(what_arg)
{
}

IOException::~IOException() throw()
{
}

} // namespace common
} // namespace ZQ
