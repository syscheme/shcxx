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
// Author: Ken Qian
// Desc  : define a time conversion utility
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Header: /ZQProjs/Common/TimeUtil.h 4     1/30/13 11:59a Hui.shao $
// $Log: /ZQProjs/Common/TimeUtil.h $
// 
// 4     1/30/13 11:59a Hui.shao
// 
// 3     1/09/13 4:59p Hui.shao
// 
// 2     6/02/11 4:53p Hui.shao
// added parseTimeLength() but postpone to export from dll in the later
// builds
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 18    10-10-20 14:44 Hui.shao
// 
// 17    09-11-16 16:32 Junming.zheng
// add static decor for some methods
// 
// 16    09-11-16 16:12 Xiaohui.chai
// add the conversion function between 64-bits ingeger stamp and UTC
// string
// 
// 15    09-11-05 18:27 Xiaohui.chai
// add timestamp function: now()
// 
// 14    09-08-31 11:28 Yixin.tian
// add Iso2TimeEx function
// 
// 13    09-07-31 19:51 Fei.huang
// * add default params for timeToUTC
// 
// 12    09-07-31 12:08 Fei.huang
// + local timezone option in Time2Iso
// 
// 11    09-06-22 11:28 Xiaoming.li
// add function to parse time with unit(ex. 1h, 1m, 1s, xx:xx:xx.xxx)
// 
// 10    08-12-15 15:10 Yixin.tian
// add struct timeval format time to ISO
// 
// 9     08-04-30 18:18 Fei.huang
// 
// 8     08-03-06 16:34 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 7     08-02-25 18:12 Yixin.tian
// 
// 6     08-02-21 13:42 Yixin.tian
// 
// 5     08-02-20 18:36 Yixin.tian
// 
// 4     07-04-05 16:25 Fei.huang
// 
// 3     07-03-28 15:45 Ken.qian
// 
// 2     06-09-25 16:01 Ken.qian



#ifndef __ZQ_Common_TimeConv_h__
#define __ZQ_Common_TimeConv_h__

#include "ZQ_common_conf.h"
#include <time.h>
#  include <stdio.h>

#ifndef _WIN32
#include <sys/time.h>
#include <string>
#endif

namespace ZQ {
namespace common {

/// get the timestamp in unit msec
int64 ZQ_COMMON_API now();

class ZQ_COMMON_API TimeUtil;

class TimeUtil
{
public:
	// 
	// routine for SYSTEMTIME <-> time_t convertion
	//
#ifdef ZQ_OS_MSWIN
	/// convert SYSTEMTIME format to time_t format
	static time_t Systime2Time(SYSTEMTIME& st_utc);
	
	/// convert SYSTEMTIME format to time_t format
	static bool Time2SystemTime(time_t t, SYSTEMTIME& st_utc);
	
	// 
	// routine for UTC string Time <-> time_t/SYSTEMTIME convertion
	//

	/// convert UTC string format to time_t format
	static bool Iso2Time(const char* UTC_datetime, time_t& t);

	static bool Iso2TimeEx(const char* UTC_datetime, time_t& t);
	
	/// convert UTC string format to SYSTEMTIME format
	static bool Iso2Time(const char* UTC_datetime, SYSTEMTIME& st_utc);

	/// convert UTC string format to SYSTEMTIME format
	static bool Iso2TimeEx(const char* UTC_datetime, SYSTEMTIME& st_utc);

	/// convert timet_t format time to UTC string format
	static bool Time2Iso(time_t t, char* UTC_datetime, int length);
	
	/// convert SYSTEMTIME format time to UTC string format
	static bool Time2Iso(SYSTEMTIME& st_utc, char* UTC_datetime, int length, bool localZone=false);

//	/// convert SYSTEMTIME format time to UTC string format with msec
//	static bool Time2IsoEx(SYSTEMTIME& st_utc, char* UTC_datetime, int length);

#else//non-win32
	/// convert struct tm time(UTC) format to time_t format
	static time_t Tmtime2Time(struct tm& st_utc);
	
	/// convert time_t format to struct tm time(UTC) format
	static bool Time2Tmtime(time_t t, struct tm& st_utc);
	
	// 
	// routine for UTC string Time <-> time_t/SYSTEMTIME convertion
	//

	/// convert UTC string format to time_t format
	static bool Iso2Time(const char* UTC_datetime, time_t& t);

	static bool Iso2TimeEx(const char* UTC_datetime, time_t& t);
	
	/// convert UTC string format to struct tm time format
	static bool Iso2Time(const char* UTC_datetime, struct tm& st_utc);

	/// convert UTC string format to struct timeval format
	static bool Iso2Time(const char* UTC_dateTime, struct timeval& tmval);

	/// convert UTC string format to struct timeval format
	static bool Iso2TimeEx(const char* UTC_datetime, struct timeval& tmval);

	/// convert timet_t format time to UTC string format
	static bool Time2Iso(time_t t, char* UTC_datetime, int length);
	
	/// convert struct tm time format time to UTC string format
	static bool Time2Iso(struct tm& st_utc, char* UTC_datetime, int length);

	/// convert struct timeval format time to UTC string format
	static bool Time2Iso(struct timeval& tmval, char* UTC_datetime, int length, bool local=false);
#endif

	// 
	// routine for UTC string Time <-> Local string Time convertion
	//

	/// convert local time to UTC time 
	static bool Local2Iso(const char* LOCAL_datetime, char* UTC_datetime, int length);

	/// convert UTC time to local time 
	static bool Iso2Local(const char* UTC_datetime, char* LOCAL_datetime, int length);

	//
	//	routine for ISO string time <-> timestamp representation (time_t)
	//
	
	/// convert ISO string to time_t
	static bool Str2Time(const char* datetime, time_t& timestamp);

	/// convert time_t to ISO string
	static bool Time2Str(time_t timestamp, char* datetime, int len);

	/// convert time string with units(ex.10s,10m,1h) to ms
	static bool UnitStr2Time(const char* unitTime, int& msTime);

	/// convert time string(ex.1:1:1.345) to ms
	static bool HMSStr2Time(const char* unitTime, int& msTime);

    // conversion function between UTC string to 64-bits integer stamp
    /// return the UTC formatted time string
    static const char* TimeToUTC(int64 time, char* buf, const int maxlen , bool bLocalZone = false );

    /// convert UTC string format to stamp
    /// return 0 on failure
    static int64 ISO8601ToTime(const char* ISO8601Str);

	static int64 now();

	static const char* msec2hms(int64 msec, char* str, int maxlen);
	static const char* msec2secstr(int64 msec, char* str, int maxlen);

#ifdef ZQ_OS_MSWIN
	/// convert UTC string format to time64_t format
	static bool Iso2Time64(const char* UTC_datetime, __time64_t& t);
#endif // ZQ_OS_MSWIN
	// accpeted units: D(d)ay, H(h)our, M(m)in, S(s)ec
	// allowed time length formats: "3day 1hour 5min", "3d1h5m40s111", "3d1h5m40s111"
	// return the time len in milliseconds
	//TODO enable later: static int64 parseTimeLength(const char* timeLenStr);
};


} // namespace common
} // namespace ZQ

#endif // __ZQ_Common_TimeConv_h__
