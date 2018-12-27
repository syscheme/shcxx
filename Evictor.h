// ============================================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai
// All Rights Reserved. Unpublished rights reserved under the copyright laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the confidential
// technology of syscheme Possession, use, duplication or dissemination of the
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
// construed as a commitment by syscheme
// --------------------------------------------------------------------------------------------
// Author: Hui Shao
// Desc  : impl an Evictor
// --------------------------------------------------------------------------------------------
// Revision History: 
// $Log: /ZQProjs/Common/Evictor.h $
// ============================================================================================
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

#define EVICTOR_DEFAULT_SIZE (200)

#define FLG_TRACE               FLAG(0)
#define FLG_SAVE_COMPLETED_ONLY FLAG(8)

// -----------------------------
// class EvictorException
// -----------------------------
class EvictorException : public Exception
{
public:
	EvictorException(int code, const std::string &what_arg) // throw()
		:Exception(what_arg), _code(code) {}

	virtual ~EvictorException() {} // throw() {}

protected:
	int _code;
};

// -----------------------------
// class Evictor
// -----------------------------
// the base Evictor is implmented as a simply in-memory dictionary
class Evictor // : public Mutex
{
public:
    
	typedef enum _Error {
		eeOK =200, 
		eeClientError =400, eeMashalError, eeNotFound=404, eeTimeout=402, 
		eeConnectErr=503, eeNoMemory =507,
	} Error;

	typedef std::vector<uint8> ByteStream;
	//typedef std::string ByteStream;

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
	// the data node maintenced in the evictor
	class Item : virtual public SharedObject, public Mutex
	{
		friend class Evictor;
	public:

		typedef ZQ::common::Pointer<Item> Ptr;
		typedef ZQ::common::Pointer<SharedObject> ObjectPtr;

		// typedef std::list<Ptr> List; // MUST be a list instead of vector

		typedef enum _State
		{
			clean      = 0,  // the element is as that of ObjectStore
			created    = 1,  // the element is created in the cache, and has not present in the ObjectStore
			modified   = 2,  // the element is modified but not yet flushed to the ObjectStore
			destroyed  = 3,  // the element is required to destroy but not yet deleted from ObjectStore
			dead       = 4   // the element should be evicted from the local cache
		} State;

		static const char* stateToString(State s);

		Ident _ident;

		typedef struct _Data
		{
            Pointer<SharedObject> servant;  // DONOT move this field
			State     status;
			int64     stampCreated;
            int64     stampLastSave;
			bool      completed; // indicate whether the object has been filled completed. Upon to the configuration
			                     // (FLG_SAVE_COMPLETED_ONLY& Evictor::_flags), some Evictor may be required to save
			                     // only completed objects in the data-storage

			_Data();
		} Data;

		Data _data;
		// int  _evictRefCount; // reference count in this evictor, such as times in _modifiedQueue

		State status() const { return _data.status; }

	public:
		virtual ~Item() {}

	private:
    public:

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

	uint16	getVerbosity() { return (ZQ::common::Log::loglevel_t)_log.getVerbosity() | (_verboseFlags<<8); }
	void    setVerbosity(uint16 verbose = (0 | ZQ::common::Log::L_ERROR)) { _log.setVerbosity(verbose & 0x0f); _verboseFlags =verbose>>8; }

	/// to limit the in-memory evictor size
	///@param size     the size of evictor, the less-used object may be evicted if the container has more than this size
	virtual void setSize(int size) { _evictorSize = size>0 ? size: EVICTOR_DEFAULT_SIZE; }

	/// get the size limitation of the evictor
	///@return the size of evictor
	virtual int getSize() const { return _evictorSize; }

	/// add an object into the evictor
	///@param obj     the object to add
	///@param ident   the unique identification of the object
	///@return the pointer to Evictor::Item wraps the object
	virtual Item::Ptr       add(const Item::ObjectPtr& obj, const Ident& ident);

	/// remove an object from the evictor
	///@param ident   the unique identification of the object
	///@return the pointer to the object
	virtual Item::ObjectPtr remove(const Ident& ident);

	/// locate an object from the evictor
	///@param ident   the unique identification of the object
	///@return the pointer to the object, NULL if not found
	virtual Item::ObjectPtr locate(const Ident& ident);

	/// mark the object has been updated and dirty
	///@param ident   the unique identification of the object
	///@param bCompleted  to indicate whether the object has been updated completed
	///@note when configuration (FLG_SAVE_COMPLETED_ONLY& Evictor::_flags) is set, the evictor only flushes the completed object into the 
	//   data-store, bCompleted=true hereby indicates the evictor that the object is ready to flush
	virtual void            setDirty(const Ident& ident, bool bCompleted =false);

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
        _StreamedObject();
        _StreamedObject(const _StreamedObject& other);
	} StreamedObject;

	typedef std::list < StreamedObject > StreamedList;
    typedef std::deque<Item::Ptr> Queue;
    typedef std::map <Ident, Item::Ptr> Map;

private:
	StreamedList _streamedList;

	Error _stream(const Item::Ptr& element, StreamedObject& streamedObj, int64 stampAsOf);
	void _evict(Item::Ptr item); 
	int  _evictBySize();

protected:
    // thread unsafe, only be called from Evictor
    void _requeue(Item::Ptr& item);
	void _queueModified(const Item::Ptr& element); 
	size_t _popModified(Queue& modifiedBatch, size_t max); // to pop a batch of modified items

protected:

	LogWrapper	_log;
//	ZQ::common::Log& _log;
	std::string _name;

	// configurations
	uint32    _flags;
	uint32    _saveSizeTrigger;
	uint32    _evictorSize;
	uint32    _batchSize;

	Map       _cache;
	IdentList _evictorList;
	Event     _event;
	Mutex     _lkEvictor;

	// tempoary await data
	Map       _modifiedMap;
	IdentList _modifiedQueue;
	Error     _lastErr;

	uint16 _verboseFlags;

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
	virtual Error loadFromStore(const std::string& key, StreamedObject& data);

	// marshal a servant object into a byte stream for saving to the object store
	virtual bool marshal(const std::string& category, const Item::Data& data, ByteStream& streamedData) { return false; }

	// unmarshal a servant object from the byte stream read from the object store
	virtual bool unmarshal(const std::string& category, Item::Data& data, const ByteStream& streamedData)  { return false; }

private: // this is a dummy example to store the data, hild class inherited from this should have its true object store
	typedef std::map <std::string, ByteStream > Dict;
	Dict _dict;
};

}} // namespace

#endif // #define __ZQ_COMMON_Evictor_H__

/*
// --------------------------------------------------------------------------------------------
// Usage Sample
// ============================================================================================

class TestData : public SharedObject
{
public:
	TestData(int64 d): _data(d){}
	int64 _data;
};

void main()
{
	ZQ::common::FileLog flog("e:\\temp\\EvictorTest.log", Log::L_DEBUG);
	ZQ::common::Evictor evictor(flog);

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