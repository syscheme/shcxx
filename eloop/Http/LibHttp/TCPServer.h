#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "ZQ_common_conf.h"
#include "Guid.h"
#include "FileLog.h"
#include "NativeThread.h"
#include "SystemUtils.h"

#include "eloop_net.h"

#include <list>
#include <map>
#include <string>

#ifdef ZQ_OS_LINUX
#include <signal.h>
#endif

#ifdef ZQ_OS_MSWIN
#  ifdef LIBHTTP_EXPORTS
#    define ZQ_ELOOP_HTTP_API __EXPORT
#  else
#    define ZQ_ELOOP_HTTP_API __DLLRTL
#  endif
#  ifdef _LIB
#    undef  ZQ_ELOOP_HTTP_API
#    define ZQ_ELOOP_HTTP_API
#  endif // _LIB
#else
#  define ZQ_ELOOP_HTTP_API
#endif // OS

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API TCPServer;
class ZQ_ELOOP_HTTP_API TCPConnection;
class ZQ_ELOOP_HTTP_API WatchDog;

//-------------------------------------
//	class WatchDog
//-------------------------------------
class WatchDog : public ZQ::eloop::Timer
{
public:
	//-------------------------------------
	//	interface Osbervee
	//-------------------------------------
	class IObservee
	{
	public:
		virtual void OnTimer() = 0;
		virtual void OnUnwatch() = 0;
	};

public:
	WatchDog() {}
	// WatchDog(IObservee* dog) { _observeeList.push_back(dog); }

	void watch(IObservee* observee);
	void unwatch(IObservee* observee);

protected: // impl of ZQ::eloop::Timer to redirect the OnTimer to _observeeList
	virtual void OnTimer();
	virtual void OnClose();

private:
	std::vector<IObservee*>	_observeeList;
	ZQ::common::Mutex		_observeeLock;
};

class ITCPEngine;
class AsyncTCPSender;
// ---------------------------------------
// class TCPConnection
// ---------------------------------------
class TCPConnection : public TCP, public WatchDog::IObservee 
{
	friend class AsyncTCPSender;

public:
	TCPConnection(ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL)
		:_logger(log), _isConnected(false), _async(NULL), _tcpServer(tcpServer), _watchDog(NULL),_isShutdown(false),_isStop(false),
		_peerPort(0), _localPort(0)
	{
		_lastCSeq.set(1);
		if (connId != NULL)
			_connId = connId;
		else
		{
			char buf[80];
			ZQ::common::Guid guid;
			guid.create();
			guid.toCompactIdstr(buf, sizeof(buf) -2);
			_connId = buf;
		}
	}

	int init(Loop &loop);
	bool start();
	bool stop(bool isShutdown = false);
	const std::string& connId() const { return _connId; }
	const std::string& hint() const 
	{
		if (!_tcpServer)
			return _descReverse;
		return _desc; 
	}
	virtual bool isPassive() const { return NULL != _tcpServer; }
	uint lastCSeq();

	int AsyncSend(const std::string& msg);

	virtual void onError( int error,const char* errorDescription ) {}

	void setWatchDog(WatchDog* watchDog)	{ _watchDog = watchDog; }
	virtual void OnTimer() {}
	virtual void OnUnwatch() {}

protected:
	virtual bool onStart() {return true;}
	virtual bool onStop()  { delete this; return true;}

	// called after buffer has been written into the stream
	virtual void OnWrote(int status) {}
	// called after buffer has been read from the stream
	
	virtual void OnRead(ssize_t nread, const char *buf) {} // TODO: uv_buf_t is unacceptable to appear here, must take a new manner known in this C++ wrapper level

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void initHint();

	virtual void OnClose();
	virtual void OnShutdown(ElpeError status);

	void OnAsyncSend();
	void OnCloseAsync();

public:
	ZQ::common::Log&		_logger;
	TCPServer*				_tcpServer;
	static	uint			_enableHexDump;

protected:
	WatchDog*			    _watchDog;
	std::string				_desc;
	std::string				_descReverse;
	bool					_isConnected;
	bool					_isShutdown;
	bool					_isStop;
	std::string				_connId;
	ZQ::common::AtomicInt _lastCSeq;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<std::string> _sendMsgList;
	AsyncTCPSender* _async;

	std::string _peerIp, _localIp;
	int _peerPort, _localPort;
};

// ---------------------------------------
// class TCPServer
// ---------------------------------------
class TCPServer : public WatchDog::IObservee
{
public:
	enum ServerMode
	{
		SINGLE_LOOP_MODE,
		MULTIPE_LOOP_MODE,
		DEFAULT_MODE = MULTIPE_LOOP_MODE
	};

	struct ServerConfig
	{
		ServerConfig()
		{
			serverName		= "Eloop TCP Server";
			host			= "127.0.0.1";
			port			= 8888;
			procTimeout				= 5000;
			maxPendings				= 1000;
			keepalive_timeout		= -1;		//ms
			keepAliveIdleMin		= 5 * 60 * 1000;
			keepAliveIdleMax		= 10 * 60 *1000;
			maxConns 				= 100 * 1000;
			mode					= DEFAULT_MODE;
			threadCount				= 4;
			cpuIds					="1";
			watchDogInterval		= 500;		//ms;
		}

		std::string cpuIds;
		std::string serverName;
		std::string	host;
		int		    port;
		ServerMode	mode;
		int			threadCount;
		uint64		procTimeout;
		uint64		maxPendings;
		uint64		keepalive_timeout;
		uint64		keepAliveIdleMin;
		uint64		keepAliveIdleMax;
		uint64		maxConns;
		uint64		watchDogInterval;
	};

public:
	TCPServer(const ServerConfig& conf, ZQ::common::Log& logger)
	: _config(conf), _logger(logger),_isStart(false),_isOnStart(false)
	{
#ifdef ZQ_OS_LINUX
		//Ignore SIGPIPE signal
		::signal(SIGPIPE, SIG_IGN);
#endif
	}

	virtual bool start();
	virtual bool stop();

	virtual void OnTimer() {}
	virtual void OnUnwatch() {}

	virtual bool onStart(ZQ::eloop::Loop& loop){ return true; }
	virtual bool onStop(){ return true; }

	void	addConn( TCPConnection* servant );
	void	delConn( TCPConnection* servant );
	TCPConnection*	findConn( const std::string& connId);

	int64 keepAliveTimeout() const
	{
		return _config.keepalive_timeout;
	}

	void onLoopThreadStart(ZQ::eloop::Loop& loop);
	void signal();
	virtual TCPConnection* createPassiveConn();

	ServerConfig		_config;
	ZQ::common::Log&	_logger;

private:
	typedef std::map<std::string, TCPConnection*>	ConnMAP;

	ConnMAP					_connMap;
	ZQ::common::Mutex		_connCountLock;
	SYS::SingleObject		_sysWakeUp;
	ITCPEngine*				_engine;
	bool					_isStart;
	bool					_isOnStart;
	ZQ::common::Mutex		_onStartLock;
};

} }//namespace ZQ::eloop

#endif