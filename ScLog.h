// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: ScLog.h,v 1.9 2004/08/05 07:08:37 wli Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define the Logger invokes SeaChange CLog APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ScLog.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// Revision 1.9  2004/08/05 07:08:37  wli
// create
//
// Revision 1.8  2004/07/29 06:55:39  wli
// Add unicode support
//
// Revision 1.7  2004/06/30 07:28:13  jshen
// remove writeLog
//

// Revision 1.6  2004/06/21 06:06:44  shao
// rolled back to 1.3
//
// Revision 1.3  2004/05/10 05:08:43  shao
// default loglevel
//
// Revision 1.1  2004/04/30 05:17:46  shao
// log definition
//
// ===========================================================================

#ifndef	__ZQ_COM_ScLOG_H__
#define	__ZQ_COM_ScLOG_H__

#include "Log.h"

namespace ZQ {
namespace common {

class ZQ_COMMON_API ScLog;

#define SCLOG_DEFAULT_FILESIZE (64*1024)
#define SCLOG_DEFAULT_BUFSIZE (4*1024)
#define SCLOG_DEFAULT_FLUSHINTERVAL (2)

// -----------------------------
// class ScLog
// -----------------------------
/// Log class that invokes SeaChange CLog APIs
class ScLog : public Log
{
public:
	
	/// constructor
	/// @param filename - filename of the logfile
	/// @param verbosity  - the verbosity level to compare with
	/// @param fileSize - max file size to roll over
	/// @param buffersize - if the log is buffered, buffersize must >0 
	/// @param flushInterval - log times between 2 flush 
	ScLog(const char* filename, const int verbosity=L_ERROR, int fileSize =SCLOG_DEFAULT_FILESIZE, int buffersize =SCLOG_DEFAULT_BUFSIZE, int flushInterval =SCLOG_DEFAULT_FLUSHINTERVAL);
	ScLog(const wchar_t* filename, const int verbosity=L_ERROR, int fileSize =SCLOG_DEFAULT_FILESIZE, int buffersize =SCLOG_DEFAULT_BUFSIZE, int flushInterval =SCLOG_DEFAULT_FLUSHINTERVAL);//20040729  new to support _UNICODE
	/// destructor
	~ScLog();

	/// flush data 
	virtual void flush();

public:
	// implemented writeMessage() by calling CLog APIs 
	
	virtual void writeMessage(const char *msg, int level=-1);
	virtual void writeMessage(const wchar_t* msg,int level = -1);//20040729  new to support _UNICODE
	HANDLE _hClog; /// handle to the opened logfile
};

} // namespace common
} // namespace ZQ

#endif __ZQ_COM_ScLOG_H__

