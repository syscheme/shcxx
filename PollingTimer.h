// ============================================================================================
// Copyright (c) 1997, 1998 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of ZQ Interactive, Inc. Possession, use, duplication or dissemination of the
// software and media is authorized only pursuant to a valid written license from ZQ Interactive,
// Inc.
// This source was copied from shcxx, shcxx's copyright is belong to Hui Shao
//
// This software is furnished under a  license  and  may  be used and copied only in accordance
// with the terms of  such license and with the inclusion of the above copyright notice.  This
// software or any other copies thereof may not be provided or otherwise made available to any
// other person.  No title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and should not be
// construed as a commitment by ZQ Interactive, Inc.
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : define a _timer for polling
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/PollingTimer.h 1     10-11-12 15:56 Admin $
// $Log: /ZQProjs/Common/PollingTimer.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     08-03-06 16:26 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-03-03 18:00 Yixin.tian
// merged changes for linux
// 
// 4     4/13/05 6:33p Hui.shao
// changed namespace
// 
// 3     4/12/05 5:20p Hui.shao
// ============================================================================================

#ifndef __PollingTimer_h__
#define __PollingTimer_h__

#include "ZQ_common_conf.h"

extern "C" {
#ifndef ZQ_OS_MSWIN
#  include <sys/time.h>
#  include <time.h>
#endif
}
//#ifndef timeout_t
//typedef ZQ::common::uint32 timeout_t;
//#endif

#ifndef _UI64_MAX
#	define _UI64_MAX     0xffffffffffffffffui64
#endif

namespace ZQ {
namespace common {

class ZQ_COMMON_API PollingTimer;

//in msec
class PollingTimer
{
public:
	
	PollingTimer();

#ifdef ZQ_OS_MSWIN	// ZQ_OS_MSWIN specific 64bit long version
	void setTimer64(timeout64_t timeout64 = 0);
	
	void incTimer64(timeout64_t timeout64);
	
	void endTimer64(void);
	
	timeout64_t getTimer64(void);
	
	timeout64_t getElapsed64(void);
	
	static timeout64_t _GetTickCount64(void);

#else
	void setTimer(timeout_t timeout = 0);
	
	void incTimer(timeout_t timeout);
	
	void endTimer(void);
	
	timeout_t getTimer(void);

	timeout_t getElapsed(void);

#endif

private:
	
#ifdef ZQ_OS_MSWIN
	timeout64_t	_timer64;
	static	timeout64_t	_baseTimer64;
	static	timeout_t	_lastTimer;
#else
	struct timeval _timer;
//	timeout_t _timer;
#endif
	
	bool _active;
	
};

} // namespace common
} // namespace ZQ

#endif // ! __PollingTimer_h__

