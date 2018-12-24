// ============================================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of syscheme Possession, use, duplication or dissemination of the
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
// construed as a commitment by syscheme
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : impl a _timer for polling
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/PollingTimer.cpp 1     10-11-12 15:56 Admin $
// $Log: /ZQProjs/Common/PollingTimer.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 13    08-03-18 17:32 Yixin.tian
// settime for linux
// 
// 12    08-03-06 16:26 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 11    08-03-03 18:00 Yixin.tian
// merged changes for linux
// 
// 10    08-02-27 12:32 Hongquan.zhang
// 
// 8     08-02-27 12:22 Hui.shao
// fix the _GetTickCount64 thread unsafe
// 
// 7     07-08-16 10:40 Fei.huang
// 
// 7     07-08-06 14:11 Fei.huang
// 
// 6     06-09-12 17:16 Bernie.zhao
// now only int64 version for PollingTimer under ZQ_OS_MSWIN
// 
// 5     06-04-24 17:43 Bernie.zhao
// 
// 4     06-04-24 12:05 Bernie.zhao
// added 64 bit version to avoid GetTickCount() rolling over after system
// up 49.7 days
// 
// 3     11/22/05 3:16p Hui.shao
// 
// 2     11/17/05 5:27p Hui.shao
// 
// 2     11/14/05 5:53p Hui.shao
// 
// 1     9/20/05 4:10p Hui.shao
// 
// 2     4/12/05 5:20p Hui.shao
// ============================================================================================

#include "PollingTimer.h"
#include "Locks.h"

namespace ZQ {
namespace common {

/// -----------------------------
/// class PollingTimer
/// -----------------------------

#ifdef ZQ_OS_MSWIN

timeout_t	PollingTimer::_lastTimer = 0;
timeout64_t PollingTimer::_baseTimer64 = 0;

timeout64_t PollingTimer::_GetTickCount64(void)
{
	timeout_t curr = 0;
	curr = GetTickCount();
	if(curr < _lastTimer)
	{
		static Mutex _slocker;
		MutexGuard g(_slocker);
		if (curr < _lastTimer)
		{
			_baseTimer64 += (uint32) (~0);	// GetTickCount() rolls over, after system up 49.7 days
			_lastTimer = curr;
		}
	}

	_lastTimer = curr;

	return _baseTimer64 + curr;
}

PollingTimer::PollingTimer()
{
	_active = false;

	_timer64 = _GetTickCount64();
}

void PollingTimer::setTimer64(timeout64_t timeout64)
{
	_timer64 = _GetTickCount64();
	_active = false;

	/* 
	*	when schedule time is EXACTLY the same with
	*	current time, the timeout64 can be zero and 
	*	should be activated and stared as well.
	*/
//	if(timeout64)
		incTimer64(timeout64);
}

void PollingTimer::incTimer64(timeout64_t timeout64)
{
	_timer64 += timeout64;
	_active = true;
}

void PollingTimer::endTimer64(void)
{
	_active = false;
}

timeout64_t PollingTimer::getTimer64(void)
{
	uint64 now64;
	int64 diff64;
	
	if(!_active)
		return _UI64_MAX;
	
	now64 = _GetTickCount64();
	diff64 = _timer64 - now64;
	
	if(diff64 < 0)
		return 0l;
	
	return diff64;
}

timeout64_t PollingTimer::getElapsed64(void)
{
	return getTimer64();
}


#else

PollingTimer::PollingTimer()
{
	_active = false;
	gettimeofday(&_timer, NULL);
}

void PollingTimer::setTimer(timeout_t timeout)
{
	gettimeofday(&_timer, NULL);
	_active = false;
//	if timeout is zero(schedule time is same as current time)
//  should be activated and stared as well. 
//	if(timeout)
		incTimer(timeout);
}

void PollingTimer::incTimer(timeout_t timeout)
{
	int secs = timeout / 1000;
	int usecs = (timeout % 1000) * 1000;
	
	_timer.tv_usec += usecs;
	if(_timer.tv_usec > 1000000l)
	{
		++_timer.tv_sec;
		_timer.tv_usec %= 1000000l;
	}
	_timer.tv_sec += secs;
	_active = true;
}

void PollingTimer::endTimer(void)
{
	_active = false;
}

timeout_t PollingTimer::getTimer(void)
{
	struct timeval now;
	long diff;
	
	if(!_active)
		return TIMEOUT_INF;
	
	gettimeofday(&now, NULL);
	diff = (_timer.tv_sec - now.tv_sec) * 1000l;
	diff += (_timer.tv_usec - now.tv_usec) / 1000l;
	
	if(diff < 0)
		return 0l;
	
	return diff;
}

timeout_t PollingTimer::getElapsed(void)
{
/*
	struct timeval now;
	long diff;
	
	if(!_active)
		return TIMEOUT_INF;
	
	gettimeofday(&now, NULL);
	diff = (now.tv_sec -_timer.tv_sec) * 1000l;
	diff += (now.tv_usec - _timer.tv_usec) / 1000l;
	if(diff < 0)
		return 0;
	return diff;
*/
	return getTimer();
}

#endif // #ifdef ZQ_OS_MSWIN

} // namespace common
} // namespace ZQ
