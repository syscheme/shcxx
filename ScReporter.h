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
// Ident : $Id: ScReporter.h,v 1.2 2004/08/05 07:08:37 wli Exp $
// Branch: $Name:  $
// Author: Kaliven Lee
// Desc  : Define the Logger invokes SeaChange Reporter APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ScReporter.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 15    07-04-27 15:16 Jie.zhang
// 
// 12    05-02-28 15:37 Bernie.zhao
// enabled log level
// 
// 11    05-02-22 14:44 Kaliven.lee
// new message dll
// 
// 10    04-11-05 11:59 Kaliven.lee
// 
// 9     04-10-27 16:56 Daniel.wang
// 
// 8     04-09-03 11:52 Kaliven.lee
// 
// 7     04-08-23 11:17 Jian.shen
// 
// 6     04-08-22 19:14 Kaliven.lee
// 
// 5     04-08-20 11:31 Kaliven.lee
// Revision 1.2  2004/08/05 07:08:37  wli
// create
//
// Revision 1.1  2004/07/23 08:31:13  wli
// File Create
//

// ===========================================================================


#ifndef	__ZQ_COM_ScReporter_H__
#define	__ZQ_COM_ScReporter_H__
#ifndef WINVER
#define WINVER 0x0501
#endif

#include "Reporter.h"
#include "Log.h"


#include <string>
using std::wstring;
namespace ZQ {
namespace common {
#ifndef LIBEXTERN
#ifdef _DEBUG
#define LIBEXTERN  "_d.lib"
#else
#define LIBEXTERN	".lib"
#endif

#endif

class ZQ_COMMON_API ScReporter;

using ZQ::common::Log;

#define SCREPORTER_DEFAULT_FILESIZE (1024*1024)
#define SCREPORTER_DEFAULT_BUFSIZE (8*1024)
#define SCREPORTER_DEFAULT_FLUSH_TIMEOUT (2*1000)
#define SCREPORTER_EXEC_LOG_LEVEL ZQ::common::Log::L_DEBUG
#define SCREPORTER_MIN_FILESIZE (64*1024)

#define	MAX_REPORTLEVEL_NUM					8
#define MAX_LOG_LEVEL                       1000

// -----------------------------
// class ScReporter
// -----------------------------
/// Log class that invokes SeaChange CReporter APIs
class ScReporter : public Log
{
public:
	
	/// constructor	
	/// @param sourcename - source name of the Nt log event
	/// @param verbosity  - the verbosity level to compare with
	ScReporter( wchar_t* servicename, const int verbosity=L_DEBUG );
	ScReporter(char* servicename, const int verbosity=L_DEBUG);
	/// destructor
	~ScReporter();
	
	// implemented ReportMessage() by calling Reporter APIs
	virtual void writeMessage(const wchar_t *msg,  int level = L_DEBUG);
	virtual void writeMessage(const char* msg,int level = L_DEBUG);

	/// setReportLevel 
	/// @param logType	-	log type ,    NT_EVENT_LOG ,	SERVICE_LOG,	ALL_LOGS,
	/// @param reportLevel	- report level 
	//////////////////////////////////////////////////////////////////////////
	/// you can call the function to set report level in your service
	/// or defaultly logType is ALL_LOGS and reportLevel is REPORT_TRACE
	//////////////////////////////////////////////////////////////////////////	
	void setReportLevel(LOGTYPE logType = ALL_LOGS, int reportLevel = L_ERROR){_Reporter.SetLogLevel(logType, MAX_LOG_LEVEL);_Report_level = reportLevel;}
	bool initialize(wchar_t* wsLogFileName,DWORD dwFileSize = SCREPORTER_DEFAULT_FILESIZE,DWORD dwBufSize = SCREPORTER_DEFAULT_BUFSIZE,DWORD dwTimeOut = SCREPORTER_DEFAULT_FLUSH_TIMEOUT);

	bool unInitialize(void);

	/// flush data 
	virtual void flush();


	public :
		static int _report_level[MAX_REPORTLEVEL_NUM];
protected:
	
	//void setSvcAttr(LPCTSTR lpszFileName,DWORD dwMaxFileSize,DWORD dwWriteTimeout,DWORD dwBufferSize);
	CReporter _Reporter;
	DWORD _pReporterId;
	std::wstring _sServiceName;
	DWORD _Report_level;
};

} // namespace common
} // namespace ZQ

#endif __ZQ_COM_ScReporter_H__

