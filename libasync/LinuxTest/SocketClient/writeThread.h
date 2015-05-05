#ifndef  _WRITETHREAD_H
#define  _WRITETHREAD_H

#include <stdio.h>
#include <queue>

#include <NativeThread.h>
#include <Log.h>
#include <Locks.h>
#include <SystemUtils.h>
#include <Pointer.h>

namespace LibAsync{

typedef struct _data{
	std::string    fileName;
	std::string    data;
} DATA;

class writeThread : public ZQ::common::NativeThread, public virtual ZQ::common::SharedObject
{
public:
	writeThread(ZQ::common::Log& log);
	~writeThread();
public:

	typedef ZQ::common::Pointer<writeThread>	Ptr;
	void addData(const std::string& fileName, const std::string& data);

	virtual bool	start();
	void            stop();
protected:
	bool popData(std::string& fileName, std::string& data);
	void writeFile(std::string& fileName, std::string& data);

	virtual int			run();
private:
	typedef std::queue<DATA>  	DATAVEC;
	ZQ::common::Mutex			_dataMapLocker;
	DATAVEC                  	_dataMap;

	typedef SYS::SingleObject Event; 
	Event		        _wakeup;

	bool                _looprun;
	ZQ::common::Log&	_log;

};





}

#endif