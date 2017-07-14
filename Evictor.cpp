#include "Evictor.h"
#include "TimeUtil.h"

using namespace ZQ::common;
// namespace ZQ{
// namespace common{

#define FLG_TRACE FLAG(0)

#define TRACE (_flags & FLG_TRACE)

// -----------------------------
// class EvictorItem
// -----------------------------
const char* EvictorItem::identToString(const Ident& ident) { return (ident.name + "@" + ident.category).c_str(); }
const char* EvictorItem::stateToString(State s)
{
	return "UNKNOWN";
}

// -----------------------------
// class Evictor
// -----------------------------
	/*
	Evictor(Log& log, Properties& props, uint size)
	: _maxSize(size), _size(0),
	_initializer(initializer),
	_dbEnv(SharedDbEnv::get(_communicator, envName, dbEnv)),
	_filename(filename),
	_createDb(createDb),
	TRACE(0),
	_txTrace(0),
	_pingObject(new PingObject)
	{

	TRACE = _communicator->getProperties()->getPropertyAsInt("Freeze.Trace.Evictor");
	_txTrace = _communicator->getProperties()->getPropertyAsInt("Freeze.Trace.Transaction");
	_deadlockWarning = (_communicator->getProperties()->getPropertyAsInt("Freeze.Warn.Deadlocks") != 0);
	_useNonmutating = (_communicator->getProperties()->getPropertyAsInt("Freeze.Evictor.UseNonmutating") != 0);

	string propertyPrefix = string("Freeze.Evictor.") + envName + '.' + _filename;

	// By default, we save every minute or when the size of the modified queue
	// reaches 10.
	_saveSizeTrigger = _communicator->getProperties()->
	getPropertyAsIntWithDefault(propertyPrefix + ".SaveSizeTrigger", 10);

	Int savePeriod = _communicator->getProperties()->
	getPropertyAsIntWithDefault(propertyPrefix + ".SavePeriod", 60 * 1000);

	_savePeriod = IceUtil::Time::milliSeconds(savePeriod);

	//
	// By default, we save at most 10 * SaveSizeTrigger objects per transaction
	//
	_maxTxSize = _communicator->getProperties()->
	getPropertyAsIntWithDefault(propertyPrefix + ".MaxTxSize", 10 * _saveSizeTrigger);

	if (_maxTxSize <= 0)
	{
	_maxTxSize = 100;
	}

	bool populateEmptyIndices =
	(_communicator->getProperties()->
	getPropertyAsIntWithDefault(propertyPrefix + ".PopulateEmptyIndices", 0) != 0);

	//
	// Instantiate all Dbs in 2 steps:
	// (1) iterate over the indices and create ObjectStore with indices
	// (2) open ObjectStores without indices
	//

	vector<string> dbs = allDbs();

	//
	// Add default db in case it's not there
	//
	dbs.push_back(defaultDb);


	for (vector<IndexPtr>::const_iterator i = indices.begin(); i != indices.end(); ++i)
	{
	string facet = (*i)->facet();

	StoreMap::iterator q = _storeMap.find(facet);
	if (q == _storeMap.end())
	{
	//
	// New db
	//

	vector<IndexPtr> storeIndices;

	for (vector<IndexPtr>::const_iterator r = i; r != indices.end(); ++r)
	{
	if ((*r)->facet() == facet)
	{
	storeIndices.push_back(*r);
	}
	}
	ObjectStore* store = new ObjectStore(facet, _createDb, this, storeIndices, populateEmptyIndices);
	_storeMap.insert(StoreMap::value_type(facet, store));
	}
	}


	for (vector<string>::iterator p = dbs.begin(); p != dbs.end(); ++p)
	{
	string facet = *p;
	if (facet == defaultDb)
	{
	facet = "";
	}

	pair<StoreMap::iterator, bool> ir =
	_storeMap.insert(StoreMap::value_type(facet, 0));

	if (ir.second)
	{
	ir.first->second = new ObjectStore(facet, _createDb, this);
	}
	}
	}
	*/

EvictorItem::Ptr Evictor::pin(const EvictorItem::Ident& ident, EvictorItem::Ptr element)
{
	for (int i=0; i < 5; i++)
	{
		if (i>0)
			Sleep(500 *i);

		{
			MutexGuard g(*this);
			Cache::iterator itCache = _cache.find(ident);
			if (_cache.end() != itCache)
				return itCache->second; // already in the cache
		}

		StreamedObject strmedObj;
		switch(loadFromStore(ident, strmedObj))
		{
		case eeNotFound:
			break;

		case eeOK:
			if (!element)
				element = new Item(*this, ident);
			if (unmarshal(ident.category, element->_data, strmedObj.data))
			{
				element->_data.status = Item::clean;
				break;
			}

			// _log error
			// TODO: clean this object from the DB
			element->_data.status = EvictorItem::dead;
			break;

		case eeTimeout:
		default:
			// _log error and retry
			continue;
		}
	}

	MutexGuard g(*this);
	Cache::iterator itCache = _cache.find(ident);
	if (_cache.end() != itCache)
		return itCache->second; // already in the cache

	if (NULL == element)
		return NULL;

	element->_ident = ident;
	_cache.insert(Cache::value_type(ident, element));
	_evictorList.push_front(ident);
	element->_pos = _evictorList.begin();
	element->_orphan = false;
	return element;
}

// add a new object under the management of evictor
//@return the EvictorItem that hosts the object just added
EvictorItem::Ptr Evictor::add(const EvictorItem::ObjectPtr& obj, const EvictorItem::Ident& ident)
{
	// DeactivateController::Guard deactivateGuard(_deactivateController);
	bool alreadyThere = false;
	EvictorItem::Ptr element = NULL;

	// Create a new entry
		element = new Item(*this, ident);
		element->_data.status = EvictorItem::dead;

		EvictorItem::Ptr oldElt = pin(ident, element); // load the element if exist, otherwise add this new element into

		if (!oldElt)
			element = oldElt;

		bool bNeedRequeue =false;
		{
			MutexGuard lk(*element);
			switch (element->_data.status)
			{
			case EvictorItem::clean:
			case EvictorItem::created:
			case EvictorItem::modified:
				alreadyThere = true;
				break;

			case EvictorItem::destroyed:
				element->_data.status = Item::modified;
				element->_data.servant = obj;
				bNeedRequeue = true;
				// No need to push it on the modified queue, as a destroyed object
				// is either already on the queue or about to be saved. When saved,
				// it becomes dead.
				break;

			case EvictorItem::dead:
				element->_data.status = EvictorItem::created;
				element->_data.servant = obj;
				element->_data.stampCreated = now();
				element->_data.stampLastSave = 0;
				// element->_data.avgSaveTime = 0;
				bNeedRequeue = true;

				{
					MutexGuard g(*this);
					_queueModified(element); // this is only be called while element is locked
				}

				break;

			default:
				break;
			}

			if (bNeedRequeue)
			{
				MutexGuard g(*this);
				_requeue(element); // this is only be called while element is locked
			}
		}

	if (alreadyThere)
		throw EvictorException(409, "add() existing object");

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "added object[%s]"), EvictorItem::identToString(ident));

	return element;
}

// remove a new object from the management of evictor
//@return the original object
Evictor::Item::ObjectPtr Evictor::remove(const EvictorItem::Ident& ident)
{
	EvictorItem::ObjectPtr servant;

	Item::Ptr element = pin(ident);
	if (NULL == element || NULL == element->_data.servant)
		return NULL;

	bool bNeedRequeue =false;
	MutexGuard lock(*element);

	switch (element->_data.status)
	{
	case EvictorItem::clean:
		servant = element->_data.servant;
		element->_data.status = EvictorItem::destroyed;
		element->_data.servant = NULL;
				bNeedRequeue = true;
		{
			MutexGuard g(*this);
			_queueModified(element); // this is only be called while element is locked
		}
		break;

	case EvictorItem::created:
		servant = element->_data.servant;
		element->_data.status = EvictorItem::dead;
		element->_data.servant = NULL;
				bNeedRequeue = true;
		break;

	case EvictorItem::modified:
		servant = element->_data.servant;
		element->_data.status = EvictorItem::destroyed;
		element->_data.servant = NULL;
				bNeedRequeue = true;

		// Not necessary to push it on the modified queue, as a modified
		// element is either on the queue already or about to be saved
		// (at which point it becomes clean)
		break;

	case EvictorItem::destroyed:
	case EvictorItem::dead:
	default:
		break;
	}

	if (bNeedRequeue)
	{
		MutexGuard g(*this);
		_requeue(element); // this is only be called while element is locked
	}

			if (NULL == servant)
		throw EvictorException(404, "remove() object not found");

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "removed object[%s]"), EvictorItem::identToString(ident));

	return servant;
}

Evictor::Item::ObjectPtr Evictor::locate(const Evictor::Item::Ident& ident)
{
	EvictorItem::Ptr element = pin(ident);
	if (NULL == element || NULL == element->_data.servant)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s] not found"), Item::identToString(ident));

		return NULL;
	}

	MutexGuard sync(*element);
	if(element->_data.status == Item::destroyed || element->_data.status == Item::dead)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s] in cache, but state[%s]"), Item::identToString(ident), Item::stateToString(element->_data.status));

		return NULL;
	}

	{
		MutexGuard g(*this);
		_requeue(element); // this is only be called while element is locked
	}
	// It's a good one!
	if(TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "locate() object[%s]state[%s] in cache/DB"), Item::identToString(ident), Item::stateToString(element->_data.status));

	return element->_data.servant;
}

void Evictor::setDirty(const EvictorItem::Ident& ident)
{
	EvictorItem::Ptr element = pin(ident);
	if (NULL == element || NULL == element->_data.servant)
	{
		if(TRACE)
			_log(Log::L_DEBUG, CLOGFMT(Evictor, "setDirty() object[%s] not found"), Item::identToString(ident));

		return;
	}

	MutexGuard sync(*element);
	if(element->_data.status == Item::clean)
	{
		// Assume this operation updated the object
		element->_data.status = Item::modified;
		element->_requeue();
		MutexGuard g(*this);
		_requeue(element); // this is only be called while element is locked
	}
}

//@return steps ever performed, 0-means idle/nothing done but wait
int Evictor::poll()
{
	Queue allObjects, deadObjects;

	size_t saveNowThreadsSize = 0;
	uint nStep =0;

	{
		MutexGuard g(*this);
		size_t evictSize = _evictorList.size();
		size_t cacheSize = _cache.size();
		if (evictSize != cacheSize)
		{
			_log(Log::L_ERROR, CLOGFMT(Evictor, "size[%d/%d] mismatched"), evictSize, cacheSize);
			// TODO: rebuild the _evictorList
		}

		// while (!_savingThreadDone && (_saveNowThreads.size() == 0)
		//	&& (_saveSizeTrigger < 0 || static_cast<Int>(_modifiedQueue.size()) < _saveSizeTrigger))
		//	return 0;

		// 	saveNowThreadsSize = _saveNowThreads.size();

		// Check first if there is something to do!
		if (_modifiedQueue.empty())
			return nStep;

		_modifiedQueue.swap(allObjects);
	}

	int64 stampStart = now();
	nStep++;

	size_t size = allObjects.size();
	std::deque<StreamedObject> streamedObjectQueue;

	// Stream each element
	for (size_t i = 0; i < size; i++)
	{
		EvictorItem::Ptr& element = allObjects[i];
		EvictorItem::ObjectPtr servant = NULL;

		if (!element)
			continue;

		// These elements can't be stale as only elements with 
		// usageCount == 0 can become stale, and the modifiedQueue
		// (us now) owns one count.
		MutexGuard gElem(*element);
		switch (element->_data.status)
		{
		case EvictorItem::created:
		case EvictorItem::modified:
			servant = element->_data.servant;
			break;

		case EvictorItem::destroyed:
			element->_data.status = EvictorItem::dead;
			deadObjects.push_back(element);
			break;

		case EvictorItem::dead:
			deadObjects.push_back(element);
			break;

		default:
			// Nothing to do (could be a duplicate)
			break;
		}

		if (NULL == servant)
			continue; // for the next element

		// stream the element per its state by calling mashal() 
		StreamedObject streamedObj;
		if (!_stream(element, streamedObj, stampStart))
		{
			_log(Log::L_ERROR, CLOGFMT(Evictor, "failed to stream element[%s](%s)"), EvictorItem::identToString(element->_ident), EvictorItem::stateToString(element->_data.status));
			continue;
		}

		MutexGuard sync(*this);
		_streamedList.push_back(streamedObj);
	} // for-loop

	allObjects.clear(); // to reduce the item->_usageInEvictor()

	int64 stampNow = now();
	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "streamed %d modified to %d, took %dmsec"), size, _streamedList.size(), (int) (stampNow - stampStart));

	// Now let's save all these streamed objects to disk using a transaction
	// Each time we get a deadlock, we reduce the number of objects to save
	// per transaction
	nStep++;
	stampStart = stampNow;
	stampNow = now();

	StreamedList batch;
	{
		MutexGuard g(*this);

		size_t batchSize = 20;
		if (batchSize > _streamedList.size())
			batchSize = _streamedList.size();

		StreamedList::iterator itE = _streamedList.begin();
		for (; batchSize>0; batchSize--)
			batch.push_back(*itE);

		_streamedList.erase(_streamedList.begin(), itE);
	}

	size = _streamedList.size();
	int c = saveBatchToStore(batch);

	if (TRACE)
		_log(Log::L_DEBUG, CLOGFMT(Evictor, "flushed %d of %d streamed objects, took %dmsec"), c, size, (int) (now() - stampStart));

	// do evicting
	{
		ZQ::common::MutexGuard sync(*this);
		// step 1. about those dead objects
		for (Queue::iterator q = deadObjects.begin(); q != deadObjects.end(); q++)
		{
			EvictorItem::Ptr& element = *q;
			if (element->_orphan)
				continue;

			MutexGuard g(*element);
			// expect one in _cache and another in deadObjects, if more, it could in in the new _modifiedQueue
			// someone must have re-used and modified this element again
			if (element->status() != EvictorItem::dead || element->_usageInEvictor() >2)
				continue; 

			element->_evict();
		}

		deadObjects.clear();

		// step 2. evict the oldest object that not used recently
		_evictBySize();
	}

	return ++nStep;
}

void Evictor::_queueModified(const EvictorItem::Ptr& element) // thread unsafe
{
	_modifiedQueue.push_back(element);

	if(_saveSizeTrigger >= 0 && _modifiedQueue.size() >= _saveSizeTrigger)
		_event.signal(); // notify alll
}

bool Evictor::_stream(const Item::Ptr& element, StreamedObject& streamedObj, int64 stampAsOf)
{
	streamedObj.status = element->_data.status;
	streamedObj.key = Item::identToString(element->_ident);
	streamedObj.stampAsOf = (stampAsOf >0) ? stampAsOf: now();

	if (streamedObj.status == Item::destroyed)
		return true; // no need to mashal further data

	if (element->_data.stampCreated <=0)
		element->_data.stampCreated = streamedObj.stampAsOf;

	// Update stats first
	int64 diff = streamedObj.stampAsOf - (element->_data.stampCreated + element->_data.stampLastSave);
	if(element->_data.stampCreated == 0)
	{
		element->_data.stampLastSave = diff;
	}
	else
	{
		element->_data.stampCreated = streamedObj.stampAsOf - element->_data.stampCreated;
		// element->_data.avgSaveTime = (int64)(element->_data.avgSaveTime * 0.95 + diff * 0.05);
	}

	return marshal(element->_ident.category, element->_data, streamedObj.data);
}

void Evictor::_requeue(EvictorItem::Ptr& item) // thread unsafe, only be called from Evictor
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
		_log(Log::L_ERROR, CLOGFMT(Evictor, "_evict() item[%s] pos mis-ref to the owner"), Item::identToString(item->_ident));
	else
		_evictorList.erase(item->_pos);

	_cache.erase(item->_ident);
	item->_orphan = true;
}

int Evictor::_evictBySize()
{
	size_t c =0;
	while (_evictorList.size() >0 && _evictorList.size() > _evictorSize)
	{
		Item::Ident ident = *_evictorList.rbegin();
		// Will be covered in _evict(Item::Ptr item) _evictorList.pop_back();

		Cache::iterator itCache = _cache.find(ident);
		if (_cache.end() == itCache)
		{
			_evictorList.pop_back();
			continue;
		}

		Item::Ptr& element = itCache->second;
		if (!element || element->_objectUsage() >1) // the element must be in use
			continue;

		element->_evict();
		c++;
	}

	return c;
}

bool Evictor::marshal(const std::string& category, const Item::Data& data, ByteStream& streamedData)
{
#pragma message ( __MSGLOC__ "TODO: marshal the data for saving")
	//char
	//size_t len = &data.servant - &data;

	//typedef struct _Data {
	//	State     status;
	//	int64     creationTime, lastSaveTime, avgSaveTime; // _creationTime =0, _lastSaveTime = 0, _avgSaveTime = 0;
	//	_Data() : status(clean) {  creationTime = lastSaveTime = avgSaveTime = 0; }
	//} Data;

	return true;
}

class TestData : public SharedObject
{
public:
	TestData(int64 d): _data(d){}
	int64 _data;
};

bool Evictor::unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData)
{
#pragma message ( __MSGLOC__ "TODO: unmarshal the data after loading per category as servant type")
	
	// dummy impl for test
	data.status = Item::clean;
	data.stampCreated = data.stampLastSave = now() - 3600*1000;
	data.servant = new TestData(now());

	return true;
}

int Evictor::saveBatchToStore(StreamedList& batch)
{
	int cUpdated =0, cDeleted =0;
	for (StreamedList::iterator it = batch.begin(); it!=batch.end(); it++)
	{
		switch(it->status)
		{
		case Item::created:   // the element is created in the cache, and has not present in the ObjectStore
		case Item::modified:  // the element is modified but not yet flushed to the ObjectStore
#pragma message ( __MSGLOC__ "TODO: call RedisClient to SET(key, data)")
			cUpdated++;
			break;

		case Item::destroyed:  // the element is required to destroy but not yet deleted from ObjectStore
#pragma message ( __MSGLOC__ "TODO: call RedisClient to DEL(key)")
			cDeleted++;
			break;

		default:
			break;
		}
	}

	return cUpdated+cDeleted;
}

Evictor::Error Evictor::loadFromStore(Evictor::Item::Ident ident, Evictor::StreamedObject& data)
{
#pragma message ( __MSGLOC__ "TODO: call RedisClient to read the data")
	return eeOK;
}


// }} // namespace


