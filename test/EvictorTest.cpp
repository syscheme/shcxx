#include "../Evictor.h"
#include "../FileLog.h"
#include "../TimeUtil.h"

using namespace ZQ::common;

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

	virtual ~MyEvictor() { poll(true); }

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

		_log(Log::L_DEBUG, CLOGFMT(MyEvictor, "saveBatchToStore() %d updated and %d destroyed, store size %d"), cUpdated, cDeleted, _dict.size());

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

		_log(Log::L_INFO, CLOGFMT(MyEvictor, "loadFromStore() %s loaded: %s"), key.c_str(), data.data.c_str());
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

	evictor.poll();
	evictor.poll();

	for (i=10; i< 30; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.setDirty(ident);
	}

	evictor.poll();

	for (i=15; i< 21; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.remove(ident);
	}
		evictor.poll();


	for (i=40; i< 50; i++)
	{
		snprintf(buf, sizeof(buf)-2, "%04d", i); ident.name =buf;
		evictor.setDirty(ident);
	}

	//for (i=0; i< 100; i++)
	//	evictor.poll();
}