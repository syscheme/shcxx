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
// $Header: /ZQProjs/Common/TimeUtil.cpp 10    3/11/16 9:52a Dejian.fei $
// $Log: /ZQProjs/Common/TimeUtil.cpp $
// 
// 10    3/11/16 9:52a Dejian.fei
// NDK android
// 
// 9     1/27/15 9:54a Zhiqiang.niu
// unfreeze  year must less than 2100
// 
// 8     6/20/13 9:46a Hongquan.zhang
// fix compiling error in CentOS6.3
// 
// 7     4/10/13 5:18p Hui.shao
// 
// 6     1/30/13 10:33a Hui.shao
// 
// 5     1/09/13 4:59p Hui.shao
// 
// 4     6/02/11 4:53p Hui.shao
// added parseTimeLength() but postpone to export from dll in the later
// builds
// 
// 3     3/30/11 6:20p Xiaohui.chai
// init the struct tm
// 
// 2     10-12-22 17:04 Fei.huang
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 33    10-10-20 14:44 Hui.shao
// 
// 32    10-08-31 15:45 Xiaohui.chai
// 
// 31    10-08-27 17:52 Fei.huang
// * fix convert time routine to thread safe version 
// 
// 30    10-02-25 17:12 Hui.shao
// 
// 29    10-01-29 15:23 Yixin.tian
// use timegm() to convert struct tm to time_t
// 
// 28    09-12-03 14:07 Yixin.tian
// modify the sscanf warning
// 
// 27    09-11-25 10:34 Fei.huang
// 
// 26    09-11-16 16:32 Junming.zheng
// add static decor for some methods
// 
// 25    09-11-16 16:12 Xiaohui.chai
// add the conversion function between 64-bits ingeger stamp and UTC
// string
// 
// 24    09-11-05 18:27 Xiaohui.chai
// add timestamp function: now()
// 
// 23    09-09-09 17:14 Yixin.tian
// 
// 22    09-09-09 16:04 Yixin.tian
// 
// 21    09-08-31 11:28 Yixin.tian
// add Iso2TimeEx function
// 
// 20    09-08-12 13:18 Xiaoming.li
// ommit ms when ms==0
// 
// 19    09-07-31 12:07 Fei.huang
// + implement local time zone option in Time2Iso
// 
// 18    09-07-13 10:10 Xiaoming.li
// 
// 17    09-06-23 15:54 Fei.huang
// 
// 16    09-06-22 11:28 Xiaoming.li
// add function to parse time with unit(ex. 1h, 1m, 1s, xx:xx:xx.xxx)
// 
// 15    08-12-15 15:10 Yixin.tian
// add struct timeval format time to ISO
// 
// 14    08-04-30 18:18 Fei.huang
// 
// 13    08-03-06 16:34 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 12    08-03-06 14:22 Xiaohui.chai
// corrected the time string buffer size check
// 
// 11    08-03-03 18:15 Yixin.tian
// 
// 10    08-02-25 18:12 Yixin.tian
// 
// 9     08-02-25 16:02 Yixin.tian
// iso time format add 'z' char
// 
// 8     08-02-20 18:36 Yixin.tian
// 
// 7     07-12-25 14:12 Fei.huang
// 
// 6     07-12-21 17:50 Fei.huang
// 
// 5     07-04-05 16:25 Fei.huang
// 
// 4     07-03-28 15:45 Ken.qian
// 
// 3     06-09-25 16:44 Ken.qian
// 
// 2     06-09-25 16:01 Ken.qian


#include "TimeUtil.h"
#include "strHelper.h"

// UTC time string format
#define UTC_LEN_LOCAL           19  // format: "%4d-%02d-%02dT%02d:%02d:%02d"
#define UTC_LEN_STD             20  // format: "%4d-%02d-%02dT%02d:%02d:%02dZ"
#define UTC_LEN_WITH_MSEC_LOCAL 23  // format: "%4d-%02d-%02dT%02d:%02d:%02d.%03d"
#define UTC_LEN_WITH_MSEC_STD   24  // format: "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ"
#define UTC_LEN_WITH_TIMEZONE   29  // format: "%4d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d"


namespace ZQ {
namespace common {

/*
*	notes: global variable _daylight shows if daylight saving zone is specified in TZ 
*		   environment variable or from OS, defaults to 1, tzset() must be used to affect
*		   or update the corresponding global values.
*			
*		   isdst in tm structure shows if currently the daylight saving time is in effect
*		   or not, however, this value can't be determined at the time of coding, better
*		   specify a negative value to have the system compute it accordingly.
*
*		   note that whether we have a daylight saving zone or the daylight saving is in 
*		   effect are different. 
*/

#ifdef ZQ_OS_MSWIN
time_t TimeUtil::Systime2Time(SYSTEMTIME& st_utc)
{
	SYSTEMTIME st_local;

	FILETIME  ft_utc, ft_local;
	SystemTimeToFileTime(
		&st_utc,  // system time
	    &ft_utc   // file time
	);

	FileTimeToLocalFileTime(&ft_utc, &ft_local);
	FileTimeToSystemTime(&ft_local, &st_local);

	struct tm atm;
    memset(&atm, 0, sizeof(struct tm));
	atm.tm_sec = st_local.wSecond;
	atm.tm_min = st_local.wMinute;
	atm.tm_hour = st_local.wHour;		
	atm.tm_mday = st_local.wDay;
	atm.tm_mon = st_local.wMonth - 1;        // tm_mon is 0 based		
	atm.tm_year = st_local.wYear - 1900;     // tm_year is 1900 based
//	atm.tm_isdst = _daylight;				 // set if day light saving
	atm.tm_isdst = (-1);
	
	time_t t = mktime(&atm);

	return t;
}

bool TimeUtil::Time2SystemTime(time_t t, SYSTEMTIME& st_utc)
{
    struct tm* ptm = gmtime(&t);
	if(NULL == ptm)  // t is not a valid value
	{
		return false;
	}

    struct tm& atm = *ptm;
	st_utc.wYear = atm.tm_year + 1900;  // tm_year is 1900 based
	st_utc.wMonth = atm.tm_mon + 1;     // tm_mon is 0 based	
	st_utc.wDay = atm.tm_mday;
	st_utc.wHour = atm.tm_hour;
	st_utc.wMinute = atm.tm_min;
	st_utc.wSecond = atm.tm_sec;
	st_utc.wMilliseconds = 0;
//	_daylight = atm.tm_isdst;  // not necessary

	return true;
}


bool TimeUtil::Iso2Time(const char* UTC_datetime, SYSTEMTIME& st_utc)
{
	int nYear,nMon,nDay, nHour, nMin, nSec;
	if (::sscanf(UTC_datetime, "%d-%d-%dT%d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		&& ::sscanf(UTC_datetime, "%4d%2d%2dT%d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6

		&& ::sscanf(UTC_datetime, "%d-%d-%d %d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		&& ::sscanf(UTC_datetime, "%4d%2d%2d %d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		)
	{
		return false;
	}
	
	if (nYear < 1970 || nYear > 2100 || nMon < 1 || nMon > 12 || nDay < 1 || nDay > 31 || 
		nHour < 0 || nHour > 23 || nMin < 0 || nMin > 59 || nSec < 0 || nSec > 59)
		return false;


	memset(&st_utc, 0, sizeof(st_utc));
	st_utc.wYear = nYear;
	st_utc.wMonth = nMon;
	st_utc.wDay = nDay;
	st_utc.wHour = nHour;
	st_utc.wMinute = nMin;
	st_utc.wSecond = nSec;

	return true;
}

bool TimeUtil::Iso2TimeEx(const char* UTC_datetime, SYSTEMTIME& st_utc)
{
	char utcTime[128];
	strcpy(utcTime, UTC_datetime);

	char* pTime = strchr(utcTime, 'T');
	char* pZone = NULL;
	char zonePrefix = '\0';
	
	if (pTime)
		*pTime++ = '\0';
	else return false;
	
	if (NULL == (pZone = strchr(pTime, 'Z')))
	{
		if (NULL ==(pZone = strchr(pTime, '+')))
			pZone = strchr(pTime, '-');
	}
		
	if (pZone)
	{
		zonePrefix = *pZone;
		*pZone++ = '\0';
	}

	char buf[8][16];
	memset(buf, 0, sizeof(buf));

	int zoneSecOffset=0;
	st_utc.wSecond = st_utc.wMilliseconds =0;
	
	// step 1 scan date part
	int c =0;
	if (::sscanf(UTC_datetime, "%4[0-9]%[/-]%2[0-9]%[/-]%2[0-9]", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]) == 5)
	{
		st_utc.wYear  = atoi(buf[0]);
		st_utc.wMonth = atoi(buf[2]);
		st_utc.wDay   = atoi(buf[4]);
	}
	else if (::sscanf(UTC_datetime, "%4[0-9]%2[0-9]%2[0-9]", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]) == 3)
	{
		st_utc.wYear  = atoi(buf[0]);
		st_utc.wMonth = atoi(buf[1]);
		st_utc.wDay   = atoi(buf[2]);
	}
	else 
		return false;

	c = ::sscanf(pTime, "%2[0-9]:%2[0-9]:%2[0-9].%[0-9]", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	if (c < 2)
		c = ::sscanf(pTime, "%2[0-9]%2[0-9]%2[0-9].%[0-9]", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	if (c < 2)
		return false;

	if (c >= 2)
	{
		st_utc.wHour = atoi(buf[0]);
		st_utc.wMinute = atoi(buf[1]);
		if (c >2)
			st_utc.wSecond = atoi(buf[2]);
		if (c >3)
		{
			int power=1; buf[3][3]='\0';
			for (int k = 3 - strlen(buf[3]); k >0; k--)
				power *=10;

			st_utc.wMilliseconds = atoi(buf[3]) * power;
		}
	}

	if ('\0' == zonePrefix || 'Z' == zonePrefix)
		return true;
	
	// apply the zone offset onto the result
	c = ::sscanf(pZone, "%[^:]:%2[0-9]", buf[0], buf[1]);
	if (c>0)
	{
		zoneSecOffset = atoi(buf[0]) *60;
		if (c>1)
			zoneSecOffset += atoi(buf[1]);
		zoneSecOffset*= 60;
		if ('-'  == zonePrefix)
			zoneSecOffset = -zoneSecOffset;
	}

	FILETIME fileTime ={0};
	if (!::SystemTimeToFileTime(&st_utc, &fileTime))
		return false;

	__int64* ftime = (__int64*) &fileTime;
	(*ftime) -= (__int64) zoneSecOffset * 10000000;

	return ::FileTimeToSystemTime(&fileTime, &st_utc);

}

bool TimeUtil::Iso2Time(const char* UTC_datetime, time_t& t)
{
	t = 0;

	SYSTEMTIME st_utc, st_local;

	if (!Iso2Time(UTC_datetime, st_utc))
		return false;

	FILETIME  ft_utc, ft_local;
	SystemTimeToFileTime(
		&st_utc,  // system time
	    &ft_utc   // file time
	);

	FileTimeToLocalFileTime(&ft_utc, &ft_local);
	FileTimeToSystemTime(&ft_local, &st_local);

	struct tm atm;
    memset(&atm, 0, sizeof(struct tm));
	atm.tm_sec = st_local.wSecond;
	atm.tm_min = st_local.wMinute;
	atm.tm_hour = st_local.wHour;		
	atm.tm_mday = st_local.wDay;
	atm.tm_mon = st_local.wMonth - 1;        // tm_mon is 0 based		
	atm.tm_year = st_local.wYear - 1900;     // tm_year is 1900 based
//	atm.tm_isdst = _daylight;				 // set if day light saving
	atm.tm_isdst = (-1);
	
	t = mktime(&atm);

	return true;
}

static bool Iso2Tm(const char* UTC_datetime, struct tm& tmstruct)
{
	SYSTEMTIME st_utc, st_local;

	if (!TimeUtil::Iso2TimeEx(UTC_datetime, st_utc))
		return false;

	FILETIME  ft_utc, ft_local;
	SystemTimeToFileTime(
		&st_utc,  // system time
		&ft_utc   // file time 
		);

	FileTimeToLocalFileTime(&ft_utc, &ft_local);
	FileTimeToSystemTime(&ft_local, &st_local);

    memset(&tmstruct, 0, sizeof(tmstruct));
	tmstruct.tm_sec   = st_local.wSecond;
	tmstruct.tm_min   = st_local.wMinute;
	tmstruct.tm_hour  = st_local.wHour;		
	tmstruct.tm_mday  = st_local.wDay;
	tmstruct.tm_mon   = st_local.wMonth - 1;        // tm_mon is 0 based		
	tmstruct.tm_year  = st_local.wYear - 1900;     // tm_year is 1900 based
	//	atm.tm_isdst  = _daylight;				 // set if day light saving
	tmstruct.tm_isdst = (-1);

	return true;
}


bool TimeUtil::Iso2TimeEx(const char* UTC_datetime, time_t& t)
{
	t = 0;
	struct tm atm;
	if (!Iso2Tm(UTC_datetime, atm))
		return false;

	t = mktime(&atm);
	return true;
}

/// convert UTC string format to time64_t format
bool TimeUtil::Iso2Time64(const char* UTC_datetime, __time64_t& t)
{
	t = 0;
	struct tm atm;
	if (!Iso2Tm(UTC_datetime, atm))
		return false;

	t = _mktime64(&atm);
	return true;
}

bool TimeUtil::Time2Iso(time_t t, char* UTC_datetime, int length)
{
	if(NULL == UTC_datetime || length <= UTC_LEN_STD)
	{
		return false;
	}

	// turn time_t to tm struct
	struct tm* ptm = gmtime(&t);
    if(NULL == ptm) {
        return false;
    }
    struct tm& atm = *ptm;

	snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02dZ", 
			              atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
						  atm.tm_hour, atm.tm_min, atm.tm_sec);	

	return true;
}

bool TimeUtil::Time2Iso(SYSTEMTIME& st_utc, char* UTC_datetime, int length, bool localZone)
{
	if(NULL == UTC_datetime || length <= UTC_LEN_WITH_MSEC_STD || (localZone && length <= UTC_LEN_WITH_TIMEZONE))
	{
		return false;
	}

	if (localZone)
	{
		TIME_ZONE_INFORMATION zoneinfo;
		SYSTEMTIME locSys;
		
		if(GetTimeZoneInformation(&zoneinfo) == TIME_ZONE_ID_INVALID)
			return false;

		if(SystemTimeToTzSpecificLocalTime(&zoneinfo,&st_utc,&locSys) == 0)
			return false;

        char symb = zoneinfo.Bias > 0 ? '-' : '+';
        int nZHour = abs(zoneinfo.Bias)/60;
        int nZMin = abs(zoneinfo.Bias)%60;

		if (locSys.wMilliseconds != 0)
			snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d", 
					locSys.wYear, locSys.wMonth, locSys.wDay,
					locSys.wHour, locSys.wMinute, locSys.wSecond, 
					locSys.wMilliseconds, symb, nZHour, nZMin);
		else
			snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", 
					locSys.wYear, locSys.wMonth, locSys.wDay,
					locSys.wHour, locSys.wMinute, locSys.wSecond,
					symb, nZHour, nZMin);

        /*
		if(zoneinfo.Bias > 0)
		{
			nZHour = zoneinfo.Bias/60;
			nZMin = zoneinfo.Bias%60;
			snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d.%03d-%02d:%02d", 
			              locSys.wYear, locSys.wMonth, locSys.wDay,
						  locSys.wHour, locSys.wMinute, locSys.wSecond, 
						  locSys.wMilliseconds,nZHour,nZMin);

		}
		else
		{
			nZHour = (-zoneinfo.Bias)/60;
			nZMin = (-zoneinfo.Bias)%60;

			snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d.%03d+%02d:%02d", 
			              locSys.wYear, locSys.wMonth, locSys.wDay,
						  locSys.wHour, locSys.wMinute, locSys.wSecond, 
						  locSys.wMilliseconds,nZHour,nZMin);
		}
        */
		return true;
	}
	
	if (st_utc.wMilliseconds != 0)
		snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ", 
	              st_utc.wYear, st_utc.wMonth, st_utc.wDay,
				  st_utc.wHour, st_utc.wMinute, st_utc.wSecond, 
				  st_utc.wMilliseconds);
	else
		snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02dZ", 
					st_utc.wYear, st_utc.wMonth, st_utc.wDay,
					st_utc.wHour, st_utc.wMinute, st_utc.wSecond);

	return true;	
}


bool TimeUtil::Local2Iso(const char* LOCAL_datetime, char* UTC_datetime, int length)
{
	if(NULL == LOCAL_datetime || NULL == UTC_datetime || length <= UTC_LEN_WITH_MSEC_STD)
	{
		return false;
	}

	SYSTEMTIME st_local, st_utc;

	if (!Iso2Time(LOCAL_datetime, st_local))
		return false;

	FILETIME  ft_local, ft_utc;
	SystemTimeToFileTime(
		&st_local,  // system time
	    &ft_local   // file time
	);

	LocalFileTimeToFileTime(&ft_local, &ft_utc);

	FileTimeToSystemTime(&ft_utc, &st_utc);

	if (st_utc.wMilliseconds != 0)
		snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ", 
	            st_utc.wYear, st_utc.wMonth, st_utc.wDay,
				st_utc.wHour, st_utc.wMinute, st_utc.wSecond, 
				st_utc.wMilliseconds);
	else
		snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02dZ", 
				st_utc.wYear, st_utc.wMonth, st_utc.wDay,
				st_utc.wHour, st_utc.wMinute, st_utc.wSecond);

	return true;
}

bool TimeUtil::Iso2Local(const char* UTC_datetime, char* LOCAL_datetime, int length)
{
	if(NULL == UTC_datetime || NULL == LOCAL_datetime || length <= UTC_LEN_WITH_MSEC_LOCAL)
	{
		return false;
	}

	SYSTEMTIME st_local, st_utc;

	if (!Iso2Time(UTC_datetime, st_utc))
		return false;

	FILETIME  ft_local, ft_utc;
	SystemTimeToFileTime(
		&st_utc,  // system time
	    &ft_utc   // file time
	);

	FileTimeToLocalFileTime(&ft_utc, &ft_local);

	FileTimeToSystemTime(&ft_local, &st_local);

	snprintf(LOCAL_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02d.%03d", 
			              st_local.wYear, st_local.wMonth, st_local.wDay,
						  st_local.wHour, st_local.wMinute, st_local.wSecond,
						  st_local.wMilliseconds);


	return true;
}

#else
#ifdef ZQ_COMMON_ANDROID
time_t my_timegm(struct tm *tm)
{
    time_t ret;
    char *tz;

   tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    return ret;
}
#endif
time_t TimeUtil::Tmtime2Time(struct tm& st_utc)
{
#ifndef ZQ_COMMON_ANDROID
	return timegm(&st_utc);
#else
	return my_timegm(&st_utc);
#endif
/*
	struct timeval val;
	struct timezone zon;
	gettimeofday(&val,&zon);
	
	time_t t = mktime(&st_utc);
	//add zone
	t -= zon.tz_minuteswest * 60; 
	return t;
*/
}

bool TimeUtil::Time2Tmtime(time_t t, struct tm& st_utc)
{
	if(gmtime_r(&t, &st_utc) == NULL) {
		return false;
	}
	return true;
}


bool TimeUtil::Iso2Time(const char* UTC_datetime, struct tm& st_utc)
{
	int nYear,nMon,nDay, nHour, nMin, nSec;
	if (::sscanf(UTC_datetime, "%d-%d-%dT%d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		&& ::sscanf(UTC_datetime, "%4d%2d%2dT%d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6

		&& ::sscanf(UTC_datetime, "%d-%d-%d %d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		&& ::sscanf(UTC_datetime, "%4d%2d%2d %d:%d:%d", &nYear, &nMon, &nDay, &nHour, &nMin, &nSec) <6
		)
	{
		return false;
	}
	
	if (nYear < 1970 || nMon < 1 || nMon > 12 || nDay < 1 || nDay > 31 || 
		nHour < 0 || nHour > 23 || nMin < 0 || nMin > 59 || nSec < 0 || nSec > 59)
		return false;
	memset(&st_utc, 0, sizeof(st_utc));
	
	st_utc.tm_year = nYear - 1900;
	st_utc.tm_mon = nMon - 1;
	st_utc.tm_mday = nDay;
	st_utc.tm_hour = nHour;
	st_utc.tm_min = nMin;
	st_utc.tm_sec = nSec;
	
	return true;
}

bool TimeUtil::Iso2Time(const char* UTC_datetime, time_t& t)
{
	t = 0;
	struct tm  tm_utc;
	if (!Iso2Time(UTC_datetime, tm_utc))
		return false;

	t = Tmtime2Time(tm_utc);
	if(t == (time_t)-1)
		return false;

	return true;
}

bool TimeUtil::Iso2TimeEx(const char* UTC_datetime, time_t& t)
{
	struct timeval tmval;
	if(! Iso2TimeEx(UTC_datetime, tmval))
		return false;

	t = tmval.tv_sec;
	
	return true;
}

bool TimeUtil::Iso2Time(const char* UTC_dateTime, struct timeval& tmval)
{
	time_t tm;
	if(!Iso2Time(UTC_dateTime, tm))
		return false;
	
	tmval.tv_sec = tm;

	const char* pFind = strchr(UTC_dateTime,'.');
	if(!pFind)
		tmval.tv_usec = 0;
	else
	{
		int nMSec = atoi(pFind+1);
		tmval.tv_usec = nMSec*1000;
	}
	return true;
}

bool TimeUtil::Iso2TimeEx(const char* UTC_dateTime, struct timeval& tmval)
{
	char utcTime[128] = {0};
	strcpy(utcTime, UTC_dateTime);

	char* pTime = NULL;
	pTime = strchr(utcTime, 'T');
	if(pTime == NULL)
		return false;
	
	char* pZone = NULL;
	char zonePrefix = '\0';
	
	if (NULL == (pZone = strchr(pTime, 'Z')))
	{
		if (NULL ==(pZone = strchr(pTime, '+')))
			pZone = strchr(pTime, '-');
	}

	if(pZone == NULL || *pZone == 'Z')	
		return Iso2Time(UTC_dateTime, tmval);
 	

	zonePrefix = *pZone;
	*pZone++ = '\0';
	
	struct timeval val;
	if( !Iso2Time(utcTime, val))
		return false;

	char buf[6][16];
	memset(buf, 0, sizeof(buf));

	int zoneSecOffset=0;
	int c =0;
	c = ::sscanf(pZone, "%[^:]:%2[0-9]", buf[0], buf[1]);
	if (c>0)
	{
		zoneSecOffset = atoi(buf[0]) *60;
		if (c>1)
			zoneSecOffset += atoi(buf[1]);
		zoneSecOffset*= 60;
		if ('-'  == zonePrefix)
			zoneSecOffset = -zoneSecOffset;
	}

	tmval.tv_sec = val.tv_sec - zoneSecOffset;
	tmval.tv_usec = val.tv_usec;

	return true;		
}

bool TimeUtil::Time2Iso(time_t t, char* UTC_datetime, int length)
{
	if(NULL == UTC_datetime || length <= UTC_LEN_STD)
	{
		return false;
	}

	// turn time_t to tm struct
	struct tm atm;

	gmtime_r(&t, &atm);

	snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02dZ", 
			              atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
						  atm.tm_hour, atm.tm_min, atm.tm_sec
						  );	

	return true;
}

bool TimeUtil::Time2Iso(struct tm& st_utc, char* UTC_datetime, int length)
{
	if(NULL == UTC_datetime || length <= UTC_LEN_STD)
	{
		return false;
	}

	snprintf(UTC_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02dZ",
							st_utc.tm_year+1900,st_utc.tm_mon+1,st_utc.tm_mday,
							st_utc.tm_hour,st_utc.tm_min,st_utc.tm_sec
							);

	return true;	
}

bool TimeUtil::Time2Iso(struct timeval& tmval, char* UTC_datetime, int length, bool local)
{
	if(NULL == UTC_datetime || length <= UTC_LEN_STD)
	{
		return false;
	}

	// turn time_t to tm struct
	struct tm atm;

    int nMSec = tmval.tv_usec/1000;
	time_t t = tmval.tv_sec;
    
    if(local) {
        localtime_r(&t, &atm);
#ifdef ZQ_COMMON_ANDROID
        long timezone;
#endif
        char sym = (timezone > 0 ? '-' : '+');
        int hour = abs(timezone)/3600;
        int min = abs(timezone)%3600/60;

		if (nMSec != 0)
			snprintf(UTC_datetime, 255,  "%4d-%02d-%02dT%02d:%02d:%02d.%03d%c%02d:%02d", 
					atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
					atm.tm_hour, atm.tm_min, atm.tm_sec, nMSec, sym, hour, min);
		else
			snprintf(UTC_datetime, 255,  "%4d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", 
					atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
					atm.tm_hour, atm.tm_min, atm.tm_sec, sym, hour, min);
    }
    else {
        gmtime_r(&t, &atm);
    
		if (nMSec != 0)
			snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ", 
                 atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
                 atm.tm_hour, atm.tm_min, atm.tm_sec, nMSec);
		else
			snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02dZ", 
				atm.tm_year+1900, atm.tm_mon+1, atm.tm_mday,
				atm.tm_hour, atm.tm_min, atm.tm_sec);
    }

	
	return true;
}

bool TimeUtil::Local2Iso(const char* LOCAL_datetime, char* UTC_datetime, int length)
{
	if(NULL == LOCAL_datetime || NULL == UTC_datetime || length <= UTC_LEN_STD)
	{
		return false;
	}
	
	struct tm tm_local,tm_utc;
	if(!Iso2Time(LOCAL_datetime,tm_local))
		return false;

	time_t utct = mktime(&tm_local);
	
	if(gmtime_r(&utct, &tm_utc) == NULL) {
		return false;
	}

	snprintf(UTC_datetime, length,  "%4d-%02d-%02dT%02d:%02d:%02dZ", 
			              tm_utc.tm_year+1900, tm_utc.tm_mon+1, tm_utc.tm_mday,
						  tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec
						  );
	
	return true;
}

bool TimeUtil::Iso2Local(const char* UTC_datetime, char* LOCAL_datetime, int length)
{
	if(NULL == UTC_datetime || NULL == LOCAL_datetime || length <= UTC_LEN_LOCAL)
	{
		return false;
	}

	struct tm tm_local,tm_utc;
	if(!Iso2Time(UTC_datetime,tm_utc))
		return false;

	time_t t = Tmtime2Time(tm_utc);
	if(t == (time_t)-1)
		return false;

	localtime_r(&t, &tm_local);

	snprintf(LOCAL_datetime, length, "%4d-%02d-%02dT%02d:%02d:%02d",
							tm_local.tm_year+1900,tm_local.tm_mon+1,tm_local.tm_mday,
							tm_local.tm_hour,tm_local.tm_min,tm_local.tm_sec
							);

	return true;
}
#endif

bool TimeUtil::Str2Time(const char* datetime, time_t& timestamp)
{
	if(!datetime) return false;

	int year, month, day, hour, minute, second;
	if(sscanf(datetime, 
			  "%4d-%02d-%02dT%02d:%02d:%02d",
		      &year, &month, &day, &hour, &minute, &second) != 6)
		return false;


	if (year < 1970 || year > 2038 || 
		month < 1   || month > 12  || 
		day < 1     || day > 31    || 
		hour < 0    || hour > 23   || 
		minute < 0  || minute > 59 || 
		second < 0  || second > 59)
		return false;

	struct tm t;
    memset(&t, 0, sizeof(struct tm));
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;
//	t.tm_isdst = 0;
	t.tm_isdst = (-1);

	timestamp = mktime(&t);

	return (timestamp != -1);
}

bool TimeUtil::Time2Str(time_t timestamp, char* datetime, int len)
{
	if(!datetime || len <= UTC_LEN_LOCAL) return false;

	struct tm* ptm = localtime(&timestamp);
    if(NULL == ptm) {
        return false;
    }
    struct tm& t = *ptm;
	short s = sprintf(datetime, "%4d-%02d-%02dT%02d:%02d:%02d", 
				t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	
	return (s == 19);
}

bool TimeUtil::UnitStr2Time(const char* unitTime, int& msTime)
{
	if (strstr(unitTime, ":"))
		return HMSStr2Time(unitTime, msTime);

	msTime = 0;
	int len = strlen(unitTime);
	int iUnit = 1;
	float iTime = 0.0;
	const char* chCur = unitTime;
	const char* p = unitTime;
	while (p - unitTime < len)
	{
		while (isdigit(*p) || *p == '.')//move to character
		{
			p++;
		}
		if (*p == 'h' || *p == 'H')//hour
		{
			iUnit = 60 * 60 * 1000;
			iTime = (float) atof(chCur);
		}

		if (*p == 'm' || *p == 'M')//minute
		{
			if (*(p+1) == 's' || *(p+1) == 'S')//ms
			{
				iUnit = 1000;
				iTime = (float) atof(chCur);
				if (iTime > 999)
					return false;
			}
			else
			{
				iUnit = 60 * 1000;
				iTime = (float) atof(chCur);
				if (iTime > 60)
					return false;
			}
		}

		if (*p == 's' || *p == 'S')//s
		{
			iUnit = 1 * 1000;
			iTime = (float) atof(chCur);
			if (iTime > 60)
				return false;
		}

		msTime += static_cast<int>(iUnit * iTime);
		p++;
		chCur = p;
	}

	return true;
}

bool TimeUtil::HMSStr2Time(const char* unitTime, int& msTime)
{
	::std::string strTime = unitTime;
	::std::vector<std::string> strTimeVec;

	//split unitTime string
	bool b = ::ZQ::common::stringHelper::SplitString(strTime, strTimeVec, ::std::string(".:"));
	if (b == false)
		return false;
	
	size_t vecLen = strTimeVec.size();
	if (vecLen != 4)
		return false;

	int iTime = 0;

	iTime = atoi(strTimeVec[0].c_str());
	if (iTime > 60 || iTime < 0)
		return false;

	msTime += 60 * 60 * 1000 * iTime;

	iTime = atoi(strTimeVec[1].c_str());
	if (iTime > 60 || iTime < 0)
		return false;
	msTime += 60 * 1000 * iTime;

	iTime = atoi(strTimeVec[2].c_str());
	if (iTime > 60 || iTime < 0)
		return false;
	msTime += 1000 * iTime;

	iTime = atoi(strTimeVec[3].c_str());
	if (iTime > 1000 || iTime < 0)
		return false;
	msTime += iTime;

	return true;
}

//
// now()
//

/// get the timestamp in unit msec
int64  now()
{
	return TimeUtil::now();
}

#ifdef ZQ_OS_MSWIN
// return the current GMT time in msec
static int64 FileTimeToTime(FILETIME filetime)
{
    unsigned __int64 ltime;
    memcpy(&ltime, &filetime, sizeof(ltime));
    ltime /= 10000;  //convert nsec to msec

    return ltime;
}

int64 TimeUtil::now()
{
	FILETIME systemtimeasfiletime;
	GetSystemTimeAsFileTime(&systemtimeasfiletime);
	return FileTimeToTime(systemtimeasfiletime);
}

// return the UTC formatted time string
const char* TimeUtil::TimeToUTC(int64 time, char* buf, const int maxlen , bool bLocalZone )
{
	if (NULL == buf)
		return NULL;

	SYSTEMTIME systime;
	FILETIME filetime;
	time *= 10000; // //convert msec to nsec
	memcpy(&filetime, &time, sizeof(filetime));

	if (::FileTimeToSystemTime(&filetime, &systime) ==FALSE)
		return NULL;

	return ZQ::common::TimeUtil::Time2Iso(systime, buf, maxlen,bLocalZone ) ? buf : NULL;
}

/// convert UTC string format to SYSTEMTIME format
int64 TimeUtil::ISO8601ToTime(const char* ISO8601Str)
{
	SYSTEMTIME systime;
	if (NULL == ISO8601Str || !ZQ::common::TimeUtil::Iso2TimeEx(ISO8601Str, systime))
		return 0;

	FILETIME filetime;
	unsigned __int64 ltime;
	
	if (FALSE == SystemTimeToFileTime(&systime, &filetime))
		return 0;

	memcpy(&ltime,&filetime,sizeof(filetime));
	ltime /= 10000;  //convert nsec to msec

	return ltime;
}

#else

/// get the timestamp in unit msec
int64 TimeUtil::now()
{
    struct timeval tmval;
    int ret = gettimeofday(&tmval,(struct timezone*)NULL);
    if(ret != 0)
        return 0;
    return (tmval.tv_sec*1000LL + tmval.tv_usec/1000);
}

// convert millisecond time to  UTC formatted time string
const char* TimeUtil::TimeToUTC(int64 time, char* buf, const int maxlen, bool local)
{
	if(buf == NULL)
		return NULL;
	struct timeval tmval;
	tmval.tv_sec = time/1000;
	tmval.tv_usec = (time%1000)*1000L;
	
	bool res = ZQ::common::TimeUtil::Time2Iso(tmval,buf,maxlen, local);
	if(res)
		return buf;
	else
		return NULL;
}

/// convert UTC string format to millisecond
/// return 0 on failure
int64 TimeUtil::ISO8601ToTime(const char* ISO8601Str)
{
	if(ISO8601Str == NULL)
		return 0;
	struct timeval tmval;
	bool res = ZQ::common::TimeUtil::Iso2TimeEx(ISO8601Str,tmval);
	if(res)
		return (tmval.tv_sec*1000LL + tmval.tv_usec/1000);
	else
		return 0;
}
#endif

static void readTimeLenToken(std::string& str, int64& len, const char* leadingChs)
{
	size_t pos = str.find_first_of(leadingChs);
	if (std::string::npos != pos)
	{
		len += atoi(str.substr(0, pos).c_str());
		str = str.substr(pos+1);
		pos = str.find_first_of("0123456789");
		if (std::string::npos != pos)
			str = str.substr(pos);
	}
}

// accpeted units: D(d)ay, H(h)our, M(m)in, S(s)ec
// allowed time length formats: "3day 1hour 5min", "3d1h5m40s111", "3d1h5m40s111"
// return the time len in milliseconds
//TODO enable later: int64 TimeUtil::parseTimeLength(const char* timeLenStr)
int64 parseTimeLength(const char* timeLenStr)
{
	int64 len =0;
	if (NULL == timeLenStr)
		return 0;
	std::string str = timeLenStr;

	// about the days
	readTimeLenToken(str, len, "Dd");
	len *= 24;

	// about the hours
	readTimeLenToken(str, len, "Hh");
	len *= 60;

	// about the minutes
	readTimeLenToken(str, len, "Mm");
	len *= 60;

	// about the seconds
	readTimeLenToken(str, len, "Ss");
	len *= 1000;

	// about the milliseconds
	len += atoi(str.c_str());
	return len;
}

}}
