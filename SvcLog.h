#pragma once

#include "log.h"

#include <tchar.h>
#include <windows.h>

namespace ZQ {
namespace common {

class ZQ_COMMON_API CSvcLog;

class CSvcLog: public Log {
public:

	enum {
		MAX_QUEUE_SIZE = 128
	};

	enum {
		BEEP_INIT_FAILED,
	};

	CSvcLog();
	virtual ~CSvcLog();

#ifdef UNICODE
	virtual void log(loglevel_t level, const TCHAR* fmt, ...);
#else
	virtual void log(loglevel_t level, const TCHAR* fmt, ...) PRINTFLIKE(3, 4);
#endif // UNICODE

	virtual void log0(loglevel_t level, const TCHAR* str);

	virtual unsigned int getMaxSize();
	bool setMaxSize(unsigned int size);

	bool init(const TCHAR* fileName, loglevel_t level, 
		unsigned int fileSize);
	void uninit();
	
	void beep(int type);

	void flush();

	void setAppName(const TCHAR* name)
	{
		lstrcpyn(_appName, name, sizeof(_appName) - 1);
		_appName[sizeof(_appName) - 1] = 0;
	}

	const TCHAR* getAppName()
	{
		return _appName;
	}

protected:
	
	virtual void writeMessage(const TCHAR* str, int level);
	void _writeLog(const TCHAR* str);
	void writeHdr();

	static unsigned long __stdcall _logThreadProc(void* param);
	unsigned long run();
	void pushLog(loglevel_t level, const TCHAR* str);
	void writeLog(loglevel_t level, const TCHAR* str);

	bool _popItem(TCHAR* str);

	void quit();

	static const TCHAR* getLevelName(int level);

protected:
	size_t				_maxLogSize;
	size_t				_hdrLen;
	// unsigned int		_index;
	CRITICAL_SECTION	_logCritSect;
	CRITICAL_SECTION	_logQueueCritSect;
	HANDLE				_logFile;
	HANDLE				_logEvent;
	HANDLE				_logThread;
	HANDLE				_logSem;
	bool				_quit;
	TCHAR				_logStrings[MAX_QUEUE_SIZE][1024];
	size_t				_queueSize;
	TCHAR				_appName[64];	
};

} // namespace ZQ {
} // namespace common {
