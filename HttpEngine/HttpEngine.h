#ifndef __ZQ_HttpEngine_H__
#define __ZQ_HttpEngine_H__

#include "HttpEngineInterface.h"
#include <Log.h>
#include <NativeThreadPool.h>
#include <DataCommunicatorUnite.h>
#include <Counter.h>

namespace ZQHttp
{
class RequestDispatcher;
class DialogFactory;

class EngineStatistics 
{
public:
	EngineStatistics();
	enum RespCode
	{
		RESP_OTHER,
		RESP_2XX,
		RESP_400,
		RESP_403,
		RESP_404,
		RESP_405,
		RESP_500,
		RESP_501,
		RESP_503,		
		RESP_COUNT
	};

	typedef struct _CountersOfMethod 
	{
		int32		respCount[RESP_COUNT];
		int32		totalCount;

		int32		maxLatencyInMs_Header, avgLatencyInMs_Header;
		int64		subtotalLatencyInMs_Header;

		int32		maxLatencyInMs_Body, avgLatencyInMs_Body;
		int64		subtotalLatencyInMs_Body;

	} CountersOfMethod;

	CountersOfMethod _counters[METHOD_MAX];
	int64		_mesureSince;

	void reset();
	void addCounter(Method mtd, int32 errCode, int64 latencyHeader, int64 latencyBody );
	static const char* nameOfMethod(int mtd);

	/*
	size_t	getStatistics(RPSTATUSMAP& rpStatus) const
	{
		ZQ::common::MutexGuard gd(_locker);
		//rpStatus  = _rpStatus;
		for (RPSTATUSMAP::const_iterator it = _rpStatus.begin(); it != _rpStatus.end(); ++it)
		{
            rpStatus[it->first] = it->second; 
		}

		return rpStatus.size();
	}

	int64		getMesuredSince() const
	{
		ZQ::common::MutexGuard gd(_locker);
		return _mesureSince;
	}

	void		addCounter(Method mtd, int32 errCode, int64 latencyHeader, int64 latencyBody );

	void		reset();
*/

private:
	RespCode	errCodeToRespCode( int32 errCode );

	// RPSTATUSMAP _rpStatus;
	ZQ::common::Mutex _locker;
};

extern EngineStatistics& getEngineStatistics();

class Engine
{
public:
    explicit Engine(ZQ::common::Log& log);
    ~Engine();
public:
    void setEndpoint(const std::string& host, const std::string& port = "80");
    void setCapacity(size_t nConcurrentThread, int maxPendingRequest = -1);
    void setMaxConnections(int maxConnections);
    void setIdleTimeout(int idleTimeout);
public:
    void enableMessageDump(bool textMode, bool incomingMessage, bool outgoingMessage);
    void registerHandler(const std::string& urlsyntax, IRequestHandlerFactory* handlerFac);

    bool start();
    void stop();
private:
    ZQ::common::Log& _log;
    ZQ::common::NativeThreadPool _pool;
    RequestDispatcher* _dispatcher;

    std::string _host;
    std::string _port;
    size_t _nConcurrentThread;

    ZQ::DataPostHouse::DataPostHouseEnv _env;
    ZQ::DataPostHouse::ObjectHandle<DialogFactory> _fac;
    ZQ::DataPostHouse::DataPostDak* _dak;
    ZQ::DataPostHouse::AServerSocketTcpPtr _svr;
};
} // namespace ZQHttp

#endif
