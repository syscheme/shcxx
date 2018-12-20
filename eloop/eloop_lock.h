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
// $Log: /ZQProjs/Common/eloop/eloop_lock.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_Lock_H__
#define __ZQ_COMMON_ELOOP_Lock_H__

#include "eloop.h"

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API Mutex;
class ZQ_ELOOP_API Condition;

// -----------------------------
// class Mutex
// -----------------------------
class Mutex
{
	friend class Condition;

public:
	explicit Mutex();
	~Mutex();

	void lock() const;

	void unlock() const;

	int tryLock() const;

private:
	uv_mutex_t _mutex;
	Mutex(const Mutex &);
	Mutex &operator=(const Mutex &);

public:

	class Guard
	{
	public:
		explicit Guard(const Mutex& mutex) : _mutex(mutex)	{ _mutex.lock(); }
		~Guard() { _mutex.unlock();	}

	private:
		const Mutex& _mutex;
	};

};

// -----------------------------
// class Condition
// -----------------------------
class Condition
{
public:
	explicit Condition();
	~Condition();

	//Unblock at least one of the threads that are blocked on this condition
	void signal();

	//Unblock all threads currently blocked on this condition
	void broadcast();

	//Block on this condition variable, mutex needs to be locked
	void wait(Mutex *mutex);

	//Block on this condition variable for the given amount of time, mutex needs to be locked
	int timedwait(Mutex *mutex, uint64_t timeout);

private:
	uv_cond_t _cond;

};

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_Lock_H__
