// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: Log.cpp,v 1.15 2004/08/09 10:07:00 jshen Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Base Logger
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Log.cpp $
// 
// 8     7/01/15 4:41p Zhiqiang.niu
// sync with git
// 
// 7     3/19/15 11:08a Hui.shao
// 
// 6     3/19/15 10:53a Hui.shao
// 
// 5     12/31/13 5:15p Hui.shao
// avoid to export variables from dll
// 
// 4     11/08/13 10:50a Hui.shao
// 
// 3     6/13/13 6:04p Hui.shao
// enlarged the hex line size from 16 to 32 bytes
// 
// 2     1/30/13 10:43a Hongquan.zhang
// work around due to vsnprintf bug in linux libc
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 28    10-06-28 18:03 Yixin.tian
// add code catch  writeMessage() exception
// 
// 27    09-11-04 19:55 Xiaohui.chai
// correct the event id
// 
// 26    09-10-21 14:40 Hui.shao
// stop guessing appname in filelog if it is not given
// 
// 25    09-10-07 17:23 Hui.shao
// 
// 24    09-07-21 12:24 Hui.shao
// 
// 23    09-07-21 12:01 Hui.shao
// added text-only dump to ease searching
// 
// 22    09-07-02 10:48 Yixin.tian
// 
// 21    09-05-11 18:29 Fei.huang
// 
// 20    08-03-06 16:21 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 19    08-03-03 17:42 Yixin.tian
// merged changes for linux
// 
// 18    07-05-11 19:44 Guan.han
// 
// 17    07-05-11 19:37 Guan.han
// 
// 16    07-05-10 12:05 Guan.han
// 
// 15    07-05-09 17:35 Guan.han
// 
// 14    07-02-09 17:11 Hongquan.zhang
// 
// 13    07-02-01 13:15 Guan.han
// 
// 12    06-12-18 14:59 Cary.xiao
// 
// 11    06-12-14 19:31 Shuai.chen
// 
// 10    06-12-14 10:15 Shuai.chen
// 
// 8     06-11-17 12:36 Ken.qian
// 
// 7     06-10-13 14:16 Ken.qian
// 
// 6     05-11-15 10:23 Ken.qian
// 
// 5     05-11-14 14:12 Ken.qian
// Revision 1.15  2004/08/09 10:07:00  jshen
// add wchar support to log
//
// Revision 1.14  2004/07/29 06:55:39  wli
// Add unicode support
//
// Revision 1.13  2004/07/22 06:15:19  shao
// global logger
//
// Revision 1.12  2004/06/15 01:28:33  mwang
// some bugs fixed
//
// Revision 1.11  2004/06/13 03:46:50  mwang
// no message
//
// Revision 1.10  2004/05/27 07:01:53  mwang
// no message
//
// Revision 1.9  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.8  2004/05/17 06:46:41  mwang
// no message
//
// Revision 1.7  2004/05/11 05:47:28  shao
// method to switch global logger
//
// Revision 1.6  2004/05/09 03:51:09  shao
// added classes SysLog and DebugMsg
//
// Revision 1.5  2004/04/30 05:17:46  shao
// log definition
//
// ===========================================================================

#include "Log.h"
#include "CombString.h"

#include "Locks.h"

extern "C"
{
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
}

#ifdef ZQ_OS_MSWIN
    #include "ZqMessages.h" // message code definition
	#pragma comment(lib, "Advapi32.lib")
#else
#define _vsnprintf		vsnprintf
#define _vsnwprintf		vswprintf
#define _snprintf		snprintf
#define _snwprintf		vswprintf

#include <ctype.h>
#endif

#define LOG_LINE_MAX_BUF     (2048)

namespace ZQ {
namespace common {

static const char* _levelstrs[8]={"EMERG", "ALERT", "CRIT", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};

Log::Log(const int verbosity /* =L_ERROR */)
{
	setVerbosity(verbosity);
	_pTaskMutex = new Mutex();
}

Log::~Log()
{
	if(_pTaskMutex)
	{
		delete _pTaskMutex;
		_pTaskMutex = NULL;
	}
}

const char* Log::getVerbosityStr(int level)
{
	return _levelstrs[level % (sizeof(_levelstrs)/sizeof(char*))];
}

int Log::getVerbosityInt(const char *levelstr)
{
	for (int i=0; i <8; i ++)
	{
#ifdef ZQ_OS_MSWIN
		if (stricmp(_levelstrs[i], levelstr) == 0)
			return i;
#else
		if (strcasecmp(_levelstrs[i], levelstr) == 0)
			return i;
#endif
	}
	return -1;
}

const char* Log::getTimeStampStr(char* str, const int size/*=-1*/, bool compacted/*=false*/)
{
	static const char *monthstr[]= {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

	if (str == NULL)
		return NULL;

	struct tm *timeData;
	time_t     longTime;

	time(&longTime);
	timeData = localtime(&longTime);

	if (compacted)
	{
		uint32 dsec= (timeData->tm_hour *60 +timeData->tm_min)*60 +timeData->tm_sec;
		uint16 dday= (timeData->tm_mon+1) *0x100 +timeData->tm_mday;

		if (size<0)
			sprintf(str, "%03x%05x", dday, dsec);
		else
			snprintf(str, size, "%03x%05x", dday, dsec);

	}
	else
	{
		if (size<0)
			sprintf(str, "%3s %02d %02d:%02d:%02d",
			monthstr[timeData->tm_mon],
			timeData->tm_mday,
			timeData->tm_hour,
			timeData->tm_min,
			timeData->tm_sec);
		else
			snprintf(str, size, "%3s %02d %02d:%02d:%02d",
			monthstr[timeData->tm_mon],
			timeData->tm_mday,
			timeData->tm_hour,
			timeData->tm_min,
			timeData->tm_sec);
	}

	return str;
}

int Log::getVerbosity()
{
	return _verbosity;
}

void Log::setVerbosity(int newlevel)
{
	if (newlevel < L_EMERG)
		_verbosity = L_EMERG;
	else if (newlevel > L_DEBUG)
		_verbosity = L_DEBUG;
	else _verbosity = newlevel;
}

const char* Log::getVerbosityStr()
{
	return getVerbosityStr(_verbosity);
}

void Log::debug( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_DEBUG, fmt,args);
}

void Log::info( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_INFO, fmt,args);
}

void Log::notice( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_NOTICE, fmt,args);
}

void Log::warning( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_WARNING, fmt,args);
}

void Log::error( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_ERROR, fmt,args);
}

void Log::crit( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_CRIT, fmt,args);
}

void Log::emerg( const char* fmt, ... ) {
	va_list args;
	va_start(args, fmt);
	return formatLogMessage(Log::L_EMERG, fmt,args);
}

void Log::formatLogMessage( int level, const char* fmt, va_list args) {
	if ((level & 0xff) > _verbosity)
		return;

	char msg[LOG_LINE_MAX_BUF];

	int nCount = _vsnprintf(msg, LOG_LINE_MAX_BUF -8, fmt, args);
	if(nCount == -1)
	{
		msg[0] = '\0';
	}
	else
	{
		msg[MIN(nCount, LOG_LINE_MAX_BUF -8)] = '\0';
	}

	try {
		writeMessage(msg, level & 0xff);
	}
	catch(...) {}

}

Log& Log::operator()(int level, const char *fmt, ...)
{
	if ((level & 0xff) > _verbosity)
		return *this;
	va_list args;
	va_start(args, fmt);
	formatLogMessage(level, fmt, args);
	
	return *this;
}

Log& Log::operator()(int level, const wchar_t *fmt, ...)
{
	if ((level & 0xff) > _verbosity)
		return *this;

	wchar_t msg[LOG_LINE_MAX_BUF];
	va_list args;

	va_start(args, fmt);
	int nCount = _vsnwprintf(msg, LOG_LINE_MAX_BUF -8, fmt, args);
	va_end(args);
	if(nCount == -1)
	{
		msg[LOG_LINE_MAX_BUF-8] = L'\0';
	}
	else
	{
		msg[nCount] = L'\0';
	}

	try {
		writeMessage(msg, level & 0xff);
	}
	catch(...) {}

	return *this;
}

Log& Log::operator()(const char *fmt, ...)
{
	char msg[LOG_LINE_MAX_BUF];
	va_list args;

	va_start(args, fmt);
	int nCount = _vsnprintf(msg, LOG_LINE_MAX_BUF-8, fmt, args);
	va_end(args);
	if(nCount == -1)
	{
		msg[LOG_LINE_MAX_BUF-8] = '\0';
	}
	else
	{
		msg[nCount] = '\0';
	}
	
	try {
		writeMessage(msg, _verbosity);
	}
	catch(...) {}

	return *this;
}

Log& Log::operator()(const wchar_t *fmt, ...)
{
	wchar_t msg[LOG_LINE_MAX_BUF];
	va_list args;

	va_start(args, fmt);
	//vswprintf(msg, fmt, args);
	int nCount = _vsnwprintf(msg, LOG_LINE_MAX_BUF-8, fmt, args);
	va_end(args);
	if(nCount == -1)
	{
		msg[LOG_LINE_MAX_BUF-8] = L'\0';
	}
	else
	{
		msg[nCount] = L'\0';
	}

	try {
		writeMessage(msg, _verbosity);
	}
	catch(...) {}

	return *this;
}

Log& Log::operator()(void)
{
	return *this;
}

#define HEX_LINE_SZ (8*4)

static void dumpHexLine(char* buffer, const unsigned char* data, 
			   int pos, int size, const char* hint)
{
	int i;
	int bufPos = 0;
	int len;

	if (hint) {
		len = sprintf(&buffer[bufPos], "%s ", hint);
		bufPos += len;
	}

	len = sprintf(&buffer[bufPos], "%08x ", pos);
	bufPos += len;
	for (i = 0; i < HEX_LINE_SZ; i++)
	{
		if (i < size) 
		{
			len = sprintf(&buffer[bufPos], "%02x ", data[i]);
			bufPos += len;
		}
		else
		{
			len = sprintf(&buffer[bufPos], "   ");
			bufPos += len;
		}

		if (i%8 == 7)
			buffer[bufPos ++] = ' ';
	}

//	buffer[bufPos ++] = ' ';
	buffer[bufPos ++] = '|';
	for (i = 0; i < HEX_LINE_SZ; i ++)
	{
		if (i < size)
		{
			if (isgraph(data[i]))
				buffer[bufPos ++] = data[i];
			else
				buffer[bufPos ++] = '.';
		}
		else
			buffer[bufPos ++] = ' ';
	}

	//buffer[bufPos ++] = '\n';
	buffer[bufPos ++] = '\0';
}

void Log::hexDump(const int level, const void* startPos, const int size, 
				  const char* hint /* = NULL */, bool textOnly /*=false*/)
{
	if ((level & 0xff) > _verbosity)
		return;

	if (startPos == NULL || size <= 0)
		return;

	if (textOnly)
	{
		textDump(level, startPos, size, hint);
		return;
	}

	unsigned char* data = (unsigned char* )startPos;
	char buffer[0x200];

	int lineSize;

	for (int i = 0; i < size; i +=HEX_LINE_SZ)
	{
		if ((i + HEX_LINE_SZ) > size) 
			lineSize = size - i;
		else
			lineSize = HEX_LINE_SZ;
	
		try{
			dumpHexLine(buffer, &data[i], i, lineSize, hint);
			writeMessage(buffer, level);
		}
		catch(...) {}
	}
}

void Log::textDump(const int level, const void* startPos, const int size, 
				  const char* hint /* = NULL */)
{
	if ((level & 0xff) > _verbosity)
		return;

#define OMMITTED_SUFFIX ">>omitted"
	char buffer[LOG_LINE_MAX_BUF];
	unsigned char* data = (unsigned char* )startPos;
	int bufPos = 0;

	if (hint)
		bufPos += sprintf(&buffer[bufPos], "%s ", hint);

	int bufLeft = sizeof(buffer) - bufPos - strlen(OMMITTED_SUFFIX) -8;

	int i =0;
	for (i = 0; i <size	&& (i< bufLeft || size < (int)(i +::strlen(OMMITTED_SUFFIX))); i++)
		buffer[bufPos ++] = (' '==data[i] || isgraph(data[i])) ? data[i] : '.';

	if (i < size)
		bufPos += sprintf(&buffer[bufPos], OMMITTED_SUFFIX);

	buffer[bufPos++] = '\0';
	buffer[bufPos++] = '\0';
#undef OMMITTED_SUFFIX

	try {
		writeMessage(buffer, level);
	}
	catch(...) {}
}

//Log NullLogger;
#ifdef _DEBUG
Log NullLogger(Log::L_DEBUG);
#else
Log NullLogger;
#endif

static Log* pGlog = &NullLogger;

void setGlogger(Log* pLogger /*=NULL*/)
{
	pGlog = (pLogger!=NULL) ? pLogger : &NullLogger;
}

Log* getGlogger()
{
	return (pGlog !=NULL) ? pGlog : &NullLogger;
}

// -----------------------------
// class SysLog
// -----------------------------
#ifdef ZQ_OS_MSWIN //ZQ_OS_MSWIN
SysLog::SysLog() 
: _hEventSource(NULL), _machine(NULL), _appname(NULL)
{
}

SysLog::~SysLog()
{
	if (_hEventSource)
		::DeregisterEventSource(_hEventSource);
}

void SysLog::open(const char* applicationName, const int verbosity/*=L_ERROR*/, const char* machine /*=NULL*/)
{
	if (machine != NULL && strlen(machine)>0)
	{
		_machine = _names;
		strcpy(_machine, machine);
	}
	
	if (applicationName != NULL && strlen(applicationName)>0)
	{
		_appname = (_machine==NULL) ? _names : (_machine + strlen(_machine) +2);
		strcpy(_appname, applicationName);
	}

	setVerbosity(verbosity);
} 

SysLog::SysLog(const char* applicationName, const int verbosity/*=L_ERROR*/, const char* machine /*=NULL*/)
       : _hEventSource(NULL), _machine(NULL), _appname(NULL)
{
	open(applicationName, verbosity, machine);
}

void SysLog::writeMessage(const char *msg, int level /*=-1*/)
{
	WORD wType = EVENTLOG_INFORMATION_TYPE;
	DWORD eventId = ZQ_GENERIC_INFO;
	// convert level to EventLog type
	switch(level)
	{
	case L_EMERG:
	case L_ALERT:
	case L_CRIT:
	case L_ERROR:
		wType = EVENTLOG_ERROR_TYPE;
        eventId = ZQ_GENERIC_ERROR;
		break;
		
	case L_WARNING:
		wType = EVENTLOG_WARNING_TYPE;
        eventId = ZQ_GENERIC_WARNING;
		break;
		
	case L_NOTICE:
	case L_INFO:
	case L_DEBUG:
	default:
		wType = EVENTLOG_INFORMATION_TYPE;
        eventId = ZQ_GENERIC_INFO;
		break;
	}
#ifdef UNICODE
	const wchar_t* ps[2];
	CombString str = msg;
	ps[0] = str.c_str();
	ps[1] = NULL;

	try{
		// Check the event source has been registered and if not then register it now
		if (NULL == _hEventSource && NULL != _appname && strlen(_appname) >0)
		{
			CombString sMachine = _machine;
			CombString sAppname = _appname;
			_hEventSource = ::RegisterEventSource(sMachine.c_str(), sAppname.c_str());
		}

		if (!_hEventSource)
			return;

		// put the event
		::ReportEvent(_hEventSource, wType, 0, eventId, NULL, // sid
						(WORD)((ps[0]==NULL) ? 0:1), 0, ps, NULL);

		::DeregisterEventSource(_hEventSource);
		_hEventSource = NULL;
	}
	catch (...) {}

#else
	const char* ps[2];
    ps[0] = msg;
    ps[1] = NULL;
	
	try{
		// Check the event source has been registered and if not then register it now
		if (NULL == _hEventSource && NULL != _appname && strlen(_appname) >0)
		{
			_hEventSource = ::RegisterEventSource(_machine,  // local machine
								_appname); // source name
		}

		if (!_hEventSource)
			return;

		// put the event
		::ReportEvent(_hEventSource, wType, 0, eventId, NULL, // sid
					(WORD)((ps[0]==NULL) ? 0:1), 0,	ps, NULL);

		::DeregisterEventSource(_hEventSource);
		_hEventSource = NULL;
	}
	catch (...) {}

#endif // UNICODE

}

#else//NON-ZQ_OS_MSWIN

SysLog::SysLog()       
{
	memset(_proname,0,sizeof(_proname));
	_facility = LOG_USER;
}

SysLog::~SysLog()
{
	closelog();	
}

SysLog::SysLog(const char* applicationName, const int option /*= LOG_ODELAY*/, const int facility /*= LOG_USER*/)
{
	memset(_proname,0,sizeof(_proname));
	_facility = facility;
	open(applicationName, option, facility);
}

void SysLog::open(const char* applicationName,const int level /*= L_ERROR*/, const int option /*= LOG_ODELAY*/, const int facility /*= LOG_USER*/)
{
	if(applicationName != NULL && strlen(applicationName) > 0)
			strcpy(_proname,applicationName);
	else
		strcpy(_proname,"zqcommon");

	openlog(_proname,option,facility);//open sys log
	
	setVerbosity(level);
} 

void SysLog::writeMessage(const char *msg, int level /*=-1*/)
{
	int nlevel = LOG_ERR;

	switch(level)
	{
	case L_EMERG:
		nlevel = LOG_EMERG;
		break;
	case L_ALERT:
		nlevel = LOG_ALERT;
		break;
	case L_CRIT:
		nlevel = LOG_CRIT;
		break;
	case L_ERROR:
		nlevel = LOG_ERR;
		break;		
	case L_WARNING:
		nlevel = LOG_WARNING;
		break;		
	case L_NOTICE:
		nlevel = LOG_NOTICE;
		break;
	case L_INFO:
		nlevel = LOG_INFO;
		break;
	case L_DEBUG:
		nlevel = LOG_DEBUG;
	default:
		nlevel = LOG_ERR; //default level:LOG_ERR
		break;
	}

	//write syslog
//	syslog(_facility | nlevel,msg);
	syslog(nlevel,msg);

}
#endif

void xlog(Log &logger, int level, const char *fmt, ...)
{
//	if (logger == NULL) return;

	char msg[LOG_LINE_MAX_BUF];
	va_list args;
	va_start(args, fmt);
	_snprintf(msg, LOG_LINE_MAX_BUF-2, fmt, args);
	va_end(args);

	logger(level, msg);
}

void xlog(Log &logger, int level, const wchar_t *fmt, ...)
{
	//	if (logger == NULL) return;

	wchar_t msg[LOG_LINE_MAX_BUF];
	va_list args;
	va_start(args, fmt);
	_snwprintf(msg, LOG_LINE_MAX_BUF-8, fmt, args);
	va_end(args);

	logger(level, msg);
}
} // namespace common
} // namespace ZQ
