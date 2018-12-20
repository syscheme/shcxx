// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai
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
// Ident : $Id: ZQSnmp.h,v 1.8 2014/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define SNMP exports
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/eloop/eloop_net.h $
// ===========================================================================
#ifndef __ZQ_COMMON_ELOOP_ThreadPool_H__
#define __ZQ_COMMON_ELOOP_ThreadPool_H__

#include "eloop.h"
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API ThreadRequest;

// ---------------------------------------
// class ThreadRequest
// ---------------------------------------
class ThreadRequest:public EloopRequest
{
public:
	ThreadRequest(Loop& loop);

protected:
	virtual void doWork() {}
	virtual void OnAfterWork(int status) {delete this;}

private:
	int work();
	static void _cbWork(uv_work_t *req);
	static void _cbAfterWrok(uv_work_t *req,int status);

private:
	Loop& _loop;
};

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_ThreadPool_H__