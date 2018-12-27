// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: Exception.h,v 1.8 2004/05/26 09:32:35 mwang Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define common exceptions
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Exception.h $
// 
// 3     12/19/12 5:26p Hongquan.zhang
// 
// 2     12/19/12 3:45p Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     08-03-03 16:45 Yixin.tian
// merged changes for linux
// 
// 7     06-12-25 15:36 Hui.shao
// specify logger in _throw
// Revision 1.8  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.7  2004/05/09 03:54:15  shao
// no message
//
// Revision 1.4  2004/04/27 02:28:06  shao
// addjusted file header format
//
// Revision 1.3  2004/04/21 04:23:45  shao
// no message
//
// Revision 1.1  2004/04/20 09:35:07  shao
// initial created
//
// ===========================================================================

#ifndef	__ZQ_COMMON_EXCEPTION_H__
#define	__ZQ_COMMON_EXCEPTION_H__

#include "ZQ_common_conf.h"
#include "Log.h"

#include <string>
#include <exception>

extern "C" {
#include <stdarg.h>
}

namespace ZQ {
namespace common {

class ZQ_COMMON_API Exception;
class ZQ_COMMON_API IOException;
class ZQ_COMMON_API SyncException;

// -----------------------------
// class Exception
// -----------------------------
/// Mainline exception handler, this is the root for all exceptions and
/// assures the ansi C++ exception class hierarchy is both followed and
/// imported into the gnu class hierarchy.
class Exception : public std::exception 
{
private:
	std::string _what;

public:
	Exception(const std::string& what_arg); // throw();
	virtual ~Exception(); // throw();
	virtual const char *getString() const; 
};


// -----------------------------
// class IOException
// -----------------------------
/// A sub-hierarchy for I/O related classes.
class IOException : public Exception
{
public:
	IOException(const std::string &what_arg); // throw();
	virtual ~IOException(); // throw();
};

// -----------------------------
// class SyncException
// -----------------------------
/// A sub-hierarchy for sync related classes
class SyncException : public IOException
{
public:
	SyncException(const std::string &what_arg) : IOException(what_arg) {}
};

/*
template <typename _EX>
void _throw (const char* fmt, ...)
{
	char msg[2048];
	va_list args;

	va_start(args, fmt);
//	vsnprintf(msg, 2040, fmt, args);
	vsprintf(msg, fmt, args);
	va_end(args);

	glog(Log::L_ERROR, msg);
	throw _EX(msg);
}
*/

template <class _EX>
class _throw
{
public:
	_throw(const char* fmt, ...) PRINTFLIKE(2, 3)
	{
		char msg[2048];
		va_list args;

		va_start(args, fmt);
	//	vsnprintf(msg, 2040, fmt, args);
		vsprintf(msg, fmt, args);
		va_end(args);

		try{ glog(Log::L_ERROR, msg); } catch(...) {}

		throw _EX(msg);
	}

	_throw(Log& logger, const char* fmt, ...) PRINTFLIKE(3, 4)
	{
		char msg[2048];
		va_list args;

		va_start(args, fmt);
	//	vsnprintf(msg, 2040, fmt, args);
		vsprintf(msg, fmt, args);
		va_end(args);

		try{ logger(Log::L_ERROR, msg); } catch(...) {}

		throw _EX(msg);
	}
};

} // namespace common
} // namespace ZQ


#endif // __ZQ_COMMON_EXCEPTION_H__
