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
// Ident : $Id: PerfCounter.h,v 1.13 2004/07/22 06:16:42 shao Exp $
// Branch: $Name:  $
// Author: Hui.Shao
// Desc  : A counter to record performance data
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/PerfCounter.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 1     08-04-07 19:00 Hui.shao
// create a util classes to count measurealbe performance data
// ===========================================================================
#ifndef __ZQ_COMMON_PerfCounter_H__
#define __ZQ_COMMON_PerfCounter_H__

#include "ZQ_common_conf.h"
#include "Locks.h"

extern "C" {
#include <time.h>
}

namespace ZQ{
namespace common{

class ZQ_COMMON_API PerfCounter;

// -----------------------------
// class PerfCounter
// -----------------------------
/// a utiltiy class to stat some performance measurable points
class PerfCounter
{
public:

	/// constructor
	///@param size the counter size to initialize
	PerfCounter(const size_t size);

	/// destructor
	virtual ~PerfCounter();

	/// add a new performnace measure record into the counter
	///@param value the measured value, the caller must round it to unsigned integer
	void put(const uint value, time_t stamp=0);

	///definition of stat result
	typedef struct _Stat
	{
		uint average, max, min;
		uint count;
		time_t from, to;
	} Stat;

	/// stat the data that have been put in the counter
	///@param from to specify the time window start time, 0 will stat as earily as possible records that are available in the counter
	///@param to to specify the time window end time, 0 will stat as late as possible records that are available in the counter
	///@return the stat result
	Stat stat(const time_t from =0, const time_t to =0);

	/// to expire some old records
	///@param til to specify til when that the records should be expired
	void expire(const time_t til);

protected:

	typedef struct _Record
	{
		uint value;
		time_t stamp;
	} Record;

	Record* _perfRecords;
	Mutex   _lock;
	size_t _idxHeader, _idxTail;
	size_t  _size;
};

} }//endof namespace

#endif // __ZQ_COMMON_PerfCounter_H__

