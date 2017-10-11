// ============================================================================================
// Copyright (c) 1997, 1998 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of ZQ Interactive, Inc. Possession, use, duplication or dissemination of the
// software and media is authorized only pursuant to a valid written license from ZQ Interactive,
// Inc.
// This source was copied from shcxx, shcxx's copyright is belong to Hui Shao
//
// This software is furnished under a  license  and  may  be used and copied only in accordance
// with the terms of  such license and with the inclusion of the above copyright notice.  This
// software or any other copies thereof may not be provided or otherwise made available to any
// other person.  No title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and should not be
// construed as a commitment by ZQ Interactive, Inc.
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : impl an Evictor
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Log: /ZQProjs/Common/Evictor.cpp $
// ============================================================================================
#include "Evictor.h"
#include "TimeUtil.h"

namespace ZQ{
namespace common{

#define TRACE (_flags & FLG_TRACE)

// -----------------------------
// class Evictor::Item
// -----------------------------
const char* Evictor::Item::stateToString(State s)
{
	switch(s)
	{
	case clean:     return "clean"; // the element is as that of ObjectStore
	case created:   return "created"; // the element is created in the cache, and has not present in the ObjectStore
	case modified:  return "modified"; // the element is modified but not yet flushed to the ObjectStore
	case destroyed: return "destroyed"; // the element is required to destroy but not yet deleted from ObjectStore
	case dead:      return "dead"; // the element should be evicted from the local cache
	}

	return "unknown";
}

// -----------------------------
// class Evictor
// -----------------------------
#define ident_cstr(IDENT) Evictor::identToStr(IDENT).c_str()
std::string Evictor::identToStr(const Ident& ident)
{ 
	return (ident.name + "@" + ident.category); 
}

Evictor::Evictor(Log& log, const std::string& name, const Evictor::Properties& props)
: _log(log), _name(name), 
  _flags(1), _evictorSize(EVICTOR_DEFAULT_SIZE), _saveSizeTrigger(2), _batchSize(20)
{
	Evictor::Properties tmpprops = props;
	std::string prop_prefix = name;
	if (!prop_prefix.empty() && prop_prefix[prop_prefix.length()-1]!='.')
		prop_prefix += '.';

	if (atoi(tmpprops[prop_prefix + "trace"].c_str()) >0)
		_flags |= FLG_TRACE;

	_evictorSize = atoi(tmpprops[prop_prefix + "maxSize"].c_str());
	_saveSizeTrigger = atoi(tmpprops[prop_prefix + "saveSizeTrigger"].c_str());

	if (_evictorSize<=0)
		_evictorSize = EVICTOR_DEFAULT_SIZE;
	if (_saveSizeTrigger<=0)
		_saveSizeTrigger = 2;
}

Evictor::~Evictor()
{
	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "destruct to flush by poll(T)"));

	// poll(true); MUST be called from the child class
}


Evictor::Item::Ptr Evictor::pin(const Ident& ident, Item::Ptr item)
{
	bool bRetry = true;
	for (int i=0; bRetry && i < 5; i++)
	{
		if (i>0)
		{
			#ifdef ZQ_OS_LINUX
				usleep(500*1000*i);
			#else
				Sleep(500*i);
			#endif
		}
		{
			MutexGuard g(_lkEvictor);
			Map::iterator itCache = _cache.find(ident);
			if (_cache.end() != itCache)
				return itCache->second; // already in the cache
		}

		bRetry = false;
		StreamedObject strmedObj;
		switch(loadFromStore(identToStr(ident), strmedObj))
		{
		case eeNotFound:
			break;

		case eeOK:
			if (!item)
				item = new Item(*this, ident);
			if (unmarshal(ident.category, item->_data, strmedObj.data))
			{
				item->_data.status = Item::clean;
				break;
			}

			// _log error
			// TODO: clean this object from the DB
			item->_data.status = Evictor::Item::dead;
			break;

		case eeTimeout:
		default:
			// _log error and retry
			bRetry = true;
			continue;
		}
	}

	MutexGuard g(_lkEvictor);
	Map::iterator itCache = _cache.find(ident);
	if (_cache.end() != itCache)
		return itCache->second; // already in the cache

	if (NULL == item)
		return NULL;

	item->_ident = ident;
	_cache.insert(Map::value_type(ident, item));
	_evictorList.push_front(ident);
	item->_pos = _evictorList.begin();
	item->_orphan = false;
	return item;
}

// add a new object under the management of evictor
//@return the Evictor::Item that hosts the object just added
Evictor::Item::Ptr Evictor::add(const Evictor::Item::ObjectPtr& obj, const Ident& ident)
{
	// DeactivateController::Guard deactivateGuard(_deactivateController);
	bool alreadyThere = false;
	Evictor::Item::Ptr item = NULL;

	// Create a new entry
	item = new Item(*this, ident);
	item->_data.status = Evictor::Item::dead;

	Evictor::Item::Ptr oldElt = pin(ident, item); // load the item if exist, otherwise add this new item into

	if (!oldElt)
		item = oldElt;

	bool bNeedRequeue =false;
	{
		MutexGuard lk(*item);
		switch (item->_data.status)
		{
		case Evictor::Item::clean:
		case Evictor::Item::created:
		case Evictor::Item::modified:
			alreadyThere = true;
			break;

		case Evictor::Item::destroyed:
			item->_data.status = Item::modified;
			item->_data.servant = obj;
			bNeedRequeue = true;
			// No need to push it on the modified queue, as a destroyed object
			// is either already on the queue or about to be saved. When saved,
			// it becomes dead.
			break;

		case Evictor::Item::dead:
			item->_data.status = Evictor::Item::created;
			item->_data.servant = obj;
			item->_data.stampCreated = now();
			item->_data.stampLastSave = 0;
			// item->_data.avgSaveTime = 0;
			bNeedRequeue = true;

			{
				MutexGuard g(_lkEvictor);
				_queueModified(item); // this is only be called while item is locked
			}

			break;

		default:
			break;
		}

		if (bNeedRequeue)
		{
			MutexGuard g(_lkEvictor);
			_requeue(item); // this is only be called while item is locked
		}
	}

	if (alreadyThere)
		throw EvictorException(409, "add() existing object");

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "added object[%s](%s)"), ident_cstr(ident), Item::stateToString(item->_data.status));

	return item;
}

// remove a new object from the management of evictor
//@return the original object
Evictor::Item::ObjectPtr Evictor::remove(const Ident& ident)
{
	Evictor::Item::ObjectPtr servant;

	Item::Ptr item = pin(ident);
	if (NULL == item || NULL == item->_data.servant)
		return NULL;

	bool bNeedRequeue =false;
	MutexGuard lock(*item);

	switch (item->_data.status)
	{
	case Evictor::Item::clean:
		servant = item->_data.servant;
		item->_data.status = Evictor::Item::destroyed;
		item->_data.servant = NULL;
		bNeedRequeue = true;
		{
			MutexGuard g(_lkEvictor);
			_queueModified(item); // this is only be called while item is locked
		}
		break;

	case Evictor::Item::created:
		servant = item->_data.servant;
		item->_data.status = Evictor::Item::dead;
		item->_data.servant = NULL;
		bNeedRequeue = true;
		break;

	case Evictor::Item::modified:
		servant = item->_data.servant;
		item->_data.status = Evictor::Item::destroyed;
		item->_data.servant = NULL;
		bNeedRequeue = true;

		// Not necessary to push it on the modified queue, as a modified
		// item is either on the queue already or about to be saved
		// (at which point it becomes clean)
		break;

	case Evictor::Item::destroyed:
	case Evictor::Item::dead:
	default:
		break;
	}

	if (bNeedRequeue)
	{
		MutexGuard g(_lkEvictor);
		_requeue(item); // this is only be called while item is locked
	}

	if (NULL == servant)
		throw EvictorException(404, "remove() object not found");

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "removed object[%s]"), ident_cstr(ident));

	return servant;
}

Evictor::Item::ObjectPtr Evictor::locate(const Evictor::Ident& ident)
{
	Evictor::Item::Ptr item = pin(ident);
	if (NULL == item || NULL == item->_data.servant)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s] not found"), ident_cstr(ident));

		return NULL;
	}

	MutexGuard sync(*item);
	if(item->_data.status == Item::destroyed || item->_data.status == Item::dead)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s] in cache, but state[%s]"), ident_cstr(ident), Item::stateToString(item->_data.status));

		return NULL;
	}

	{
		MutexGuard g(_lkEvictor);
		_requeue(item); // this is only be called while item is locked
	}
	// It's a good one!
	if(TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s]state[%s] in cache/DB"), ident_cstr(ident), Item::stateToString(item->_data.status));

	return item->_data.servant;
}

void Evictor::setDirty(const Ident& ident, bool bCompleted)
{
	Evictor::Item::Ptr item = pin(ident);
	if (NULL == item || NULL == item->_data.servant)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "setDirty() object[%s] not found"), ident_cstr(ident));

		return;
	}

	MutexGuard g(_lkEvictor);
	MutexGuard sync(*item);
	if(item->_data.status == Item::clean)
	{
		// Assume this operation updated the object
		item->_data.status = Item::modified;
		item->_data.completed = bCompleted;
		_queueModified(item);
		_requeue(item); // this is only be called while item is locked

		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "setDirty() object[%s] modified"), ident_cstr(ident));
	}
}

//@return steps ever performed, 0-means idle/nothing done but wait
bool Evictor::poll(bool flushAll)
{
	Queue modifiedBatch, deadObjects, flushObjects, requeObjects;

	{
		MutexGuard g(_lkEvictor);
		size_t evictSize = _evictorList.size();
		size_t cacheSize = _cache.size();
		if (evictSize != cacheSize)
		{
			_log(Log::L_ERROR, CLOGFMT(Evictor, "evictlist[%d/%d] mismatched, rebuilding list"), evictSize, cacheSize);
			_evictorList.clear();
			for (Map::iterator itCache = _cache.begin(); itCache != _cache.end(); itCache++)
				_evictorList.push_back(itCache->first);
		}

		// Check first if there is something to do!
		if (_modifiedQueue.empty() && _streamedList.empty())
			return false;

		_popModified(modifiedBatch, flushAll ? 0: _batchSize);
	}

	int64 stampStart = now();

	size_t size = modifiedBatch.size();
	int cToStream =0;

	// Stream each item
	for (size_t i = 0; i < size; i++)
	{
		Evictor::Item::Ptr& item = modifiedBatch[i];
		Evictor::Item::ObjectPtr servant = NULL;

		if (!item)
			continue;

		// These elements can't be stale as only elements with usageCount == 0 can become stale, and 
		// the modifiedQueue (us now) owns one count.
		MutexGuard gElem(*item);
		StreamedObject streamedObj;
		bool bStreamed =false;

		switch (item->_data.status)
		{
		case Evictor::Item::created:
		case Evictor::Item::modified:

			// skip flushing those incompleted objects if the Evictor is configured to save completed-objects only
			if ((FLG_SAVE_COMPLETED_ONLY & _flags) && !item->_data.completed)
			{
				requeObjects.push_back(item);
				break;
			}

			servant = item->_data.servant;
			// stream the item per its state by calling mashal() 
			bStreamed = _stream(item, streamedObj, stampStart);
			if (!bStreamed)
				_log(Log::L_ERROR, CLOGFMT(Evictor, "failed to stream item[%s](%s)"), ident_cstr(item->_ident), Evictor::Item::stateToString(item->_data.status));
			
            flushObjects.push_back(item);
            break;

		case Evictor::Item::destroyed:
			deadObjects.push_back(item);
			bStreamed = _stream(item, streamedObj, stampStart);
			if (!bStreamed)
				_log(Log::L_ERROR, CLOGFMT(Evictor, "failed to stream item[%s](%s)"), ident_cstr(item->_ident), Evictor::Item::stateToString(item->_data.status));

			item->_data.status = Evictor::Item::dead;
			break;

		case Evictor::Item::dead:
			deadObjects.push_back(item);
			break;

		default:
			// Nothing to do (could be a duplicate)
			break;
		}

		if (!bStreamed)
			continue; // for the next item

		MutexGuard sync(_lkEvictor);
		_streamedList.push_back(streamedObj);
		cToStream++;
	} // for-loop

	modifiedBatch.clear(); // to reduce the item->_usageInEvictor()

	int cReque = requeObjects.size();
	{
		MutexGuard g(_lkEvictor);
		for (Queue::iterator q = requeObjects.begin(); q != requeObjects.end(); q++)
			_queueModified(*q);
	}
	requeObjects.clear();

	int64 stampNow = now();
	if (TRACE)
	{
		_log((size != cToStream +deadObjects.size())?Log::L_WARNING : Log::L_DEBUG, CLOGFMT(Evictor, "streamed %d to %d modified +%d dead, pending %d streamed, took %dmsec"), 
			size, cToStream, deadObjects.size(), _streamedList.size(), (int) (stampNow - stampStart));
	}

	// Now let's save all these streamed objects to disk using a transaction
	// Each time we get a deadlock, we reduce the number of objects to save per transaction
	stampStart = stampNow;
	stampNow = now();

	StreamedList batch;
	{
		MutexGuard g(_lkEvictor);

		size_t batchSize = _batchSize;
		if (flushAll || batchSize > _streamedList.size())
			batchSize = _streamedList.size();

		StreamedList::iterator itE = _streamedList.begin();
		for (; batchSize>0; batchSize--)
			batch.push_back(*itE++);

		_streamedList.erase(_streamedList.begin(), itE);
		size = _streamedList.size();
	}

	int c = saveBatchToStore(batch);

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "flushed a batch: %d of %d streamed objects, %d pending, took %dmsec"), c, batch.size(), size, (int) (now() - stampStart));

	// do flushing and evicting
	int cEvictedPerDead =0, cEvictedBySize =0, cFlushed =0;
	{
		ZQ::common::MutexGuard sync(_lkEvictor);
		// step 1. evict those dead objects
		for (Queue::iterator q = deadObjects.begin(); q != deadObjects.end(); q++)
		{
			Evictor::Item::Ptr& item = *q;
			if (item->_orphan)
				continue;

			MutexGuard g(*item);
			// expect one in _cache and another in deadObjects, if more, it could in in the new _modifiedQueue
			// someone must have re-used and modified this item again
			int objUsage = item->_objectUsage();
			int itemUsage = item->_itemUsage();
			if (item->status() != Evictor::Item::dead || itemUsage >2)
			{
				if (TRACE)
					_log(Log::L_DEBUG, CLOGFMT(Evictor, "skipping item[%s](%s) in use, usage %d/%d"), ident_cstr(item->_ident), Evictor::Item::stateToString(item->_data.status), objUsage, itemUsage);
				continue; 
			}

			_evict(item);
			cEvictedPerDead++;
			if (TRACE)
				_log(Log::L_DEBUG, CLOGFMT(Evictor, "item[%s](%s) evicted"), ident_cstr(item->_ident), Evictor::Item::stateToString(item->_data.status));
		}

		deadObjects.clear();

		// step 2. flush those dirty objects
        for (Queue::iterator q = flushObjects.begin(); q != flushObjects.end(); q++)
        {
            Evictor::Item::Ptr& item = *q;
            if (item->_orphan)
                continue;

            MutexGuard g(*item);
            
            item->_data.status = Evictor::Item::clean;
            cFlushed++;
            if (TRACE)
                _log(Log::L_DEBUG, CLOGFMT(Evictor, "item[%s](%s) flushed"), ident_cstr(item->_ident), Evictor::Item::stateToString(item->_data.status));
        }

		flushObjects.clear();

		// step 3. evict the oldest object that not used recently
		cEvictedBySize = _evictBySize();
	}

	if ((cEvictedPerDead + cEvictedBySize + cFlushed)>0 && TRACE)
		_log(Log::L_INFO, CLOGFMT(Evictor, "%d+%d objects evicted and %d objects flushed, new size %d, %d requeued"), cEvictedPerDead, cEvictedBySize, cFlushed, _evictorList.size(), cReque);

	return true;
}

void Evictor::_queueModified(const Evictor::Item::Ptr& item) // thread unsafe
{
	if (!item)
		return;

	_modifiedQueue.push_back(item->_ident);
	if (_modifiedMap.end() == _modifiedMap.find(item->_ident))
		_modifiedMap.insert(Map::value_type(item->_ident, item));
	else
		_modifiedMap[item->_ident] = item;
}

size_t Evictor::_popModified(Queue& modifiedBatch, size_t max) // thread unsafe
{
	size_t c=0;
	while (!_modifiedQueue.empty() && (max<=0 || c < max))
	{
		Ident ident = _modifiedQueue.front();
		_modifiedQueue.pop_front();

		Map::iterator itMod = _modifiedMap.find(ident);
		if (_modifiedMap.end() == itMod)
			continue;

		modifiedBatch.push_back(itMod->second);
		_modifiedMap.erase(itMod);
		c++;
	}

	return c;
}

bool Evictor::_stream(const Item::Ptr& item, StreamedObject& streamedObj, int64 stampAsOf)
{
	streamedObj.status = item->_data.status;
	streamedObj.key = ident_cstr(item->_ident);
	streamedObj.stampAsOf = (stampAsOf >0) ? stampAsOf: now();

	if (streamedObj.status == Item::destroyed)
		return true; // no need to mashal further data

	if (item->_data.stampCreated <=0)
		item->_data.stampCreated = streamedObj.stampAsOf;

	// Update stats first
	int64 diff = streamedObj.stampAsOf - (item->_data.stampCreated + item->_data.stampLastSave);
	if(item->_data.stampCreated == 0)
	{
		item->_data.stampLastSave = diff;
	}
	else
	{
		item->_data.stampLastSave = streamedObj.stampAsOf - item->_data.stampCreated;
		// item->_data.avgSaveTime = (int64)(item->_data.avgSaveTime * 0.95 + diff * 0.05);
	}

	return marshal(item->_ident.category, item->_data, streamedObj.data);
}

void Evictor::_requeue(Evictor::Item::Ptr& item) // thread unsafe, only be called from Evictor
{
	if (!item || item->_orphan)
		return;

	if ((*item->_pos) == item->_ident)
		_evictorList.erase(item->_pos);

	_evictorList.push_front(item->_ident);
	item->_pos = _evictorList.begin();
}

void Evictor::_evict(Item::Ptr item) // thread unsafe, only be called from Evictor
{
	if (!item || item->_orphan)
		return;

	if (!(*(item->_pos) == item->_ident))
		_log(Log::L_ERROR, CLOGFMT(Evictor, "_evict() item[%s] pos mis-ref to the owner"), ident_cstr(item->_ident));
	else
		_evictorList.erase(item->_pos);

	_cache.erase(item->_ident);
	item->_orphan = true;
}

int Evictor::_evictBySize()
{
	size_t c =0;
	while (_evictorList.size() > _evictorSize)
	{
		Ident ident = *_evictorList.rbegin();
		// Will be covered in _evict(Item::Ptr item) _evictorList.pop_back();

		Map::iterator itCache = _cache.find(ident);
		if (_cache.end() == itCache)
		{
			_evictorList.pop_back();
			continue;
		}

		Item::Ptr& item = itCache->second;
		if (!item)
		{
			_cache.erase(itCache);
			continue; // invalid one
		}

		int itemUsage = item->_itemUsage();
		int objUsage = item->_objectUsage();

		if (itemUsage>1 || objUsage >1)
			break; // the item must be in use, break the loop because others must newer than it

		_evict(item);
		c++;
	}

	return c;
}

//bool Evictor::marshal(const std::string& category, const Item::Data& data, ByteStream& streamedData)
//{
//#pragma message ( __MSGLOC__ "TODO: marshal the data for saving")
//	//char
//	//size_t len = &data.servant - &data;
//
//	//typedef struct _Data {
//	//	State     status;
//	//	int64     creationTime, lastSaveTime, avgSaveTime; // _creationTime =0, _lastSaveTime = 0, _avgSaveTime = 0;
//	//	_Data() : status(clean) {  creationTime = lastSaveTime = avgSaveTime = 0; }
//	//} Data;
//
//	return true;
//}
//
//class TestData : public SharedObject
//{
//public:
//	TestData(int64 d): _data(d){}
//	int64 _data;
//};
//
//bool Evictor::unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData)
//{
//#pragma message ( __MSGLOC__ "TODO: unmarshal the data after loading per category as servant type")
//
//	// dummy impl for test
//	data.status = Item::clean;
//	data.stampCreated = data.stampLastSave = now() - 3600*1000;
//	data.servant = new TestData(now());
//
//	return true;
//}
//

Evictor::Item::_Data::_Data() : status(clean), completed(false)
{  
    stampCreated = stampLastSave = 0; 
}

}} // namespace


