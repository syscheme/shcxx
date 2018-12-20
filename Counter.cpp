// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: Socket.cpp,v 1.7 2004/07/29 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define Socket class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Counter.cpp $
// 
// 4     9/09/13 12:54p Hui.shao
// 
// 3     9/06/13 2:44p Hui.shao
// 
// 2     9/02/13 2:28p Hui.shao
// added till stamp to calculate more exactly
// 
// 1     2/16/12 11:10a Build
// 
// 3     1/05/12 5:51p Hui.shao
// 
// 2     11/18/11 1:28p Hui.shao
// 
// 1     11/18/11 12:10p Hui.shao
// ===========================================================================

#include "Counter.h"
#include "TimeUtil.h"

namespace ZQ {
namespace common {

// -----------------------------
// class FloatWindowCounters
// -----------------------------
FloatWindowCounters::FloatWindowCounters(uint winSize, uint countSize)
: _subCounters(NULL), _winSize(winSize), _countSize(countSize)
{
	if (_winSize < FloatWindowCounters_MIN_TIME_WIN)
		_winSize = FloatWindowCounters_MIN_TIME_WIN;

	memset(&_summary, 0, sizeof(_summary));

	if (_countSize <0 )
		_countSize =0;

	_subCounters = new CounterNode[_countSize +1];
	if (NULL != _subCounters)
		memset(_subCounters, 0, sizeof(CounterNode) * (_countSize +1));
}

FloatWindowCounters::~FloatWindowCounters()
{
	ZQ::common::MutexGuard g(_locker);
	if (NULL != _subCounters)
		delete[] _subCounters;

	_subCounters = NULL;
}

void FloatWindowCounters::addCount(uint counterIndx, int64 stamp, uint count, bool individual)
{
	if (stamp <=0)
		stamp = ZQ::common::now();

	ZQ::common::MutexGuard g(_locker);
	if (!individual)
	{
		_levelage(_summary, stamp);
		_incCount(_summary, count, stamp);
	}

	if (counterIndx < 0 || counterIndx >= _countSize)
		return;

	_levelage(_subCounters[counterIndx], stamp);
	_incCount(_subCounters[counterIndx], count, stamp);
}

void FloatWindowCounters::levelageAll(int64 stamp)
{
	if (stamp <=0)
		stamp = ZQ::common::now();

	ZQ::common::MutexGuard g(_locker);
		_levelage(_summary, stamp);

	for(int i =0; i<_countSize; i++)
		_levelage(_subCounters[i], stamp);
}


int32 FloatWindowCounters::getCount(uint counterIdx)
{
	CounterNode* pCounter = getCounter(counterIdx);
	if (NULL == pCounter)
		return 0;
	
	int32 ret = pCounter->countInWin;
//	ret <<=1;
//	ret += (ret & 0x07)?1:0;
	return ret;
}

FloatWindowCounters::CounterNode* FloatWindowCounters::getCounter(uint counterIdx)
{
	CounterNode* pCounter = &_summary;

	if (counterIdx >= 0 && counterIdx < _countSize)
		pCounter = &_subCounters[counterIdx];

	ZQ::common::MutexGuard g(_locker);
	if (NULL != pCounter)
		_levelage(*pCounter, 0);

	return pCounter;
}


void FloatWindowCounters::_incCount(CounterNode& counter, uint32 count, int64 stamp)
{
	counter.countInWin += count;
	if (counter.countInWin <0)
		counter.countInWin =0;

	if (stamp > counter.winTill)
		counter.winTill = stamp;

	// _levelage(counter, stamp);
}

#ifndef max
#define max(_X, _Y) ((_X>_Y)?_X:_Y)
#define min(_X, _Y) ((_X<_Y)?_X:_Y)
#endif

void FloatWindowCounters::_levelage(CounterNode& counter, int64 newTill)
{
	if (newTill <= 0)
		newTill = ZQ::common::now();

	int64 newSince  = newTill - _winSize;
	newSince        = max(newSince, counter.winSince);
	newTill         = min(newTill, counter.winTill); // new Till

	int64 oldWin = counter.winTill - counter.winSince;

	if (oldWin <=0)
	{
		// illegal, adjust the values
		counter.winTill = counter.winSince;
		return;
	}

	int64 delta = newSince - counter.winSince;
	if (delta >0)
	{
		// cut-off needed, keep the old values
		delta = (counter.countInWin * delta) / oldWin;
		if (delta>0)
		{
			counter.countInWin -= delta;  
			counter.winSince   = newSince;
		}
	}

	delta = (double)counter.winTill - newTill;
	if (delta >0)
	{
		// cut-off needed, keep the old values
		delta = (counter.countInWin * delta) / oldWin;
		if (delta>0)
		{
			counter.countInWin -= delta;  
			counter.winTill   = newTill;
		}
	}

	if (counter.countInWin <0)
		counter.countInWin =0;

/*
	counter.countInWin = (counter.countInWin * deltaWin + (oldWin>>1)) / oldWin;

		// quickly calculate for the case with no time window overlaps
		counter.countInWin = 0;
		counter.winTill = counter.winSince = newTill;
		return;
	}

	if (deltaWin >=oldWin)
	{
		// completely covered, keep the old values with no changes
		return;
	}

	counter.countInWin = (counter.countInWin * deltaWin + (oldWin>>1)) / oldWin;
	counter.winSince   = newSince;
	counter.winTill    = newTill;

/*
	int64 thisWinSz = stamp - counter.winSince;
	int64 deltaWin  = newFrom - counter.winSince;

	if (thisWinSz == 0 || deltaWin <=0)
		return;

	if (thisWinSz > _winSize *2)
	{
		counter.countInWin = 0;
		counter.winSince = stamp;
		return;
	}

//	uint countToEvict = (uint)((float) counter.countInWin * deltaWin / thisWinSz +0.5);
	uint countToEvict = counter.countInWin * deltaWin / thisWinSz;

	//		uint countToEvict = (uint) ((float) (counter.countInWin * (newFrom - counter.winSince)) / _winSize + 0.5);

	if (countToEvict >0)
	{
		counter.countInWin -= countToEvict;
		counter.winSince = newFrom;
	}
	else if (counter.countInWin >0 && thisWinSz > _winSize *1.2)
	{
		counter.countInWin *= _winSize;
		counter.countInWin /= thisWinSz;
		counter.winSince = newFrom;
	}
*/
}

} // namespace common
} // namespace ZQ
