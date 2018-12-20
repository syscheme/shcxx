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
// Ident : $Id: ScLog.cpp,v 1.10 2004/08/09 10:07:16 jshen Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl a Logger that invokes SeaChange CLog APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ScLog.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 7     05-10-21 5:19 Jie.zhang
// add flush() and modify the writemessage, all change to output wide char
// version
// 
// 6     04-08-19 12:06 Jian.shen
// Revision 1.10  2004/08/09 10:07:16  jshen
// add wchar support to log
//
// Revision 1.9  2004/08/05 07:08:37  wli
// create
//
// Revision 1.8  2004/07/29 06:55:39  wli
// Add unicode support
//
// Revision 1.7  2004/06/30 07:26:36  jshen
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

#include "StdAfx.h" // SeaChange CLog.dll invoked MFC
#include "ScLog.h"
#include "CombString.h"
#define BLDCLOG   // DISABLE: Make UNICODE seamless to users of the DLL - REB
#include "CLog.h" // SeaChange CLog.dll apis

#pragma comment (lib, "clog" VODLIBEXT)

namespace ZQ {
namespace common {

#define SCLOG_EXEC_LOG_LEVEL 1
#define SCLOG_MIN_FILESIZE (16*1024)

// -----------------------------
// class ScLog
// -----------------------------
ScLog::ScLog(const char *filename, const int verbosity /*=L_ERROR*/, int fileSize /*=SCLOG_DEFAULT_FILESIZE*/, int buffersize /*=SCLOG_DEFAULT_BUFSIZE*/, int flushInterval /*=SCLOG_DEFAULT_FLUSHINTERVAL*/)
      :Log(verbosity), _hClog(INVALID_HANDLE_VALUE)
{
	if (fileSize < SCLOG_MIN_FILESIZE)
		fileSize = SCLOG_MIN_FILESIZE;
	
	if (flushInterval < SCLOG_DEFAULT_FLUSHINTERVAL)
		flushInterval = SCLOG_DEFAULT_FLUSHINTERVAL;	

	if (buffersize)
		_hClog = (buffersize>0)
		? ClogInitH((char*)filename, SCLOG_EXEC_LOG_LEVEL+1, fileSize, buffersize, flushInterval)
		: ClogInitH((char*)filename, SCLOG_EXEC_LOG_LEVEL+1, fileSize, 0, flushInterval);
		
}
ScLog::ScLog(const wchar_t *filename, const int verbosity /*=L_ERROR*/, int fileSize /*=SCLOG_DEFAULT_FILESIZE*/, int buffersize /*=SCLOG_DEFAULT_BUFSIZE*/, int flushInterval /*=SCLOG_DEFAULT_FLUSHINTERVAL*/)
:Log(verbosity), _hClog(INVALID_HANDLE_VALUE)
{
	if (fileSize < SCLOG_MIN_FILESIZE)
		fileSize = SCLOG_MIN_FILESIZE;

	if (flushInterval < SCLOG_DEFAULT_FLUSHINTERVAL)
		flushInterval = SCLOG_DEFAULT_FLUSHINTERVAL;

	if (buffersize)

		_hClog = (buffersize>0)
		? ClogInitHW((wchar_t *)filename, SCLOG_EXEC_LOG_LEVEL+1, fileSize, buffersize, flushInterval)
		: ClogInitHW((wchar_t *)filename, SCLOG_EXEC_LOG_LEVEL+1, fileSize, 0, flushInterval);
}
ScLog::~ScLog()
{
	if (_hClog != INVALID_HANDLE_VALUE)
	{
		ClogFlushH(_hClog);
		ClogTermH(&_hClog);
	}
	_hClog = INVALID_HANDLE_VALUE;
}

void ScLog::writeMessage(const char *msg, int level /*=-1*/)
{
	if (_hClog != INVALID_HANDLE_VALUE)
	{
#if 1
		wchar_t  wszMsg[4096*2];
		MultiByteToWideChar(CP_ACP, NULL, msg, -1, wszMsg, sizeof(wszMsg)/sizeof(wchar_t));
		ClogHW(_hClog, SCLOG_EXEC_LOG_LEVEL, (wchar_t*)wszMsg);
#else
		ClogH(_hClog, SCLOG_EXEC_LOG_LEVEL,  (char*)msg);
#endif
	}
}
void ScLog::writeMessage(const wchar_t *msg, int level /*=-1*/)//20040729  new to support _UNICODE
{
	if (_hClog != INVALID_HANDLE_VALUE)
		ClogHW(_hClog, SCLOG_EXEC_LOG_LEVEL, (wchar_t*)msg);
}

void ScLog::flush()
{
	if (_hClog != INVALID_HANDLE_VALUE)
	{
		ClogFlushH(_hClog);
	}
}

} // namespace common
} // namespace ZQ

