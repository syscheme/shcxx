#ifndef _SENDTHREAD_H
#define _SENDTHREAD_H

#include <NativeThreadPool.h>
#include <stdio.h>
#include <SystemUtils.h>

#include "DoSocket.h"
namespace LibAsync {
	
typedef SYS::SingleObject Event; 

class SendThread : public ZQ::common::ThreadRequest{
public:
	SendThread(DoSocketPtr pt, const char* buffer, int buflen, ZQ::common::NativeThreadPool& pool, ZQ::common::Log& log);

	virtual ~SendThread();
	void final(int retcode =0, bool bCancelled =false);

	void ready(bool err = false)
	{
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SendThread, "ready() entry."));
		if (err)
			_run = false;
		_wakeup.signal();
	}

	int run();
	int getData(int pos, char* buf, int buflen);
private:
	ZQ::common::Log&              _log;
	DoSocketPtr                   _ptr;
	std::string 				  _buffer;
	bool                          _run;
	Event						  _wakeup;
	FILE*                         _fd;
};


}
#endif