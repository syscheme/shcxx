// ===========================================================================
// Copyright (c) 2006 by
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
// Ident : $Id: TimeoutContainer.h $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/TimeoutContainer.h $
// 
// 1     07-07-20 19:19 Hui.shao
// ===========================================================================

#ifndef __ZQTianShanTimeoutEvictor_H__
#define __ZQTianShanTimeoutEvictor_H__

#include "TianShanDefines.h"
#include "Exception.h"
#include "Locks.h"
#include "Log.h"
#include "NativeThreadPool.h"

#include "TimeoutObj.h"

#include <IceUtil/IceUtil.h>
#include <Freeze/Freeze.h>

namespace ZQTianShan {

class TimeoutContainer;
class TimeoutObjImpl;

// -----------------------------
// class TimeoutObjImpl
// -----------------------------
class TimeoutObjImpl : virtual public ::TianShanIce::TimeoutObj, virtual public IceUtil::AbstractMutexReadI<IceUtil::RWRecMutex>
{	
public:
    TimeoutObjImpl(TimeoutContainer& container);
	virtual ~TimeoutObjImpl() {}

	typedef ::IceInternal::Handle< TimeoutObjImpl > Ptr;

public:	// impls of TimeoutObj

    virtual ::Ice::Identity getIdent(const ::Ice::Current& c) const;
    virtual ::Ice::Long getExpiration(const ::Ice::Current& c) const;
    virtual void OnTimeout(const ::Ice::Current& c);
    virtual void destroy(const ::Ice::Current& c);
    virtual void renew(::Ice::Long TTL, const ::Ice::Current& c);

protected:

	TimeoutContainer&			_container;
};

// -----------------------------
// class TimeoutContainer
// -----------------------------
class TimeoutContainer : public ZQ::common::ThreadRequest, public IceUtil::AbstractMutexReadI<IceUtil::RWRecMutex>
{
	friend class TimeoutObjImpl;
public:

	TimeoutContainer(ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdPool, ::Ice::ObjectAdapterPtr& iceAdapter, int containerSize, const char* dbDir, const char* dbName);

	// no public constructor, use static create() to create adapter
	virtual ~TimeoutContainer();

	bool addIndex(Freeze::IndexPtr& idx);
    
	::Ice::ObjectPrx add(TimeoutObjImpl::Ptr& timeoutObj);

	void TimeoutContainer::wakeup();
	void TimeoutContainer::quit();

	void watch(const Ice::Identity& ident, ::Ice::Long msTimeout=0);

	::TianShanIce::TimeoutObjPrx identToObj(const Ice::Identity& ident);
	
	ZQ::common::Log&				_log;

protected:

	virtual Ice::ObjectPtr createForFactory(const std::string& type);
	bool regFactoryObjId(const std::string& objStaticId);

	friend class EvictorTimeoutCmd;
	friend class TimeoutObjFactory;

	typedef IceUtil::Handle<TimeoutObjFactory> FactoryPtr;

	virtual bool init(void);
	virtual int run(void);
	virtual void final(int retcode =0, bool bCancelled =false) {}

	ZQ::common::NativeThreadPool&	_thrdPool;
	std::string						_dbDir, _dbName;

	Ice::ObjectAdapterPtr			_adapter;
	FactoryPtr						_factory;

	std::vector<Freeze::IndexPtr>	_indices;
	::Freeze::EvictorPtr			_evictor;
	ZQ::common::Mutex				_evictorLocker;
	int								_evictorSize;

	HANDLE							_hWakeupEvent;
	bool							_bQuit;
	
	typedef std::map <::Ice::Identity, ::Ice::Long > ExpirationMap; // sessId to expiration map
	ZQ::common::Mutex   _lockExpirations;
	ExpirationMap		_expirations;

	::Ice::Long			_nextWakeup;
	bool				_bReady;

	std::set<::std::string> _objStaticIds;
};



} // namespace

#endif // __ZQTianShanTimeoutEvictor_H__
