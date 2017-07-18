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

namespace ZQ{
namespace common{

class ZQ_COMMON_API EvictorException;
class ZQ_COMMON_API Evictor;

#define EVICTOR_DEFAULT_SIZE (10)

// -----------------------------
// class EvictorException
// -----------------------------
class EvictorException : public Exception
{
public:
	EvictorException(int code, const std::string &what_arg) throw()
		:Exception(what_arg), _code(code) {}

	virtual ~EvictorException() throw() {}

protected:
	int _code;
};

// -----------------------------
// class Evictor
// -----------------------------
class Evictor // : public Mutex
{
public:

	typedef enum _Error {
		eeOK =200, eeNotFound=404, eeTimeout=402, eeConnectErr=503
	} Error;

	// typedef std::vector<uint8> ByteStream;
	typedef std::string ByteStream;

	typedef struct _Ident
	{
		std::string category, name;
		static int  comp(const _Ident& x, const _Ident& y) { int r= x.category.compare(y.category); return r?r:x.name.compare(y.name); }
		bool operator < (const _Ident& o) const { return (comp(*this, o) < 0); }
		bool operator == (const _Ident& o) const { return (comp(*this, o) == 0); }
	} Ident;

	typedef std::list <Ident> IdentList;

	static std::string identToStr(const Ident& ident);

	// -----------------------------
	// sub class Item
	// -----------------------------
	class Item : virtual public SharedObject, public Mutex
	{
		friend class Evictor;

	public:

		typedef ZQ::common::Pointer<Item> Ptr;
		typedef ZQ::common::Pointer<SharedObject> ObjectPtr;

		// typedef std::list<Ptr> List; // MUST be a list instead of vector

		typedef enum _State
		{
			clean = 0,     // the element is as that of ObjectStore
			created = 1,   // the element is created in the cache, and has not present in the ObjectStore
			modified = 2,  // the element is modified but not yet flushed to the ObjectStore
			destroyed = 3,  // the element is required to destroy but not yet deleted from ObjectStore
			dead = 4       // the element should be evicted from the local cache
		} State;

		static const char* stateToString(State s);

		Ident _ident;

		typedef struct _Data {
			State     status;
			int64     stampCreated, stampLastSave;
			ObjectPtr servant;

			_Data() : status(clean) {  stampCreated = stampLastSave = 0; }
		} Data;

		Data _data;
		// int  _evictRefCount; // reference count in this evictor, such as times in _modifiedQueue

		State status() const { return _data.status; }

	public:
		virtual ~Item() {}

	private:

		Item(Evictor& owner, const Ident& ident) // should only be instantized by Evictor
			: _owner(owner), _ident(ident), _orphan(true)
		{}

		// virtual void _evict(); // thread unsafe, only be called from Evictor
		int _objectUsage() { if (!_data.servant) return 0; return _data.servant->__getRef(); } 
		int _itemUsage() { return SharedObject::__getRef(); } 

		// relates to the Evictor
		Evictor& _owner;
		IdentList::iterator _pos; // NULL if it is an orphan
		bool  _orphan; // true if not managed in _owner._cache
	};

	typedef std::map < ::std::string, ::std::string> Properties;

	Evictor(Log& log, const std::string& name, const Properties& props);
	virtual ~Evictor();

	virtual void setSize(int size) { _evictorSize = size>0 ? size: EVICTOR_DEFAULT_SIZE; }
	virtual int getSize() const { return _evictorSize; }

	virtual Item::Ptr       add(const Item::ObjectPtr& obj, const Ident& ident);
	virtual Item::ObjectPtr remove(const Ident& ident);
	virtual Item::ObjectPtr locate(const Ident& ident);
	virtual void            setDirty(const Ident& ident);

	// the evictor relies on a thread to to evict/flush data
	//@return false if completely idle
	virtual bool poll(bool flushAll= false);

protected:

	typedef struct _StreamedObject
	{
		std::string key;
		uint8 status;

		ByteStream data;
		int64 stampAsOf;
		// ObjectStore* store;
	} StreamedObject;

	typedef std::list < StreamedObject > StreamedList;

private:
	StreamedList _streamedList;

	typedef std::deque<Item::Ptr> Queue;
	typedef std::map <Ident, Item::Ptr> Map;

	// thread unsafe, only be called from Evictor
	void _requeue(Item::Ptr& item);
	bool _stream(const Item::Ptr& element, StreamedObject& streamedObj, int64 stampAsOf);
	void _evict(Item::Ptr item); 
	int  _evictBySize();
	void _queueModified(const Item::Ptr& element); 
	size_t _popModified(Queue& modifiedBatch, size_t max); // to pop a batch of modified items

protected:

	Log&        _log;
	std::string _name;

	// configurations
	uint32    _flags;
	uint32    _saveSizeTrigger;
	uint32    _evictorSize;
	uint32    _batchSize;

	Map            _cache;
	IdentList        _evictorList;
	Event            _event;
	Mutex     _lkEvictor;

	// tempoary await data
	Map       _modifiedMap;
	IdentList _modifiedQueue;

	// ping an item in the mapped ObjectStore
	//@param element if non-NULL element is given, means when the object is not exists in the ObjectStore,
	//               the given element should be taken to reserve the ident at least in the layer of the local cache-layer
	//@return the object loaded from the ObjectStore, NULL if not found
	virtual Item::Ptr pin(const Ident& ident, Item::Ptr element = NULL);

protected: // the child class inherited from this evictor should implement the folloing method

	// save a batch of streamed object to the target object store
	virtual int saveBatchToStore(StreamedList& batch) { return 0; }

	// load a specified object from the object store
	//@ return IOError, NotFound, OK
	virtual Error loadFromStore(const std::string& key, StreamedObject& data) { return eeNotFound; }

	// marshal a servant object into a byte stream for saving to the object store
	virtual bool marshal(const std::string& category, const Item::Data& data, ByteStream& streamedData) { return false; }

	// unmarshal a servant object from the byte stream read from the object store
	virtual bool unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData)  { return false; }
};

}} // namespace

#endif // #define __ZQ_COMMON_Evictor_H__

/* Usage sample
class TestData : public SharedObject
{
public:
	TestData(int64 d): _data(d){}
	int64 _data;
};


class MyEvictor : public ZQ::common::Evictor
{
public:
	MyEvictor(Log& log, const Properties& props = Properties())
		: Evictor(log, std::string("MyEvictor"), props)
	{}

	virtual ~MyEvictor() {}

	typedef std::map <std::string, ByteStream > Dict;
	Dict _dict;

protected: // the child class inherited from this evictor should implement the folloing method

	// save a batch of streamed object to the target object store
	virtual int saveBatchToStore(StreamedList& batch)
	{
		int cUpdated =0, cDeleted =0;
		for (StreamedList::iterator it = batch.begin(); it!=batch.end(); it++)
		{
			switch(it->status)
			{
			case Item::created:   // the item is created in the cache, and has not present in the ObjectStore
			case Item::modified:  // the item is modified but not yet flushed to the ObjectStore
#pragma message ( __MSGLOC__ "TODO: call API to save SET(key, data)")
				_dict[it->key] = it->data;
				cUpdated++;
				break;

			case Item::destroyed:  // the item is required to destroy but not yet deleted from ObjectStore
#pragma message ( __MSGLOC__ "TODO: call RedisClient to DEL(key)")
				_dict.erase(it->key);
				cDeleted++;
				break;

			default:
				break;
			}
		}

		_log(Log::L_DEBUG, CLOGFMT(Evictor, "saveBatchToStore() %d updated and %d destroyed"), cUpdated, cDeleted);

		return cUpdated+cDeleted;
	}

	// load a specified object from the object store
	//@ return IOError, NotFound, OK
	virtual Error loadFromStore(const std::string& key, StreamedObject& data)
	{
		Dict::iterator it = _dict.find(key);
		if (_dict.end() == it)
			return eeNotFound;

		data.data = it->second;
		data.stampAsOf = ZQ::common::now();

		_log(Log::L_INFO, CLOGFMT(MyEvictor, "loadFromStore() %s loaded"), key.c_str());
		return eeOK;
	}

	// marshal a servant object into a byte stream for saving to the object store
	virtual bool marshal(const std::string& category, const Evictor::Item::Data& data, Evictor::ByteStream& streamedData)
	{
#pragma message ( __MSGLOC__ "TODO: marshal the data for saving")
		//char
		//size_t len = &data.servant - &data;

		//typedef struct _Data {
		//	State     status;
		//	int64     creationTime, lastSaveTime, avgSaveTime; // _creationTime =0, _lastSaveTime = 0, _avgSaveTime = 0;
		//	_Data() : status(clean) {  creationTime = lastSaveTime = avgSaveTime = 0; }
		//} Data;
		TestData* td = dynamic_cast<TestData*> (data.servant.get());
		if (!td)
			return false;

		char buf[100];
		snprintf(buf, sizeof(buf)-2, "s%d,c%lld,v%lld,d%lld", data.status, data.stampCreated, data.stampLastSave, td->_data);
		_log(Log::L_INFO, CLOGFMT(MyEvictor, "marshal() %s"), buf);
		streamedData.assign(buf, buf+strlen(buf));

		return true;
	}

	// unmarshal a servant object from the byte stream read from the object store
	virtual bool unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData)
	{
#pragma message ( __MSGLOC__ "TODO: unmarshal the data after loading per category as servant type")

		// dummy impl for test
		int64 d=0;
		sscanf(streamedData.c_str(), "s%d,c%lld,v%lld,d%lld", &data.status, &data.stampCreated, &data.stampLastSave, &d);

		// data.status = Item::clean;
		// data.stampCreated = data.stampLastSave = now() - 3600*1000;
		data.servant = new TestData(d);

		return true;
	}
};

void main()
{
	ZQ::common::FileLog flog("e:\\temp\\EvictorTest.log", Log::L_DEBUG);
	MyEvictor evictor(flog);

	size_t i;
	char buf[20];
	Evictor::Ident ident; ident.category="TestData";
	for (i=0; i< 50; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.add(new TestData(i), ident);
	}

	for (i=0; i< 100; i++)
		evictor.poll();

	for (i=20; i< 30; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.setDirty(ident);
	}

	for (i=5; i< 12; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.remove(ident);
	}

	for (i=0; i< 100; i++)
		evictor.poll();
}
*/