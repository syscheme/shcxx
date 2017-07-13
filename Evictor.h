#ifndef __ZQ_COMMON_Evictor_H__
#define __ZQ_COMMON_Evictor_H__

#include "ZQ_common_conf.h"
#include "Pointer.h"
#include "Locks.h"
#include "Log.h"

#include <string>
#include <map>
#include <list>
#include <queue>

using namespace ZQ::common;
// namespace ZQ{
// namespace common{

// class ZQ_COMMON_API EvictorItem;

// -----------------------------
// class EvictorItem
// -----------------------------
class EvictorItem : public SharedObject, public Mutex
{
	friend class Evictor;

public:

	typedef ZQ::common::Pointer<EvictorItem> Ptr;
	typedef ZQ::common::Pointer<SharedObject> ObjectPtr;

	typedef std::list<Ptr> List; // MUST be a list instead of vector

	typedef enum _State
	{
		clean = 0,     // the element is as that of ObjectStore
		created = 1,   // the element is created in the cache, and has not present in the ObjectStore
		modified = 2,  // the element is modified but not yet flushed to the ObjectStore
		destroyed = 3,  // the element is required to destroy but not yet deleted from ObjectStore
		dead = 4       // the element should be evicted from the local cache
	} State;

	typedef struct _Ident
	{
		std::string category, name;
		bool operator < (const _Ident& o) const { return category < o.category && name < o.name; }
	} Ident;

	static const char* identToString(const Ident& ident);
	static const char* stateToString(State s);

	Ident _ident;
	bool  _orphan; // true if not managed in _owner._cache

	typedef struct _Data {
		State     status;
		int64     stampCreated, stampLastSave;
		ObjectPtr servant;

		_Data() : status(clean) {  stampCreated = stampLastSave = 0; }
	} Data;

	Data _data;
	// int  _evictRefCount; // reference count in this evictor, such as times in _modifiedQueue

	State status() const { return _data.status; }

	// relates to the Evictor
	Evictor& _owner;
	List::iterator _pos;

public:
	virtual ~EvictorItem() {}

protected:

	EvictorItem(Evictor& owner) // should only be instantized by Evictor
		: _owner(owner), _orphan(true)
	{}
	
	virtual void _requeue(); // thread unsafe, only be called from Evictor
	virtual void _evict(); // thread unsafe, only be called from Evictor
	int _objectUsage() { if (!_data.servant) return 0; return _data.servant->__getRef(); } 
	int _usageInEvictor() { return SharedObject::__getRef(); } 

	/*

		//
		// Immutable
		//
		ObjectStore& store;

		//
		// Immutable once set
		//
		Cache::Position cachePosition;

		//
		// Protected by EvictorI
		//
		std::list<EvictorElementPtr>::iterator evictPosition;
		int usageCount;
		int keepCount;
		bool stale;

		//
		// Protected by mutex
		//
		IceUtil::Mutex mutex;
		ObjectRecord rec;
		Ice::Byte status;
		*/
};

// -----------------------------
// class EvictorException
// -----------------------------
class EvictorException : public Exception
{
public:
	EvictorException(int code, const std::string &what_arg) throw()
		:Exception(what_arg), _code(code) {}

	virtual ~EvictorException() throw();

protected:
	int _code;
};

// -----------------------------
// class Evictor
// -----------------------------
#define Key std::string // TODO: Dummy
#define Value std::string // TODO: Dummy
#define ObjectStore void // TODO: Dummy

class Evictor : public Mutex
{
	friend class EvictorItem;

public:

	typedef EvictorItem Item;
	typedef std::map < ::std::string, ::std::string> Properties;

	uint _size, _maxSize;
	Log&      _log;
	Event     _event;

	// configurations
	bool      _trace;
	uint32    _saveSizeTrigger;
	uint32    _evictorSize;

	virtual ~Evictor();

	virtual void setSize(int size);
	virtual int getSize();

	virtual Item::Ptr       add(const EvictorItem::ObjectPtr& obj, const EvictorItem::Ident& ident);
	virtual Item::ObjectPtr remove(const EvictorItem::Ident& ident);
	virtual Item::ObjectPtr locate(const EvictorItem::Ident& ident);
	virtual void            setDirty(const EvictorItem::Ident& ident);

		// ping an item in the mapped ObjectStore
	//@param element if non-NULL element is given, means when the object is not exists in the ObjectStore,
	//               the given element should be taken to reserve the ident at least in the layer of the local cache-layer
	//@return the object loaded from the ObjectStore, NULL if not found
	virtual EvictorItem::Ptr pin(const EvictorItem::Ident& ident, EvictorItem::Ptr element = NULL);

	// the evictor relies on two threads: one to evict/flush data, one to load object
	virtual int poll();
	virtual int poll_load(size_t batchSize = 10);


private:

	typedef std::vector<uint8> ByteStream;
	typedef struct _StreamedObject
	{
		std::string key;
		uint8 status;

		ByteStream data;
		int64 stampAsOf;
		// ObjectStore* store;
	} StreamedObject;

	typedef std::list < StreamedObject > StreamedList;

	StreamedList _streamedList;

	// thread unsafe, only be called from Evictor
	bool _stream(const Item::Ptr& element, StreamedObject& streamedObj, int64 stampAsOf);
	void _evict(Item::Ptr item); 
	int _evictBySize();
	void _queueModified(const Item::Ptr& element); 

protected:

	typedef std::deque<Item::Ptr> Queue;
	typedef std::map <Item::Ident, Item::Ptr > Cache;

	Cache             _cache;
	EvictorItem::List _evictorList;

	// tempoary await data
	Queue             _modifiedQueue;
	Cache             _awaitLoad;

	virtual int saveBatch(StreamedList& batch);

	//@ return IOError, NotFound, OK
	virtual Error loadFromStore(Item::Ident ident, StreamedObject& data);

	virtual bool marshal(const Item::Data& data, ByteStream& streamedData);
	virtual bool unmarshal(Item::Data& data, const ByteStream& streamedData);

};

/*
//
// Accessors for other classes
//
void saveNow();

DeactivateController& deactivateController();
const Ice::CommunicatorPtr& communicator() const;
const SharedDbEnvPtr& dbEnv() const;
const std::string& filename() const;

bool deadlockWarning() const;
Ice::Int trace() const;
Ice::Int txTrace() const;


void initialize(const Ice::Identity&, const std::string&, const Ice::ObjectPtr&);


static std::string defaultDb;
static std::string indexPrefix;

private:

	Ice::ObjectPtr locateImpl(const Ice::Current&, Ice::LocalObjectPtr&);
	bool hasFacetImpl(const Ice::Identity&, const std::string&);
	bool hasAnotherFacet(const Ice::Identity&, const std::string&);

	void evict();
	void evict(const EvictorElementPtr&);
	void _queueModified(const EvictorElementPtr&);
	void fixEvictPosition(const EvictorElementPtr&);

	void stream(const EvictorElementPtr&, Ice::Long, StreamedObject&);
	void saveNowNoSync();

	ObjectStore* findStore(const std::string&) const;

	std::vector<std::string> allDbs() const;


	typedef std::map<std::string, ObjectStore*> StoreMap;
	StoreMap _storeMap;

	//
	// The _evictorList contains a list of all objects we keep,
	// with the most recently used first.
	//
	std::list<EvictorElementPtr> _evictorList;
	std::list<EvictorElementPtr>::size_type _evictorSize;
	std::list<EvictorElementPtr>::size_type _currentEvictorSize;

	//
	// The _modifiedQueue contains a queue of all modified objects
	// Each element in the queue "owns" a usage count, to ensure the
	// element containing the pointed element remains in the cache.
	//
	std::deque<EvictorElementPtr> _modifiedQueue;

	DeactivateController _deactivateController;
	bool _savingThreadDone;
	WatchDogThreadPtr _watchDogThread;

	Ice::ObjectAdapterPtr _adapter;
	Ice::CommunicatorPtr _communicator;

	ServantInitializerPtr _initializer;

	SharedDbEnvPtr _dbEnv;

	std::string _filename;
	bool _createDb;

	Ice::Int _trace;
	Ice::Int _txTrace;

	//
	// Threads that have requested a "saveNow" and are waiting for
	// its completion
	//
	std::deque<IceUtil::ThreadControl> _saveNowThreads;

	Ice::Int _saveSizeTrigger;
	Ice::Int _maxTxSize;
	IceUtil::Time _savePeriod;

	bool _deadlockWarning;

	bool _useNonmutating;

	Ice::ObjectPtr _pingObject;
};


inline DeactivateController&
EvictorI::deactivateController()
{
	return _deactivateController;
}

inline const Ice::CommunicatorPtr&
EvictorI::communicator() const
{
	return _communicator;
}

inline const SharedDbEnvPtr&
EvictorI::dbEnv() const
{
	return _dbEnv;
}

inline bool
EvictorI::deadlockWarning() const
{
	return _deadlockWarning;
}

inline Ice::Int
EvictorI::trace() const
{
	return _trace;
}



	void evict(const EvictorItem::Ptr& item)
	{
		assert(!item->stale);
		assert(item->keepCount == 0);

		_evictorList.erase(item->evictPosition);
		_currentEvictorSize--;
		element->stale = true;
		element->store.unpin(item->cachePosition);
	}

	void Evictor::_evictBySize()
	{
		//
		// Must be called with *this locked
		//

		assert(_currentEvictorSize == _evictorList.size());

		list<EvictorElementPtr>::reverse_iterator p = _evictorList.rbegin();

		while (_currentEvictorSize > _evictorSize)
		{
			//
			// Get the last unused element from the evictor queue.
			//
			while (p != _evictorList.rend())
			{
				if ((*p)->usageCount == 0)
				{
					break; // Fine, servant is not in use (and not in the modifiedQueue)
				}
				++p;
			}
			if (p == _evictorList.rend())
			{
				//
				// All servants are active, can't evict any further.
				//
				break;
			}

			EvictorElementPtr& element = *p;
			assert(!element->stale);
			assert(element->keepCount == 0);

			if (_trace >= 2 || (_trace >= 1 && _evictorList.size() % 50 == 0))
			{
				string facet = element->store.facet();

				Trace out(_communicator->getLogger(), "Freeze.Evictor");
				out << "evicting \"" << _communicator->identityToString(element->cachePosition->first) << "\" ";
				if (facet != "")
				{
					out << "-f \"" << facet << "\" ";
				}
				out << "from the queue\n"
					<< "number of elements in the queue: " << _currentEvictorSize;
			}

			//
			// Remove last unused element from the evictor queue.
			//
			element->stale = true;
			element->store.unpin(element->cachePosition);
			p = list<EvictorElementPtr>::reverse_iterator(_evictorList.erase(element->evictPosition));
			_currentEvictorSize--;
		}
	}
*/
#endif // #define __ZQ_COMMON_Evictor_H__
