// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: Counter.h,v 1.5 2004/07/09 11:21:36 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define Socket class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Counter.h $
// 
// 3     9/06/13 2:44p Hui.shao
// 
// 2     9/02/13 2:28p Hui.shao
// added till stamp to calculate more exactly
// 
// 1     2/16/12 11:10a Build
// 
// 4     11/28/11 12:44p Hui.shao
// 
// 3     11/18/11 2:20p Build
// 
// 2     11/18/11 1:28p Hui.shao
// 
// 1     11/18/11 12:10p Hui.shao
// 
// ============================================================================================

#ifndef __ZQ_COM_Count_H__
#define __ZQ_COM_Count_H__

#include "ZQ_common_conf.h"
#include "Locks.h"

#ifndef _WIN32
#include <sys/time.h>
#include <string>
#endif

#define FloatWindowCounters_MIN_TIME_WIN (100) // 100msec

namespace ZQ {
namespace common {

// class ZQ_COMMON_API FloatWindowCounters;

// -----------------------------
// class FloatWindowCounters
// -----------------------------
class FloatWindowCounters
{
public:

	typedef struct _CounterNode
	{
		int64  winSince, winTill;
		int32  countInWin;
	} CounterNode;

	FloatWindowCounters(uint winSize, uint countSize=0);
	virtual ~FloatWindowCounters();

	virtual void addCount(uint counterIndx =-1, int64 stamp=0, uint count=1, bool individual=true);
	virtual void levelageAll(int64 stamp =0);
	int32 getCount(uint counterIndx =-1);
	CounterNode* getCounter(uint counterIndx =-1);

protected:

	void _incCount(CounterNode& counter, uint32 count=1, int64 stamp=0);
	void _levelage(CounterNode& counter, int64 stamp =0);

	CounterNode     _summary;

	CounterNode*    _subCounters;
	ZQ::common::Mutex _locker;
	uint _winSize, _countSize;
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COM_Count_H__
