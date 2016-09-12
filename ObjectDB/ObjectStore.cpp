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
// Ident : $Id: ObjectStore.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl ObjectStore over Redis
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/ObjectDB/ObjectStore.cpp $
// 
// 2     5/28/15 7:36p Hui.shao
// ===========================================================================

#include "ObjectStore.h"

namespace ZQ {
namespace ObjectDB {

// -----------------------------
// class Index
// -----------------------------
//static 
std::string Index::_cbIndexAssociated(Index& idx, ServantPtr& servant)
{
	return idx.accessKey(servant);
}

std::string Index::_indexKey(const std::string& untypedValue, const std::string& category) const
{
	// idx set "I<indexname>@<category>?<so.indexValues[i]>"
	return _keyPrefix + ((category.length() >0 || NULL == _pStore) ? category : _pStore->category()) + "?" + untypedValue;
}

Identities Index::untypedFindFirst(const std::string& key, size_t firstN) const
{
	Identities ids = untypedFind(key);
	if (firstN >0 && firstN < ids.size())
		ids.erase(ids.begin()+firstN, ids.end());
	return ids;
}

Identities Index::untypedFind(const std::string& key) const
{
	Identities ids;
	if (NULL == _pStore)
		return ids;

	// step 3. read the idx set "I<indexname>@<category>?<so.indexValues[i]>"
	std::string ikey = _indexKey(key);
	if (ZQ::common::RedisSink::rdeOK != _pStore->getClient()->SMEMBERS(ikey, ids))
		_pStore->_log(ZQ::common::Log::L_ERROR, CLOGFMT(Index, "untypedFind() failed to find SET[%s]"), ikey.c_str());

	return ids;
}

size_t Index::untypedCount(const std::string& key) const
{
	Identities ids = untypedFind(key);
	return ids.size();
}
    
// -----------------------------
// class ObjectIterator
// -----------------------------
// object hirechy format in redis: "<identname>@<category>"="<mashal-ed value>"
ObjectIterator::ObjectIterator(ObjectStore& store, size_t batchSize)
	: _store(store), _batchSize(batchSize), _bEndOfDB(false)
{
	_itBatch = _batch.end();
}

bool ObjectIterator::hasNext()
{
	if(_itBatch != _batch.end()) 
		return true;

	_itBatch = _nextBatch();
	return (_itBatch != _batch.end());
}

std::string ObjectIterator::next()
{
	if (hasNext())
		return *_itBatch++;

	ZQ::common::_throw< NotFound > ("%s(%d) end of iteration", __FILE__, __LINE__);
	return "";
}

Identities::const_iterator ObjectIterator::_nextBatch()
{
	std::string keyFrom;
	
	// determine the last key of the previous batch
	if (_batch.size() <=0)
		keyFrom = _store._objectKey(""); // initialize with the first possible key of the db
	else
	{
		Identities::iterator itLast = _batch.end();
		keyFrom = *(--itLast);
	}

	_batch.clear();
	if(_bEndOfDB)
		return _batch.end();

	StringList keys;

#pragma message ( __MSGLOC__ "TODO: read the KEYS of ObjectStore PAGE by PAGE")
	if (ZQ::common::RedisSink::rdeOK == _store._redisClient->KEYS(std::string("!*@") + _store.category(), keys))
	{
		// chop the ids from keys
		std::string suffix = std::string("@") +  _store.category();
		for (StringList::iterator it =keys.begin(); it<keys.end(); it++)
		{
			size_t pos = (*it).find(suffix);
			if (std::string::npos == pos || pos <=1)
				continue;

			_batch.push_back((*it).substr(1, pos-1));
		}

		std::sort(_batch.begin(), _batch.end());
		_bEndOfDB = true;
	}

	return _batch.begin();
}

// -----------------------------
// class StoredObject
// -----------------------------
StoredObject::StoredObject(ObjectStore& store)
: _store(store)
{
}

StoredObject::~StoredObject()
{
}

// -----------------------------
// class ObjectStore
// -----------------------------
ObjectStore::ObjectStore(const std::string& category, ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdpool, const std::string& dbUrl, const Index::List& indices)
  : _log(log), _thrdpool(thrdpool), _category(category), _indices(indices), _dbUrl(dbUrl), _bQuit(false)
{
	_redisClient = new ZQ::common::RedisClient(_log, _thrdpool, "localhost");
	for (size_t i=0; i < _indices.size(); i++)
	{
		if (NULL == _indices[i])
			continue;

		_indices[i]->_pStore = this;
	}
}

ObjectStore::~ObjectStore()
{
	stop("");
}

std::string ObjectStore::_objectKey(const std::string& id) const
{
	return std::string("!") + id + "@" + _category;
}

int ObjectStore::run(void)
{
	while(!_bQuit)
	{
		size_t c = flush();

		// clean up those cOpened <=0
		{
			StringList idsToClean;
			ZQ::common::MutexGuard g(_lock);
			for (StoredObject::Map::iterator it = _openedObjs.begin(); it != _openedObjs.end(); it++)
			{
				if (NULL == it->second)
				{
					idsToClean.push_back(it->first);
					continue;
				}

				ZQ::common::MutexGuard g2(it->second->_mutex);
				if (it->second->_cOpened <=0)
					idsToClean.push_back(it->first);
			}

			for (size_t i=0; i< idsToClean.size(); i++)
				_openedObjs.erase(idsToClean[i]);
		}

		::Sleep(c>0 ? 1:100);
	}

	return 0;
}

void ObjectStore::final(void)
{}

#define OBJ_REC_MAX_LEN (8192)

StoredObject::Ptr ObjectStore::open(const std::string& id, bool bCreateIfNotExist)
{
	StoredObject::Ptr so;
	{
		ZQ::common::MutexGuard g(_lock);
		StoredObject::Map::iterator it = _openedObjs.find(id);
		if (_openedObjs.end() != it)
			so = it->second;
	}

	if (so)
	{
		ZQ::common::MutexGuard g(so->_mutex);
		so->_cOpened++;
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "open() found pre-opened obj[%s], copen[%d]"), id.c_str(), so->_cOpened);
		return so;
	}

	bool bCreated = false;
	so = new StoredObject(*this);
	so->_id = id;

	std::string dbOid = _objectKey(id); 
	uint8 buf[OBJ_REC_MAX_LEN];
	uint vlen = sizeof(buf);
	if (ZQ::common::RedisSink::rdeOK == _redisClient->GET(dbOid, buf, vlen))
	{
		buf[vlen] ='\0';
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "open() found existing obj[%s] in DB, vlen[%d]: %s"), dbOid.c_str(), vlen, buf);
		if (!unmarshal(so, std::string((char*)buf, vlen)) || NULL == so->_servant)
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "open() failed to unmarshal obj[%s]: %s"), dbOid.c_str(), vlen, buf);
			ZQ::common::_throw< BadRecord > ("%s(%d) failed to unmarshal obj[%s]", __FILE__, __LINE__, dbOid.c_str());
		}

		so->_status = StoredObject::sos_Clean;

		so->_indexValues.clear();
		size_t i = 0;
		for (i=0; !_bQuit && i < _indices.size(); i++)
		{
			std::string idxV = Index::_cbIndexAssociated(*_indices[i], so->_servant);
			so->_indexValues.push_back(idxV);
		}

		so->_cOpened =0;
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "open() obj[%s] indices[%d] populated from DB"), dbOid.c_str(), i);
	}
	else if (bCreateIfNotExist)
	{
		// trusted as a new object
		so->_servant = NULL;
		so->_status = StoredObject::sos_Created;
		so->_cOpened = 0;
		so->_stampCreated = ZQ::common::now();
		bCreated = true;
	}

	if (NULL == so->_servant && !bCreateIfNotExist)
		so = NULL; // failed to open
	else
	{
		// re-search the opened map, see if the same object was opened by other threads during the above communication with DB
		ZQ::common::MutexGuard g(_lock);
		StoredObject::Map::iterator it = _openedObjs.find(id);
		if (_openedObjs.end() != it)
			so = it->second;
		else _openedObjs.insert(StoredObject::Map::value_type(so->_id, so)); 
	}
	
	if (so)
	{
		ZQ::common::MutexGuard g(so->_mutex);
		++so->_cOpened;
		_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "open() obj[%s] create[%c] => created[%c] cOpen[%d]"), dbOid.c_str(), bCreateIfNotExist?'T':'F', bCreated?'T':'F', so->_cOpened);
	}
	else
		_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "open() failed to open obj[%s] w/ create[%c]"), dbOid.c_str(), bCreateIfNotExist?'T':'F');

	return so;
}

void ObjectStore::close(StoredObject::Ptr& so, bool accessedMutably)
{
	if (!so)
		return;

	bool bFlush = false;
	int cOpened = 0;
	std::string oid;
	{
		ZQ::common::MutexGuard g(so->_mutex);
		oid = so->_id;
		if (accessedMutably)
		{
			// Assume this operation updated the object
			if (so->_status == StoredObject::sos_Clean)
			{
				so->_status = StoredObject::sos_Modified;
				bFlush = true;
			}
		}

		cOpened = --so->_cOpened;
	}

	ZQ::common::MutexGuard g(_lock);
	if (cOpened <=0)
		_openedObjs.erase(so->_id);

	if (bFlush && !_bQuit)
		_dirtyObjs.push_back(so); // let the run() to save the object into redis

	_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "close() obj[%s] closed: dirty[%c] cOpen[%d], dirtySize[%d] openSize[%d]"), oid.c_str(), bFlush?'T':'F', cOpened, _dirtyObjs.size(), _openedObjs.size());
}

void ObjectStore::stop(const ::std::string&)
{ 
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "stop()"));
	_bQuit = true; // stop this thread
	flush();
	_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "stop() executed"));
}

#define BOOL_CHAR(_B) ((_B)?'T':'F')

size_t ObjectStore::flush()
{
	StoredObject::List dirtyToFlush;
	{
		ZQ::common::MutexGuard g(_lock);
		dirtyToFlush =_dirtyObjs;
		_dirtyObjs.clear();
	}

	size_t cExpected = dirtyToFlush.size();
	if (cExpected <=0)
		return 0;

	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "flush() %d dirty objects to flush"), dirtyToFlush.size());
	int64 stampStart = ZQ::common::now();

	size_t cSaved=0, cDel=0, cRequeued=0;
	std::string savedlist, dellist;
	for (StoredObject::List::iterator it = dirtyToFlush.begin(); it != dirtyToFlush.end(); it++)
	{
		if (NULL == (*it))
			continue;

		// ZQ::common::MutexGuard g((*it)->_mutex);

		// step 1. backup so.indexValues => prevIValues;
		StringList prevValues = (*it)->_indexValues;
		std::string oid = (*it)->_id;

		std::string dbkey;

		// case 1. the object is destroyed
		if (StoredObject::sos_Destroyed == (*it)->_status)
		{
			bool bReQueued=false;
			dbkey= _objectKey(oid);
			if (ZQ::common::RedisSink::rdeOK != _redisClient->DEL(dbkey))
			{
				if (!_bQuit)
				{
					ZQ::common::MutexGuard g(_lock);
					_dirtyObjs.push_back((*it));
					bReQueued = true, cRequeued++;
				}

				_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to del obj[%s], requeued[%c]"), oid.c_str(), BOOL_CHAR(bReQueued));
				continue;
			}

			cDel++;
			dellist += oid + ",";

			int cIdxDel=0;
			for (size_t j=0; j< prevValues.size() && j< _indices.size(); j++)
			{
				// step 3. update index
				std::string idxkey = _indices[j]->_indexKey(prevValues[j]);
				if (ZQ::common::RedisSink::rdeOK != _redisClient->SREM(idxkey, oid))
					_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to remove obj[%s] from idx[%s]"), oid.c_str(), idxkey.c_str());
				else cIdxDel++;
			}

			_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "flush() obj[%s] deleted, [%d]indicies cleaned"), oid.c_str(), cIdxDel);
			continue;
		}

		// case 2. dirty object to save
		//   step 2.1. save into DB
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "flush() saving obj[%s]"), oid.c_str());
		std::string val;
		if (!marshal((*it), val))
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to mashal obj[%s]"), oid.c_str());
			continue;
		}

		dbkey= _objectKey(oid);
		if (ZQ::common::RedisSink::rdeOK != _redisClient->SET(dbkey, (const uint8*)val.c_str(), (int)val.length()))
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to save obj[%s]"), oid.c_str());
			continue;
		}

		cSaved++;
		savedlist += oid + ",";
	
		// step 2. repopulate the index mashalld value into do.indexValues[i]
		int cIdxUpdated=0, cIdxChanged=0;
		for (size_t j=0;  j< _indices.size(); j++)
		{
			std::string idxV = Index::_cbIndexAssociated(*_indices[j], (*it)->_servant);
			(*it)->_indexValues.push_back(idxV);

			// step 3. update index
			//   3a) add the id into index SET: "I<indexname>@<category>?<so.indexValues[i]>"
			std::string ikey = _indices[j]->_indexKey(idxV);
			if (ZQ::common::RedisSink::rdeOK != _redisClient->SADD(ikey, (*it)->_id))
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to add new obj[%s] from idx[%s]"), (*it)->_id.c_str(), ikey.c_str());
			
			cIdxUpdated++;

			//   3b) clean up the id from the old index SET: "I<indexname>@<category>?<prevIValues[i]>"
			if (j >= prevValues.size() || idxV == prevValues[j])
				continue;

			ikey = _indices[j]->_indexKey(prevValues[j]);
			if (ZQ::common::RedisSink::rdeOK != _redisClient->SREM(ikey, oid))
				_log(ZQ::common::Log::L_ERROR, CLOGFMT(ObjectStore, "flush() failed to remove obj[%s] from idx[%s]"), oid.c_str(), ikey.c_str());

			cIdxChanged++;
		}

		_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "flush() obj[%s] saved, [%d/%d]indicies updated"), oid.c_str(), cIdxChanged, cIdxUpdated);
	}

	_log( ((cSaved+ cDel) == cExpected) ? ZQ::common::Log::L_INFO : ZQ::common::Log::L_WARNING, CLOGFMT(ObjectStore, "flush() flushed (%d +%d)/%d objects, took %dmsec: %s; %s"), cSaved, cDel, cExpected, (int)(ZQ::common::now() - stampStart), savedlist.c_str(), dellist.c_str());
	return cSaved+ cDel;
}

size_t ObjectStore::auditIdx(size_t cMax)
{
	size_t cAudited =0;

	while (cAudited < cMax)
	{
		StringList dirtyValues;
		Index::Ptr pIdx =NULL;
		{
			ZQ::common::MutexGuard g(_lock);
			// step 1. scan for each attached index, see if it has dirties
			_idxLastAudit %= _indices.size();
			pIdx = _indices[_idxLastAudit];
			size_t i =0;
			for ( ; !_bQuit && pIdx->_dirties.size() >0 && i < (cMax -cAudited); i++)
			{
				dirtyValues.push_back(pIdx->_dirties.begin()->first);
				pIdx->_dirties.erase(pIdx->_dirties.begin());
			}
		}

		if (_bQuit || NULL == pIdx)
			break;

		if (dirtyValues.size() <=0)
		{
			// take the next round for the next index
			_idxLastAudit++;
			break;
		}

		// step 2. audit the dirty values of pIdx
		for (StringList::iterator itV = dirtyValues.begin(); itV < dirtyValues.end(); itV++)
		{
			Identities ids = pIdx->untypedFind(*itV);
			std::string ikey = pIdx->_indexKey(*itV);
			size_t cOrphan =0, cCleaned =0;
			for (size_t v=0; v <ids.size(); v++)
			{
				StoredObject::Ptr so = open(ids[v], false);
				if (NULL != so)
					continue;

				cOrphan++;

				if (ZQ::common::RedisSink::rdeOK != _redisClient->SREM(ikey, ids[v]))
					_log(ZQ::common::Log::L_WARNING, CLOGFMT(ObjectStore, "auditIdx() failed to clean orphan[%s] from idx[%s]"), ids[v].c_str(), ikey.c_str());
				else
				{
					cCleaned++;
					_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "auditIdx() cleaned orphan[%s] from idx[%s]"), ids[v].c_str(), ikey.c_str());
				}
			}

			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "auditIdx() cleaned %d/%d orphans from idx[%s]"), cCleaned, cOrphan, ikey.c_str());
		}

		cAudited++;
	}

	return cAudited;
}

StoredObject::Ptr ObjectStore::add(const ServantPtr& servant, const std::string& id)
{
	bool alreadyThere = false;
	StoredObject::Ptr so =NULL;

	do { // while(0) loop

		// open object by ident, create if not exist. (new created to make status=sos_Created)
		so = open(id, true);

		ZQ::common::MutexGuard g(so->_mutex);

		if (_bQuit)
			return NULL;

		switch(so->_status)
		{
		case StoredObject::sos_Clean:
		case StoredObject::sos_Modified:
			alreadyThere = true;
			break;

		case StoredObject::sos_Destroyed:
			so->_status = StoredObject::sos_Modified;
			so->_servant = servant;
			so->_stampCreated = ZQ::common::now();

			// No need to push it on the modified queue, as a destroyed object
			// is either already on the queue or about to be saved. When saved,
			// it becomes dead.
			break;

		case StoredObject::sos_Created: // the object is newly created
			so->_status = StoredObject::sos_Modified;
			so->_servant = servant;
			so->_stampCreated = ZQ::common::now();
			so->_stampSaved   = 0;
			// so->_avgSaveTime  = 0;

			if (!_bQuit)
			{
				ZQ::common::MutexGuard g(_lock);
				_dirtyObjs.push_back(so); // let the run() to save the object into redis
			}
			break;

		default:
			return NULL;
		}

	} while(0);

	if (alreadyThere)
		ZQ::common::_throw<AlreadyExists> ("%s(%d) id[%s]", __FILE__, __LINE__, id.c_str());

	_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "add() obj[%s] added"), id.c_str());
	return so;
}

ServantPtr ObjectStore::remove(const ::std::string& id)
{
	StoredObject::Ptr so = open(id);
	if (NULL == so)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(ObjectStore, "remove() obj[%s] not found, ignore"), id.c_str());
		return NULL;
	}

	ServantPtr servant;
	bool bDirty = false;
	{
		ZQ::common::MutexGuard g(so->_mutex);
		servant = so->_servant;
		switch(so->_status)
		{
		case StoredObject::sos_Clean:
			so->_status = StoredObject::sos_Destroyed;
			so->_servant = NULL; // release the servant
			bDirty = true;
			break;

		case StoredObject::sos_Created: // assume the object has not yet ready, keep the old state
			so->_servant = NULL;
			break;

		case StoredObject::sos_Modified:
			so->_status = StoredObject::sos_Destroyed;
			so->_servant = NULL;
			// Not necessary to push it on the modified queue, as a modified
			// element is either on the queue already or about to be saved
			// (at which point it becomes clean)
			break;

		case StoredObject::sos_Destroyed:
		default:
			//		// default must 
			//		so->_status = StoredObject::sos_Destroyed;
			break;
		}
	}

	if (bDirty && !_bQuit)
	{
		ZQ::common::MutexGuard g(_lock);
		_dirtyObjs.push_back(so); // let the run() to save the object into redis
	}

	close(so, false);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(ObjectStore, "remove() obj[%s] removed dirty[%c]"), id.c_str(), bDirty?'T':'F');
	return servant;
}

ObjectIterator::Ptr ObjectStore::getIterator(size_t batchSize)
{
	return new ObjectIterator(*this, batchSize);
}

bool ObjectStore::marshal(const StoredObject::Ptr& so, std::string& ostream)
{
#pragma message ( __MSGLOC__ "TODO: virtual ObjectStore::marshal()")
	return false;
/*
	IceInternal::InstancePtr instance = IceInternal::getInstance(communicator);
    IceInternal::BasicStream stream(instance.get());
    stream.startWriteEncaps();
    v.__write(&stream);
    stream.writePendingObjects();
    stream.endWriteEncaps();
    vector<Byte>(stream.b.begin(), stream.b.end()).swap(bytes);
*/
}

bool ObjectStore::unmarshal(StoredObject::Ptr& so, const std::string& istream)
{
#pragma message ( __MSGLOC__ "TODO: virtual ObjectStore::unmarshal()")
	return false;
}

}}
