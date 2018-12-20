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
// Ident : $Id: ScReporter.cpp,v 1.2 2004/08/05 07:08:37 wli Exp $
// Branch: $Name:  $
// Author: Kaliven Lee
// Desc  : impl a Logger that invokes SeaChange Reporter APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ScReporter.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 25    05-11-18 15:15 Bernie.zhao
// Adjusted the log level number.  Now 500,600,900 will be logged into
// event log as Error, Warning, and Information.  The lowest log number
// now is 0.
// 
// 24    05-10-21 5:11 Jie.zhang
// add flush() and modify writemessage for performance turning
// 
// 23    05-08-29 14:25 Bernie.zhao
// adjusted log level sequence to reflect real relationship
// 
// 22    05-05-13 14:04 Kaliven.lee
// 
// 21    05-05-13 13:24 Bernie.zhao
// move itvmessage.h out
// 
// 20    05-03-15 15:39 Daniel.wang
// 
// 19    05-03-15 15:38 Daniel.wang
// Add support for AnsiString
// 
// 18    3/11/05 11:48a Daniel.wang
// delete exception
// 
// 17    3/11/05 11:45a Daniel.wang
// Add exception into source while cannot reporter
// 
// 16    05-02-28 15:37 Bernie.zhao
// enabled log level
// 
// 14    05-02-22 14:52 Bernie.zhao
// enabled new itvmessages
// 
// 13    05-02-22 14:44 Kaliven.lee
// new message dll
// 
// 12    05-01-14 15:30 Kaliven.lee
// log event to evenlog
// 
// 11    05-01-14 14:43 Kaliven.lee
// 
// 9     04-10-27 16:56 Daniel.wang
// 
// 8     04-08-22 19:14 Kaliven.lee
// 
// 7     04-08-21 13:15 Kaliven.lee
// 
// 6     04-08-20 11:31 Kaliven.lee
// Revision 1.2  2004/08/05 07:08:37  wli
// 
//
// Revision 1.1  2004/07/23 08:31:13  wli
// File Create
//

//
// ===========================================================================

// =====================comment by kaliven lee================================
//	ScReporter encapsulate SeaChange CReporter class for ZQ team
//	this class log the message to appointed log file and reporter the message 
//	to NT event. 
//
//	Note: Your application must be set in register at 
//	"LocalMachine\\system\\CurrentControlSet\\services\\Eventlog\\application"
//	Defaultly your application is a seachange service.
//	And the register is set when service is installed. 
// ===========================================================================

#include "ScReporter.h"

#include "CombString.h"

#include "itvmessages.h"

// because in the itvmessages.h, it define ZQ to a value, cause the namespace ZQ error
#ifdef ZQ
#undef ZQ
#endif


#pragma comment (lib, "reporter" LIBEXTERN )

namespace ZQ {
namespace common {

	///////////////////////////////
	// see the define of REPORTLEVEL
	///////////////////////////////
	int ScReporter::_report_level[MAX_REPORTLEVEL_NUM] = {
														  REPORT_EXCEPTION,
														  REPORT_EXCEPTION,
														  REPORT_CRITICAL,														
														  REPORT_SEVERE,
														  REPORT_WARNING,
														  REPORT_DEBUG,														  
														  REPORT_TRACE,	
														  REPORT_INFORMATIONAL,
														};
// -----------------------------
// class ScReporter
// -----------------------------
ScReporter::ScReporter(wchar_t* servicename,const int verbosity /*=L_ERROR*/)
      :Log(verbosity), _sServiceName(servicename)
{
	_Report_level = verbosity;
//_Reporter.Register(&_pReporterId,servicename,servicename);	
}
ScReporter::ScReporter(char* servicename, const int verbosity /*=L_ERROR*/)
      :Log(verbosity)
{	
	CombString str = servicename;
	_sServiceName = str.c_str();
	_Report_level = verbosity;
	//_Reporter.Register(&_pReporterId,str.c_str(),str.c_str());	
}
ScReporter::~ScReporter()
{		
	
}

void ScReporter::writeMessage(const wchar_t *msg, int level /*=REPORT_TRACE*/)
{	
	// because of the inconsistent between Log::Level and REPORTLEVEL
	// we must dispatch the process according to the REPORTLEVEL 
	if (level >= MAX_REPORTLEVEL_NUM)
		level = L_DEBUG;
	DWORD dwErr = 0;
	DWORD dwID = 0;
	if(level > _Report_level)
		return;
	RPTSTATUS rt = RPT_SUCCESS;
	if(level <6)
	{
		wchar_t * strShow [2];
		strShow[0] = (wchar_t*)msg;
		strShow[1] = NULL;
		switch(level) {
			case  0:
			case  1:
			case  2:
			case  3:
				dwID = ZQ_GENERIC_ERROR;
				break;
			case  4:
				dwID = ZQ_GENERIC_WARNING;
				break;
			case  5:
				dwID = ZQ_GENERIC_INFO;
				break;
			default:
				break;
		}
		rt = _Reporter.Report(_pReporterId,(REPORTLEVEL)_report_level[level],dwID,1,strShow,dwErr);
	}
	else 
		rt = _Reporter.Report(_pReporterId,(REPORTLEVEL)_report_level[level], msg);

	/*
	if (RPT_SUCCESS != rt)
	{
		std::string strExp("Cannot report message- Message:");
		strExp += CombString(msg);
		throw Exception(strExp);
	}
	*/
	
}
void ScReporter::writeMessage(const char* msg,int level /*= REPORT_TRACE*/)
{
#if 1
	wchar_t  wszMsg[4096*2];
	MultiByteToWideChar(CP_ACP, NULL, msg, -1, wszMsg, sizeof(wszMsg)/sizeof(wchar_t));
	writeMessage(wszMsg, level);
#else
	int len = strlen(msg);
	wchar_t* wmsg = new wchar_t[len+1];
	memset(wmsg, 0, (len+1)*sizeof(wchar_t));
	int nconv = mbstowcs(wmsg, msg, len);
	if (nconv == 0)
		return;
	
	writeMessage(wmsg, level);

	delete[] wmsg;
#endif
}

bool ScReporter::initialize(wchar_t* wsLogFileName,DWORD dwFileSize,DWORD dwBufSize,DWORD dwTimeOut)
{
	_Reporter.SetSvcLogMaxFileSize(dwFileSize);
	_Reporter.SetSvcLogWriteTimeout(dwTimeOut);
	_Reporter.SetSvcLogBufferSize(dwBufSize);

	if (RPT_SUCCESS != _Reporter.Init(wsLogFileName))
	{
		return false;
	}

	const wchar_t* pt = _sServiceName.c_str();
	RPTSTATUS rt = _Reporter.Register(&_pReporterId, pt,_sServiceName.c_str());

	if (RPT_SUCCESS != rt)
	{
		return false;
	}
	return true;
}
bool ScReporter::unInitialize(void)
{
	if (RPT_SUCCESS != _Reporter.Flush())
	{
		return false;
	}	
	if (RPT_SUCCESS != _Reporter.Uninit())
	{
		return false;
	}
	//Sleep(SCREPORTER_DEFAULT_FLUSH_TIMEOUT);
	return true;
}

void ScReporter::flush()
{
	 _Reporter.Flush();
}

} // namespace common
} // namespace ZQ

