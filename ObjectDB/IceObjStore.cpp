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
// Ident : $Id: ObjectStore.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl ObjectStore over Redis
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ObjectDB/IceObjStore.cpp $
// 
// 1     5/28/15 7:36p Hui.shao
// ===========================================================================

#include "IceObjStore.h"
#include <Ice/ObjectAdapter.h>
#include <Ice/BasicStream.h>

namespace ZQ {
namespace ObjectDB {

class IceServant : public ZQ::ObjectDB::ObjectServant
{
public:
	typedef POINTER< IceServant > Ptr;
	IceServant() {}
	IceServant(::Ice::ObjectPtr iservant) : ZQ::ObjectDB::ObjectServant(), _iservant(iservant) {}
	::Ice::ObjectPtr _iservant;

//public:
};

//void IceServant::__write(::IceInternal::BasicStream* os, const IceServant::Ptr& v)
//{/
//	os->write(v->_iservant);
//}
void __write(::IceInternal::BasicStream* os, const IceServant::Ptr& v);
void __read(::IceInternal::BasicStream* os, IceServant::Ptr& v);


// -----------------------------
// class IceObjStore
// -----------------------------
IceObjStore::IceObjStore(const Ice::ObjectAdapterPtr adapter, const std::string& category, ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdpool, const std::string& dbUrl, const Index::List& indices)
	: _adapter(adapter), ObjectStore(category, log, thrdpool, dbUrl, indices)
{
}

IceObjStore::~IceObjStore()
{
}

// overwrite the StoredObject
bool IceObjStore::marshal(const StoredObject::Ptr& so, std::string& ostream)
{
	//!!!TODO: this line should be updated by compiler
	// IceServant::Ptr servant = reinterpret_cast< IceServant* > so->_servant.get();
	IceServant::Ptr servant = (IceServant*) so->_servant.get();
	//!!!ends

	if (NULL == servant)
		return false;

	::IceInternal::BasicStream bstrm(NULL);
	__write(&bstrm, servant); // this func is generated by slice2cpp for each slice class
	ZQ::common::RedisClient::encode(ostream, bstrm.b.begin(), bstrm.b.size());
	return true;
}

bool IceObjStore::unmarshal(StoredObject::Ptr& so, const std::string& istream)
{
	//!!!TODO: this line should be updated by compiler
	IceServant::Ptr servant = new IceServant();
	//!!!ends

	if (NULL == servant)
		return false;

	::IceInternal::BasicStream bstrm(NULL);
	size_t maxlen = istream.length();
	bstrm.resize(maxlen);

	size_t len = ZQ::common::RedisClient::decode(istream.c_str(), bstrm.b.begin(), maxlen);
	bstrm.resize(len);

	__read(&bstrm, servant); // this func is generated by slice2cpp for each slice class
	so->_servant = servant;
	return true;
}

ServantPtr IceObjStore::locate(const ::Ice::Current& current, LocalObjectPtr& cookie)
{
//	DeactivateController::Guard deactivateGuard(_deactivateController);
	if (_bQuit || current.id.category != ObjectStore::category())
		return NULL;

	StoredObject::Ptr so = ObjectStore::open(current.id.name);
	cookie = so;
	return so->_servant;
}

void IceObjStore::finished(const ::Ice::Current& current, const ServantPtr& servant, const LocalObjectPtr& cookie)
{
//	DeactivateController::Guard deactivateGuard(_deactivateController);
	if (_bQuit || !cookie)
		return;

	StoredObject::Ptr so = StoredObject::Ptr::dynamicCast(cookie);
	ObjectStore::close(so, current.mode != Ice::Nonmutating);
}

void IceObjStore::deactivate(const ::std::string& reason)
{
	stop(reason);
}

// clone of interface Freeze::Evictor
::Ice::ObjectPrx IceObjStore::add(const ServantPtr& servant, const ::Ice::Identity& ident)
{
	if (_bQuit || ident.category != ObjectStore::category())
		return NULL;

	//DeactivateController::Guard deactivateGuard(_deactivateController);
	if (!ObjectStore::add(servant, ident.name))
		return NULL;

	::Ice::ObjectPrx obj = _adapter->createProxy(ident);
	return obj;
}

::Ice::ObjectPtr IceObjStore::remove(const ::Ice::Identity& ident)
{
	if (_bQuit || ident.category != ObjectStore::category())
		return NULL;

	//DeactivateController::Guard deactivateGuard(_deactivateController);
	return ::Ice::ObjectPtr::dynamicCast(ObjectStore::remove(ident.name).get());
}

bool IceObjStore::hasObject(const ::Ice::Identity& ident)
{
	if (_bQuit || ident.category != ObjectStore::category())
		return false;

	//DeactivateController::Guard deactivateGuard(_deactivateController);
	return ObjectStore::open(ident.name)? true :false;
}

ObjectIterator::Ptr IceObjStore::getIterator(size_t batchSize)
{
	if (_bQuit)
		return NULL;

	//DeactivateController::Guard deactivateGuard(_deactivateController);
	return new ObjectIterator(*this, batchSize);
}

}} // namespaces
