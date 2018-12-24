// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: ObjectStore.h,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define ObjectStore over Redis
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ObjectDB/ObjectStore.h $
// 
// 2     5/28/15 7:36p Hui.shao
// ===========================================================================

#ifndef __ZQ_ObjectDB_ObjectStore_H__
#define __ZQ_ObjectDB_ObjectStore_H__

#include "RedisClient.h"
#include "FileLog.h"
#include "NativeThreadPool.h"
#include "Exception.h"

#include <string>
#include <list>

namespace ZQ {
namespace ObjectDB {

#define POINTER     ZQ::common::Pointer       // should be ::IceUtil::Handle for Ice
#define SOBJECT     ZQ::common::SharedObject  // should be ::Ice::Object for Ice
#define SLOCALOBJ   ZQ::common::SharedObject  // should be ::Ice::LocalObject for Ice

// typedef std::vector < Ice::Identity > Identities;
typedef std::vector < std::string >   StringList;
typedef StringList Identities;

typedef SOBJECT  ObjectServant; // should be ::Ice::Object for Ice
typedef POINTER< ObjectServant > ServantPtr; // should be Ice::ObjectPtr for Ice
typedef POINTER< SLOCALOBJ > LocalObjectPtr; 

// -----------------------------
// class Index
// -----------------------------
// index hirechy format in redis::SET : Index::KEY_PREFFIX "<IndexName>@<category>?<key>"=[ident1,ident2...]
class Index : public virtual SOBJECT
{
public:
	typedef POINTER< Index > Ptr;
	typedef std::vector < Ptr > List;
    
	virtual ~Index() {}

	const std::string& name() const { return _name; }

protected:
	Index(const std::string& name) : _name(name) { _keyPrefix =std::string("#") + _name + "@"; }

	// read the index value of a given object servant, called by _cbIndexAssociated()
	//@return key the returning would be a mashalled string key of the index
	//@note refer to IndexI::secondaryKeyCreate() 
	virtual std::string accessKey(const ZQ::ObjectDB::ServantPtr& servant) const { return ""; }

    Identities untypedFindFirst(const std::string& key, size_t firstN) const;
    Identities untypedFind(const std::string& key) const;
    size_t untypedCount(const std::string& key) const;

private:

    friend class ObjectStore;
	std::string _indexKey(const std::string& untypedValue, const std::string& category="") const;

	//@return the mashalled index value
	static std::string _cbIndexAssociated(Index& idx, ServantPtr& servant);
    
    std::string  _name, _keyPrefix;
	ObjectStore* _pStore;

	// when the index updating has problem, the idx could be dirty
	std::map <std::string, int> _dirties;
	int64        _stampLastAudit;
};

// -----------------------------
// class ObjectIterator
// -----------------------------
// object hirechy format in redis: "O<identname>@<category>"="<mashal-ed value>"
class ObjectIterator : public virtual SOBJECT
{
public:
	typedef POINTER< ObjectIterator > Ptr;

	ObjectIterator(ObjectStore& store, size_t batchSize);

	virtual bool hasNext();
	virtual std::string next();

private:

	Identities::const_iterator _nextBatch();
    ObjectStore& _store;

	Identities _batch;
    size_t     _batchSize;
    Identities::const_iterator _itBatch;

    bool _bEndOfDB;
    bool _initialized;
};

// -----------------------------
// class StoredObject
// -----------------------------
class StoredObject : public virtual SLOCALOBJ
{
public:
	typedef enum _SOState
    {
      sos_Created =0,
      sos_Clean,
      sos_Modified,
      sos_Destroyed,
    } SOState;

	typedef POINTER< StoredObject > Ptr;
	typedef std::list< Ptr > List;
	typedef std::map< std::string, Ptr > Map;
    
    StoredObject(ObjectStore& store);
    virtual ~StoredObject();

    // Immutable
    ObjectStore&   _store;

	std::string    _id;
	ServantPtr     _servant;

	int64          _stampCreated;
	int64          _stampSaved;
    
    // Protected by mutex
	ZQ::common::Mutex _mutex;
	// std::string    _valueInDB;
	StringList       _indexValues;
	
    uint8            _status;
	int              _cOpened;
};

// -----------------------------
// Exception ObjectStoreException
// -----------------------------
class ObjectStoreException : public ZQ::common::Exception
{
public:
	ObjectStoreException(const std::string& what_arg) throw()
		 : Exception(what_arg) {}
	virtual ~ObjectStoreException() throw() {}
};

// -----------------------------
// Exception AlreadyExists
// -----------------------------
class AlreadyExists : public ObjectStoreException
{
public:
	AlreadyExists(const std::string& what_arg) throw()
		 : ObjectStoreException(what_arg) {}
};

// -----------------------------
// Exception NotFound
// -----------------------------
class NotFound : public ObjectStoreException
{
public:
	NotFound(const std::string& what_arg) throw() 
		: ObjectStoreException(what_arg) {}
};

// -----------------------------
// Exception BadRecord
// -----------------------------
class BadRecord : public ObjectStoreException
{
public:
	BadRecord(const std::string& what_arg) throw() 
		: ObjectStoreException(what_arg) {}
};

#define OBJ_ACCESS_FLG_MUTABLE  FLAG(0)

// -----------------------------
// class ObjectStore
// -----------------------------
// refer to ObjectStore.cpp, EvictorI.cpp(Not supports facet)
// key format in redis: 
//    a) primary key: StoredObject::KEY_PREFFIX "<identname>@<category>"
//    b) facet:       StoredObject::KEY_PREFFIX "<identname>@<cateory>:<facetname>"
class ObjectStore : virtual public SLOCALOBJ, public ZQ::common::NativeThread //, virtual public ServantLocator
{
public:
	typedef POINTER< ObjectStore > Ptr;

    ObjectStore(const std::string& category, ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdpool, const std::string& dbUrl, const Index::List& indices =Index::List());
    virtual ~ObjectStore();

	std::string category() const { return _category; }
	ObjectIterator::Ptr getIterator(size_t batchSize=10);

	void stop(const std::string& reason);
	
	ZQ::common::RedisClient::Ptr getClient() { return _redisClient; }

protected: // impl of NativeThread as the watchdog/saver
	virtual bool init(void)	{ return true; }

	virtual int run(void);
	virtual void final(void);

public:
	virtual StoredObject::Ptr add(const ServantPtr& servant, const std::string& id);
	virtual ServantPtr remove(const std::string& id);
	virtual void close(StoredObject::Ptr& so, bool accessedMutably = true);
	virtual ServantPtr locate(const std::string& id, StoredObject::Ptr& cookie)
	{
		StoredObject::Ptr so = open(id);
		cookie = so;
		return so ? so->_servant : NULL;
	}

protected: 
	virtual size_t flush();
	virtual size_t auditIdx(size_t cMax=2);

	virtual StoredObject::Ptr open(const std::string& id, bool bCreateIfNotExist=false);

protected: // thread-unsafe utitlities

public: // marshal/unmarshal the StoredObject of this store
	virtual bool marshal(const StoredObject::Ptr& so, std::string& ostream);
	virtual bool unmarshal(StoredObject::Ptr& so, const std::string& istream);

protected:
	friend class Index;
	friend class ObjectIterator;

	std::string _objectKey(const std::string& id) const;

    std::string _category;
	bool        _bQuit;

	ZQ::common::RedisClient::Ptr  _redisClient;
	ZQ::common::Log&              _log;
	ZQ::common::NativeThreadPool& _thrdpool;
	std::string                   _dbUrl;

    Index::List        _indices;
	size_t             _idxLastAudit;

	StoredObject::Map  _openedObjs;
	StoredObject::List _dirtyObjs;
	ZQ::common::Mutex  _lock;
};

}} // namespaces

#endif // __ZQ_ObjectDB_ObjectStore_H__

