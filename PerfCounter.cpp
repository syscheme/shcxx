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
// Ident : $Id: PerfCounter.h,v 1.13 2004/07/22 06:16:42 shao Exp $
// Branch: $Name:  $
// Author: Hui.Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/PerfCounter.cpp $
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

#include "PerfCounter.h"

namespace ZQ{
namespace common{

#define MIN_COUNTER_SIZE 10

// -----------------------------
// class PerfCounter
// -----------------------------
/// constructor
PerfCounter::PerfCounter(const size_t size)
: _perfRecords(NULL), _idxHeader(0), _idxTail(0), _size(size +1)
{
	if (_size < MIN_COUNTER_SIZE)
		_size = MIN_COUNTER_SIZE;

	_perfRecords = new Record[size];
}

PerfCounter::~PerfCounter()
{
	MutexGuard g(_lock);

	if (_perfRecords)
		delete [] _perfRecords;
	
	_perfRecords = NULL;
	_idxHeader = _idxTail =0;
}

void PerfCounter::put(const uint value, time_t stamp)
{
	if (stamp <=0)
		::time(&stamp);

	MutexGuard g(_lock);
	if (!_perfRecords)
		return;

	_perfRecords[_idxHeader].value = value;
	_perfRecords[_idxHeader].stamp = stamp;

	_idxHeader = (++_idxHeader) % _size;
	if (_idxTail == _idxHeader)
		_idxTail = (_idxHeader +1) % _size;
}

PerfCounter::Stat PerfCounter::stat(const time_t from, const time_t to)
{
	Stat result;
	memset(&result, 0x00, sizeof(result));

	Record* tmpAllRecords = new Record[_size];
	size_t idxHeader =0,  idxTail =0;

	if (NULL == tmpAllRecords)
	{
		delete[] tmpAllRecords;
		return result;
	}
	else
	{
		// make a copy of _perfRecords to shortern the locking
		MutexGuard g(_lock);
		if (NULL == _perfRecords)
		{
			delete[] tmpAllRecords;
			return result;
		}

		::memcpy(tmpAllRecords, _perfRecords, sizeof(Record) * _size);
		idxHeader = _idxHeader;
		idxTail = _idxTail;
	}

	int64 subtotal =0;
	result.min = ~ (uint) 1;
	result.max = 0;

	for (size_t i = idxTail; i != idxHeader && (to <=0 || tmpAllRecords[i].stamp <= to); i= (++i) % _size)
	{
		// skip all that earier than "from"
		if (from >0 && from > tmpAllRecords[i].stamp)
			continue;

		if (result.count++ ==0)
			result.from = tmpAllRecords[i].stamp;
		subtotal += tmpAllRecords[i].value;
		result.min = min(result.min, tmpAllRecords[i].value);
		result.max = max(result.max, tmpAllRecords[i].value);

		result.to = tmpAllRecords[i].stamp;
	}

	delete[] tmpAllRecords;

	if (result.count >0)
		result.average = (uint) (subtotal / result.count); 

	return result;
}

void PerfCounter::expire(const time_t til)
{
	MutexGuard g(_lock);
	if (!_perfRecords)
		return;

	while (_perfRecords[_idxTail].stamp < til && _idxTail != _idxHeader)
		_idxTail = (++_idxTail) % _size;
}

}}//endof namespace

/*
void main()
{
	ZQ::common::PerfCounter counter(1000);

	for (int i=0; i<7700; i++)
	{
		counter.put(i %9 +1);
//		::Sleep(50);
	}

	::Sleep(1000);
	time_t now;
	counter.expire(::time(&now));

	ZQ::common::PerfCounter::Stat stat = counter.stat();
	printf("count=%d, min=%d, max=%d, avg=%d, from=%x, to=%x\n",
		stat.count, stat.min, stat.max, stat.average, stat.from, stat.to);
}

*/