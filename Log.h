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
// Ident : $Id: Log.h,v 1.13 2004/08/09 10:06:56 jshen Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Base Logger
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Log.h $
// 
// 12    7/01/15 4:41p Zhiqiang.niu
// sync with git
// 
// 11    3/27/15 10:50a Hui.shao
// 
// 10    1/26/15 3:07p Ketao.zhang
// 
// 9     12/31/13 5:13p Hui.shao
// avoid to export variables from dll
// 
// 8     7/09/13 10:51a Hongquan.zhang
// 
// 7     2/07/13 11:22a Hui.shao
// 
// 6     12/19/12 3:45p Hui.shao
// 
// 5     12/19/12 2:37p Hongquan.zhang
// support printf linke function parameter list checking
// 
// 4     3/22/12 5:38p Hui.shao
// 
// 3     2/16/12 4:52p Hongquan.zhang
// 
// 2     3/08/11 2:13p Fei.huang
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 34    10-10-22 14:06 Hui.shao
// RTSPClient linkable
// 
// 33    10-10-22 13:47 Hui.shao
// 
// 32    10-10-08 17:18 Hui.shao
// added LogWrapper
// 
// 31    09-07-21 12:01 Hui.shao
// added text-only dump to ease searching
// 
// 30    09-01-09 18:23 Yixin.tian
// 
// 29    08-11-14 12:03 Jie.zhang
// 
// 28    08-11-11 16:53 Jie.zhang
// log format changed, thread id added
// 
// 27    08-03-11 10:45 Fei.huang
// 
// 26    08-03-06 15:12 Yixin.tian
// merrge for linux
// 
// 25    08-03-03 17:42 Yixin.tian
// merged changes for linux
//
// 24    08-02-27 14:15 Hui.shao
//
// 23    08-02-21 12:17 Jie.zhang
// add a new log format 
//
// 22    07-05-11 18:16 Guan.han
// Add a default constructor and a open function
// 
// 21    06-12-14 19:31 Shuai.chen
// 
// 20    06-12-14 10:07 Shuai.chen
// 
// 19    06-12-13 13:33 Hui.shao
// 
// 18    06-10-25 14:35 Jie.zhang
// 
// 17    06-10-18 11:35 Jie.zhang
// 
// 16    06-10-16 11:13 Jie.zhang
// add NEWLOGFMT
// 
// 10    05-06-23 19:19 Bernie.zhao
//
// 9     5/13/05 11:53a Hui.shao
// added screen trace macro
// 
// 8     4/21/05 3:35p Hui.shao
// 
// 7     04-12-02 12:05 Daniel.wang
// 
// 6     04-11-21 16:29 Jie.zhang
// 
// 5     04-08-20 11:22 Hui.shao
// make gettimestamp() static
// Revision 1.13  2004/08/09 10:06:56  jshen
// add wchar support to log
//
// Revision 1.12  2004/07/29 06:55:39  wli
// Add unicode support
//
// Revision 1.11  2004/07/22 06:15:19  shao
// global logger
//
// Revision 1.10  2004/07/10 12:54:21  shao
// fixed connection bug
//
// Revision 1.9  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.8  2004/05/11 05:47:28  shao
// method to switch global logger
//
// Revision 1.7  2004/05/09 05:46:56  shao
// export classes
//
// Revision 1.6  2004/05/09 03:51:09  shao
// added classes SysLog and DebugMsg
//
// Revision 1.5  2004/04/30 05:17:46  shao
// log definition
//
// ===========================================================================

#ifndef	__ZQ_COM_LOG_H__
#define	__ZQ_COM_LOG_H__

#include "ZQ_common_conf.h"

extern "C" {
#include <stdio.h>
#ifndef WIN32
#include <syslog.h>
#include <sys/types.h>
#include <sys/syscall.h>

#endif
}

#define glog ( *ZQ::common::getGlogger() )

namespace ZQ {
namespace common {

class ZQ_COMMON_API Log;
class ZQ_COMMON_API DebugMsg;
class ZQ_COMMON_API SysLog;
class ZQ_COMMON_API LogWrapper;
class Mutex;

ZQ_COMMON_API void setGlogger(Log* pLogger =NULL);
ZQ_COMMON_API Log* getGlogger(void);

ZQ_COMMON_API void xlog(Log &logger, int level, const char *sfn, int line, const char *fmt, ...)  PRINTFLIKE(5, 6);
ZQ_COMMON_API void xlog(Log &logger, int level, const wchar_t *sfn, int line, const wchar_t *fmt, ...);

//#define LOG_DEBUG			(0x400)
//#define DUMP_BYTES_PER_LINE (0x10)

// -----------------------------
// class Log
// -----------------------------
/// Base log class, all other log implementation inherineted from it has to
/// implement void writeMessage(const char *msg). Log itself is a NULL logger

class Log
{
	friend class LogWrapper;
public:

	typedef enum  //	level range: value 0~7
	{
		L_EMERG=0, L_ALERT,	///< are those fatal problem will crash program or even the machine
		L_CRIT,		///< are the problems that stops current processing and may also harmful to the program itself
		L_ERROR,	///< are those problems that will quit the current processing
		L_WARNING,	///< lists some minor problem or unexpected problem but those won't
					///< affect the transaction continue to be handled
		L_NOTICE,	///< confirms a transaction
		L_INFO,		///< a level to confirm a major step has be performed
					///< some time also confirms the transaction step
		L_DEBUG		///< is a detailed tracing level for debug
	} loglevel_t;
	
	/// constructor
	/// @param verbosity  - the verbosity level to compare with
	Log(const int verbosity =L_ERROR);
	virtual ~Log();
	
	/// convert an integer level value to a display level string
	/// @param level - log level
	/// @return      - log level display string
	static const char *getVerbosityStr(int level);
	
	/// convert a display level string to an integer level value
	/// @param levelstr - log level display string
	/// @return         - log level
	static int getVerbosityInt(const char *levelstr);
	
	/// generates a time stamp string
	/// @param str       -  space where to keep the output string
	/// @param size      -  max size of the space str specified, negative if skip the qualification
	/// @param compacted - if true, the output string will be %03x%05x", dday, dsec
	///                    else it will be in "MMM DD HH:mm:ss"
	/// @return         - the stamp string
	static const char* getTimeStampStr(char* str, const int size/*=-1*/, bool compacted/*=false*/);
	
	/// get the default verbosity log level
	int				getVerbosity();
	/// get the default verbosity log level string
	const char*		getVerbosityStr();
	
	/// set the default verbosity log level
	void			setVerbosity(int newlevel = L_ERROR);

	///add helper functions
	void			debug( const char* fmt, ... ) PRINTFLIKE(2,3);
	void 			info( const char* fmt, ... ) PRINTFLIKE(2,3);
	void			notice( const char* fmt, ... ) PRINTFLIKE(2,3);
	void			warning( const char* fmt, ... ) PRINTFLIKE(2,3);
	void			error( const char* fmt, ... ) PRINTFLIKE(2,3);
	void			crit( const char* fmt, ... ) PRINTFLIKE(2,3);
	void			emerg( const char* fmt, ... ) PRINTFLIKE(2,3);
	/// full log entry
	/// @param level   - Log level to write the message
	/// @param fmt     - format string
	/// @param ...     - the items to be filled into the message
	Log& operator()(int level, const char *fmt, ...) PRINTFLIKE(3,4);
	Log& operator()(int level, const wchar_t *fmt, ...) ;
	
	/// simple log entry, using the current verbosity as the log level
	/// @param fmt     - format string
	/// @param ...     - the items to be filled into the message
	Log& operator()(const char *fmt, ...) PRINTFLIKE(2,3);
	Log& operator()(const wchar_t *fmt, ...);
	Log& operator()(void);
	
	/// flush data 
	virtual void flush(){}

	void hexDump(const int level, const void* startPos, const int size, const char *hint=NULL, bool textOnly=false);
	void textDump(const int level, const void* startPos, const int size, const char* hint= NULL);

protected:
	/// have to be implemented in the child, do nothing here as a NULL logger
	/// @param msg     - the log msg body to write down
	/// @param level   - log level. the verbosity testing has already been
	///                  performed when this method is called. the subclass
	///                  can ignore this paramter mostly
	virtual void writeMessage(const char *msg, int level=-1)
	{
#ifdef _DEBUG
		printf("%02x %s\n", level, msg);
#endif //
	}
	virtual void writeMessage(const wchar_t *msg , int level = -1)
	{
#ifdef _DEBUG
		printf("%02x %ls\n",level,msg);
#endif
	}

	void formatLogMessage( int level, const char* fmt, va_list args);

private:
	#define LOG_LEN_PER_LINE (0x200)
	char ChangeChar(char input, int index = -1, int lefLen = 0);
	uint8* ChangePtr(uint8* input, int index, int lefLen);
	Mutex* _pTaskMutex;

protected:
	int _verbosity;
};

// -----------------------------
// class DebugMsg
// -----------------------------
/// Debugging support, puts messages in the debugger
class DebugMsg : public Log
{
public:
	/// constructor
	/// @param verbosity  - the verbosity level to compare with
	DebugMsg(const int verbosity=L_ERROR) : Log(verbosity) {};
	virtual ~DebugMsg() {};
	
protected:
	/// implement Log::writeMessage() 
	/// @param msg     - the log msg body to write down
	/// @param level   - log level. the verbosity testing has already been
	///                  performed when this method is called.
	virtual void writeMessage(const char *msg, int level=-1)
	{
#if defined(ZQ_OS_MSWIN)
		::OutputDebugStringA(msg);
		::OutputDebugStringA("\n");
#endif // WIN32
	}
};

// -----------------------------
// class SysLog
// -----------------------------
/// implementataion of syslog().
/// in MS Windows, it puts messages as event log that can be listed in control
/// panel/event viewer
class SysLog : public Log
{
public:
	SysLog();
	virtual ~SysLog();
#ifdef WIN32
	/// constructor
	/// @param applicationName - the application name that shown in the 'source'
	///                          column in eventviewer
	/// @param verbosity  - the verbosity level to compare with
	/// @param machine    - the target event recever, NULL if localhost
	SysLog(const char* applicationName, const int verbosity=L_ERROR, const char* machine=NULL);
	
	/// @param applicationName - the application name that shown in the 'source'
	///                          column in eventviewer
	/// @param verbosity  - the verbosity level to compare with
	/// @param machine    - the target event recever, NULL if localhost
	void open(const char* applicationName, const int verbosity=L_ERROR, const char* machine=NULL);	
	
#else
	SysLog(const char* applicationName, const int option= LOG_ODELAY, const int facility=LOG_USER);
	void open(const char* applicationName,const int level=L_ERROR, const int option= LOG_ODELAY, const int machine=LOG_USER);
	
#endif
protected:
	/// implement Log::writeMessage() 
	/// @param msg     - the log msg body to write down
	/// @param level   - log level. the verbosity testing has already been
	///                  performed when this method is called.
	virtual void writeMessage(const char *msg, int level=-1);
	
private:
#if defined(ZQ_OS_MSWIN)
	HANDLE _hEventSource;
	char _names[MAX_PATH]; // a buffer to keep the values of _machine and _appname
	char *_machine, *_appname;

#else
	char _proname[MAX_PATH];
	int _facility;
#endif // WIN32
};

// -----------------------------
// class LogWrapper
// -----------------------------
/// LogWrapper is a forwarder to true Logger by filtering via its own verbosity level
class LogWrapper : public Log
{
public:
	LogWrapper(Log& to, loglevel_t level) : Log(level), _to(to) {};
	virtual ~LogWrapper() {}

protected:
	/// implement Log::writeMessage() 
	/// @param msg     - the log msg body to write down
	/// @param level   - log level. the verbosity testing has already been
	///                  performed when this method is called.
	virtual void writeMessage(const char *msg, int level=-1)
	{
		level &= 0xff;

		if (level > _verbosity || level > _to._verbosity)
			return;

		_to.writeMessage(msg, level);
	}

	Log& _to;
};


} // namespace common
} // namespace ZQ

#define SCRTRACE
#define SCRTRACE2(_X)
// trace error log
#define GTRACEERR glog(ZQ::common::Log::L_DEBUG, "\t! ERROR SOURCE ! - %s(%d)", __FILE__,__LINE__)

#ifdef _DEBUG
#  ifdef _TRACE_STDOUT_ECHO
#    undef  SCRTRACE
#    define SCRTRACE printf(LOGFMT("\n"))
#    undef  SCRTRACE2(_X)
#    define SCRTRACE2 printf(LOGFMT(_X "\n"))
#  endif
#else
#  undef  _TRACE_STDOUT_ECHO
#endif // _DEBUG


#  ifdef ZQ_OS_MSWIN
#      define __THREADID__  GetCurrentThreadId()
#   else
#      define __THREADID__  ZQ::common::getthreadid()
#   endif//ZQ_OS_MSWIN

#ifndef __FUNCTION__
#	define __FUNCTION__ __FILE__
#endif//__FUNCTION__

#ifdef _DEBUG
#  define LOGFMT_WITH_LINENO
#endif

#ifndef LOGFMT
#  ifdef LOGFMT_WITH_LINENO
#     define LOGFMT(_X)          __FUNCTION__ "  \t[L%03dT%08u] " _X, __LINE__, __THREADID__
#     define CLOGFMT(_C, _X)     #_C          "  \t[L%03dT%08u] " _X, __LINE__, __THREADID__
#     define FLOGFMT(_C, _FUNC, _X)  #_C      "  \t[L%03dT%08u] " #_FUNC "() " _X, __LINE__, __THREADID__
#  else
#     define LOGFMT(_X)      _X
#     define CLOGFMT(_C, _X)         #_C  "  \t[T%08u] " _X, __THREADID__
#     define FLOGFMT(_C, _FUNC, _X)  #_C  "  \t[T%08u] " #_FUNC "() " _X, __THREADID__
#  endif // 
#endif //LOGFMT

#endif // __ZQ_COM_LOG_H__

