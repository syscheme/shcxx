// ===========================================================================
// Copyright (c) 2006 by
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
// Ident : $Id: TimeoutContainer.h $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : 
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/TimeoutContainer.cpp $
// 
// 1     07-07-20 19:19 Hui.shao
// ===========================================================================
#include "TimeoutContainer.h"

namespace ZQTianShan {

// -----------------------------
// class EvictorTimeoutCmd
// -----------------------------
class EvictorTimeoutCmd : public ZQ::common::ThreadRequest
{
	friend class TimeoutContainer;
	EvictorTimeoutCmd(TimeoutContainer& owner, const Ice::Identity& ident)
		: _owner(owner), _ident(ident), ThreadRequest(owner._thrdPool)
	{
	}

protected:
	virtual int run(void);
	virtual void final(int retcode =0, bool bCancelled =false);

	TimeoutContainer& _owner;
	Ice::Identity   _ident;
};

// -----------------------------
// class TimeoutObjFactory
// -----------------------------
class TimeoutObjFactory : public Ice::ObjectFactory
{
	friend class TimeoutContainer;

public:

    TimeoutObjFactory(TimeoutContainer& owner);

    // Operations from ObjectFactory
    virtual Ice::ObjectPtr create(const std::string&);
    virtual void destroy();

protected:
	TimeoutContainer& _owner;
};


// -----------------------------
// class TimeoutContainer
// -----------------------------
TimeoutContainer::TimeoutContainer(ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdPool, ::Ice::ObjectAdapterPtr& iceAdapter, int evictorSize, const char* dbDir, const char* dbName)
: _log(log), _thrdPool(thrdPool), ThreadRequest(thrdPool), _adapter(iceAdapter), _evictorSize(evictorSize), _hWakeupEvent(NULL), _bQuit(false), _bReady(false)
{
	if (NULL != dbDir)
	{
		::CreateDirectory(dbDir, NULL);
		_dbDir = dbDir;
	}

	if (NULL !=dbName && strlen(dbName) >0)
		_dbName = dbName;

	_hWakeupEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (_evictorSize <0)
		_evictorSize = 0;

	_factory = new TimeoutObjFactory(*this);

	regFactoryObjId(::TianShanIce::TimeoutObj::ice_staticId());
}

TimeoutContainer::~TimeoutContainer()
{
	quit();

	if(_hWakeupEvent)
		::CloseHandle(_hWakeupEvent);
	_hWakeupEvent = NULL;
}

void TimeoutContainer::wakeup()
{
	if (_hWakeupEvent)
		::SetEvent(_hWakeupEvent);
	::Sleep(1);
}

void TimeoutContainer::quit()
{
	_bQuit=true;
	wakeup();
}

void TimeoutContainer::watch(const Ice::Identity& ident, ::Ice::Long msTimeout)
{
	if (msTimeout <0)
		msTimeout = 0;
	
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutContainer, "watch obj [%s/%s] timeout=%dmsec"), ident.category.c_str(), ident.name.c_str(), msTimeout);
	
	::Ice::Long newExp = now() + msTimeout;
	{
		ZQ::common::MutexGuard gd(_lockExpirations);
		_expirations[ident] = newExp;
	}
	
	if (newExp < _nextWakeup)
		wakeup();
}

::TianShanIce::TimeoutObjPrx TimeoutContainer::identToObj(const Ice::Identity& ident)
{
	ZQ::common::MutexGuard gd(_evictorLocker);
	return ::TianShanIce::TimeoutObjPrx::checkedCast(_adapter->createProxy(ident));
}

bool TimeoutContainer::addIndex(Freeze::IndexPtr& idx)
{
	if (!idx || _bReady)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) null index or wrong state: bReady=%d"), (DWORD) this, _bReady?1:0);
		return false;
	}

	ZQ::common::MutexGuard gd(_evictorLocker);
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutContainer, "(%08x) add index: %s"), (DWORD) this, idx->name().c_str());

	_indices.push_back(idx);
	return true;
}

::Ice::ObjectPrx TimeoutContainer::add(TimeoutObjImpl::Ptr& timeoutObj)
{
	if (!_bReady)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) add() not ready for adding yet, reject"), (DWORD)this);
		return NULL;
	}
	
	try
	{
		if (timeoutObj->ident.name.empty())
			timeoutObj->ident.name = ::IceUtil::generateUUID();

		if (timeoutObj->ident.category.empty())
			timeoutObj->ident.category = _dbName;

		ZQ::common::MutexGuard gd(_evictorLocker);
		::TianShanIce::TimeoutObjPrx prx =::TianShanIce::TimeoutObjPrx::uncheckedCast(_evictor->add(timeoutObj, timeoutObj->ident));

		printf("add.prx=%s\n", _adapter->getCommunicator()->proxyToString(prx).c_str());
		printf("add.name=%s\n", prx->getIdent().name.c_str());

		watch(timeoutObj->ident, timeoutObj->expiration - now());

		return prx;
	}
	catch(const Ice::Exception& ex)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) add obj caught exception: %s"), (DWORD)this, ex.ice_name().c_str());
		return NULL;
	}
	catch(...)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) add obj caught unknown exception"), (DWORD)this);
		return NULL;
	}
}

Ice::ObjectPtr TimeoutContainer::createForFactory(const std::string& type)
{
	if (::TianShanIce::TimeoutObj::ice_staticId() == type)
		return new TimeoutObjImpl(*this);

	return NULL;
}

bool TimeoutContainer::regFactoryObjId(const std::string& objStaticId)
{
	if (!_adapter || objStaticId.empty() || !_factory)
		return false;

	if (_objStaticIds.end() != _objStaticIds.find(objStaticId))
		return true;

	Ice::CommunicatorPtr ic = _adapter->getCommunicator();
	
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutObjFactory, "(%08x) regFactoryObj onto communicator: %s"), (DWORD)this, objStaticId.c_str());
	
	ic->addObjectFactory(_factory, objStaticId);

	_objStaticIds.insert(objStaticId);

	return true;
}


bool TimeoutContainer::init(void)
{
	if (_dbDir.empty() || _dbName.empty() || NULL == _hWakeupEvent)
		return false;

	try 
	{
		_log(ZQ::common::Log::L_INFO, CLOGFMT(TimeoutContainer, "(%08x) opening database %s at path: %s"), (DWORD)this, _dbName.c_str(), _dbDir.c_str());
		
		_evictor = Freeze::createEvictor(_adapter, _dbDir, _dbName, 0, _indices);
		_adapter->addServantLocator(_evictor, _dbName);
		_evictor->setSize(_evictorSize);
	}
	catch(const Ice::Exception& ex)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) open db caught exception: %s"), (DWORD)this, ex.ice_name().c_str());
		return false;
	}
	catch(...)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(TimeoutContainer, "(%08x) open db caught unknown exception"), (DWORD)this);
		return false;
	}

	_log(ZQ::common::Log::L_INFO, CLOGFMT(TimeoutContainer, "(%08x) database opened"), (DWORD)this);

	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutContainer, "(%08x) restoring all the object from last open"), (DWORD)this);
	IdentCollection identities;
	try	{
		ZQ::common::MutexGuard gd(_evictorLocker);
		::Freeze::EvictorIteratorPtr itptr = _evictor->getIterator("", 100);
		while (itptr->hasNext())
			identities.push_back(itptr->next());
	}
	catch(...)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SessionManager, "failed to list all the existing sessions"));
	}

	_log(ZQ::common::Log::L_INFO, CLOGFMT(SessionManager, "(%08x) found %d objects to watch"), (DWORD)this, identities.size());

	for (IdentCollection::iterator it = identities.begin(); it !=identities.end(); it ++)
		watch(*it, 0);

	_bReady = true;

	return _bReady;
}

#define MIN_YIELD	(100)  // 100 msec
#define DEFAULT_MAX_IDLE	(60* 1000)  // 1min

int TimeoutContainer::run()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutContainer, "(%08x) is running"), (DWORD)this);
	while(!_bQuit)
	{
		::Ice::Long timeOfNow = now();

		IdentCollection timeoutIdents;

		{
			ZQ::common::MutexGuard gd(_lockExpirations);
			_nextWakeup = timeOfNow + DEFAULT_MAX_IDLE;
			
			for(ExpirationMap::iterator it = _expirations.begin(); it != _expirations.end(); it ++)
			{
				if (it->second <= timeOfNow)
					timeoutIdents.push_back(it->first);
				else
					_nextWakeup = (_nextWakeup > it->second) ? it->second : _nextWakeup;
			}

			for (IdentCollection::iterator it2 = timeoutIdents.begin(); it2 < timeoutIdents.end(); it2++)
				_expirations.erase(*it2);
		}

		if(_bQuit)
			break;	// should quit polling

		for (IdentCollection::iterator it = timeoutIdents.begin(); it < timeoutIdents.end(); it++)
			(new EvictorTimeoutCmd(*this, *it))->start();

		if(_bQuit)
			break;	// should quit polling

		long sleepTime = (long) (_nextWakeup - now());

		if (sleepTime < MIN_YIELD)
			sleepTime = MIN_YIELD;

		::WaitForSingleObject(_hWakeupEvent, sleepTime);

	} // while

	_log(ZQ::common::Log::L_WARNING, CLOGFMT(SessionWatchDog, "end of Session WatchDog"));

	return 0;
}

// -----------------------------
// class EvictorTimeoutCmd
// -----------------------------
int EvictorTimeoutCmd::run(void)
{
	::TianShanIce::TimeoutObjPrx prx;

	try
	{
		prx = _owner.identToObj(_ident);
		printf("prx=%s\n", _owner._adapter->getCommunicator()->proxyToString(prx).c_str());
		printf("name=%s\n", prx->getIdent().name.c_str());
		
		prx->OnTimeout();
		return 0;
	}
	catch(const ::Ice::ObjectNotExistException&)
	{
		_owner._log(ZQ::common::Log::L_DEBUG, CLOGFMT(EvictorTimeoutCmd, "obj[%s/%s] Object not found in container"), _ident.category.c_str(), _ident.name.c_str());
		return 0;
	}
	catch(const ::Ice::Exception& ex)
	{
		_owner._log(ZQ::common::Log::L_ERROR, CLOGFMT(EvictorTimeoutCmd, "obj[%s/%s] exception occurs: %s"), _ident.category.c_str(), _ident.name.c_str(), ex.ice_name().c_str());
	}
	catch(...)
	{
		_owner._log(ZQ::common::Log::L_ERROR, CLOGFMT(EvictorTimeoutCmd, "obj[%s/%s] unknown exception occurs"), _ident.category.c_str(), _ident.name.c_str());
	}

	// when reaches here, an exception might occur, when re-post a timer command to ensure no action is dropped
	try
	{
		_owner.watch(_ident, DEFAULT_MAX_IDLE);
	}
	catch(...)
	{
	}

	return -1;
}

void EvictorTimeoutCmd::final(int retcode, bool bCancelled)
{
	if (bCancelled)
		_owner._log(ZQ::common::Log::L_DEBUG, CLOGFMT(EvictorTimeoutCmd, "obj[%s/%s] user canceled timer activity"), _ident.category.c_str(), _ident.name.c_str());
	delete this;
}

// -----------------------------
// class TimeoutObjFactory
// -----------------------------
TimeoutObjFactory::TimeoutObjFactory(TimeoutContainer& owner)
: _owner(owner)
{
}

Ice::ObjectPtr TimeoutObjFactory::create(const std::string& type)
{
	if (_owner._objStaticIds.end() != _owner._objStaticIds.find(type))
		return _owner.createForFactory(type);

	_owner._log(ZQ::common::Log::L_WARNING, CLOGFMT(TimeoutObjFactory, "(%08x) create(%s) type unknown"), (DWORD)&_owner, type.c_str());
    return NULL;
}

void TimeoutObjFactory::destroy()
{
	_owner._log(ZQ::common::Log::L_DEBUG, CLOGFMT(TimeoutObjFactory, "(%08x) destroy()"), (DWORD)&_owner);
}

// -----------------------------
// class TimeoutObjImpl
// -----------------------------
TimeoutObjImpl::TimeoutObjImpl(TimeoutContainer& container)
: _container(container) 
{
	 WLock sync(*this);
}

::Ice::Identity TimeoutObjImpl::getIdent(const ::Ice::Current& c) const
{
	RLock sync(*this);
	return ident;
}

::Ice::Long TimeoutObjImpl::getExpiration(const ::Ice::Current& c) const
{
	RLock sync(*this);
	return expiration;
}

void TimeoutObjImpl::OnTimeout(const ::Ice::Current& c)
{
	destroy(c);
}

void TimeoutObjImpl::destroy(const ::Ice::Current& c)
{
    WLock sync(*this);
	try
	{
		_container._log(ZQ::common::Log::L_INFO, OBJLOGFMT(TimeoutObjImpl, "(%08x) destroy()"), (DWORD)&_container);
		_container._evictor->remove(c.id);
	}
	catch(const ::Ice::ObjectNotExistException&)
	{
		_container._log(ZQ::common::Log::L_DEBUG, OBJLOGFMT(TimeoutObjImpl, "(%08x) object already gone, ignore"), (DWORD)&_container);
	}
	catch(const ::Ice::Exception& ex)
	{
		_container._log(ZQ::common::Log::L_DEBUG, OBJLOGFMT(TimeoutObjImpl, "(%08x) exception %s"), (DWORD)&_container, ex.ice_name().c_str());
	}
}

void TimeoutObjImpl::renew(::Ice::Long TTL, const ::Ice::Current& c)
{
	if (TTL < 0)
		return;

    WLock sync(*this);
	expiration = now() + TTL;
}

} // namespace
