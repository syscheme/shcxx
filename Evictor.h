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
class Evictor : public Mutex
{
public:

	typedef enum _Error {
		eeOK =200, eeNotFound=404, eeTimeout=402, eeConnectErr=503
	} Error;

	typedef struct _Ident
	{
		std::string category, name;
		bool operator < (const _Ident& o) const { return category < o.category && name < o.name; }
		bool operator == (const _Ident& o) const { return (category ==o.category) && (name == o.name); }
	} Ident;

	typedef std::list <Ident> IdentList;

	static const char* identToString(const Ident& ident);

	// -----------------------------
	// sub class Item
	// -----------------------------
	class Item : public SharedObject, public Mutex
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
		~Item() {}

	private:

		Item(Evictor& owner, const Ident& ident) // should only be instantized by Evictor
			: _owner(owner), _ident(ident), _orphan(true)
		{}

		virtual void _evict(); // thread unsafe, only be called from Evictor
		int _objectUsage() { if (!_data.servant) return 0; return _data.servant->__getRef(); } 
		int _usageInEvictor() { return SharedObject::__getRef(); } 

		// relates to the Evictor
		Evictor& _owner;
		IdentList::iterator _pos; // NULL if it is an orphan
		bool  _orphan; // true if not managed in _owner._cache
	};

	typedef std::map < ::std::string, ::std::string> Properties;

	Evictor(Log& log, const std::string& name, const Properties& props);
	virtual ~Evictor();

	virtual void setSize(int size);
	virtual int getSize();

	virtual Item::Ptr       add(const Item::ObjectPtr& obj, const Ident& ident);
	virtual Item::ObjectPtr remove(const Ident& ident);
	virtual Item::ObjectPtr locate(const Ident& ident);
	virtual void            setDirty(const Ident& ident);

	// the evictor relies on a thread to to evict/flush data
	virtual int poll();

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
	void _requeue(Item::Ptr& item);
	bool _stream(const Item::Ptr& element, StreamedObject& streamedObj, int64 stampAsOf);
	void _evict(Item::Ptr item); 
	int  _evictBySize();
	void _queueModified(const Item::Ptr& element); 

protected:

	Log&  _log;
	std::string _name;

	// configurations
	uint32    _flags;
	uint32    _saveSizeTrigger;
	uint32    _evictorSize;

	typedef std::deque<Item::Ptr> Queue;
	typedef std::map <Ident, Item::Ptr> Map;

	Map            _cache;
	IdentList        _evictorList;
	Event            _event;

	// tempoary await data
	Queue       _modifiedQueue;

	// ping an item in the mapped ObjectStore
	//@param element if non-NULL element is given, means when the object is not exists in the ObjectStore,
	//               the given element should be taken to reserve the ident at least in the layer of the local cache-layer
	//@return the object loaded from the ObjectStore, NULL if not found
	virtual Item::Ptr pin(const Ident& ident, Item::Ptr element = NULL);

protected: // the child class inherited from this evictor should implement the folloing method

	// save a batch of streamed object to the target object store
	virtual int saveBatchToStore(StreamedList& batch);

	// load a specified object from the object store
	//@ return IOError, NotFound, OK
	virtual Error loadFromStore(Ident ident, StreamedObject& data);

	// marshal a servant object into a byte stream for saving to the object store
	virtual bool marshal(const std::string& category, const Item::Data& data, ByteStream& streamedData);

	// unmarshal a servant object from the byte stream read from the object store
	virtual bool unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData);
};

}} // namespace

#endif // #define __ZQ_COMMON_Evictor_H__
