#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "FileLog.h"
#include "eloop_net.h"
#include <NativeThread.h>
#include <SystemUtils.h>
#include <boost/regex.hpp>
#include <set>
#include <string>

#include "ZQ_common_conf.h"
#include "Guid.h"

#ifdef ZQ_OS_MSWIN
#  ifdef LIBHTTP_EXPORTS
#    define ZQ_HTTP_API __EXPORT
#  else
#    define ZQ_HTTP_API __DLLRTL
#  endif
#else
#  define ZQ_HTTP_API
#endif // OS

namespace ZQ {
namespace eloop {

class ZQ_HTTP_API TCPServer;
class ZQ_HTTP_API TCPConnection;

//-------------------------------------
//	class watchDog
//-------------------------------------
class IWatchDog
{
	friend class watchDogTimer;
protected:
	virtual void OnWatchDog() = 0;
	virtual void OnWatchDogClose() = 0;
};

//-------------------------------------
//	class watchDogTimer
//-------------------------------------
class watchDogTimer : public ZQ::eloop::Timer
{
public:
	watchDogTimer(IWatchDog* dog) {_watchDogVec.push_back(dog);}

	virtual void OnTimer();
	virtual void OnClose();
	void addWatchDog(IWatchDog* whDog);
	void delWatchDog(IWatchDog* whDog);

private:
	std::vector<IWatchDog*>	_watchDogVec;
	ZQ::common::Mutex		_watchDogLock;
};


class ITCPEngine;
class TCPServer;
class AsyncTCPSender;
// ---------------------------------------
// class TCPConnection
// ---------------------------------------
class TCPConnection : public TCP, public IWatchDog 
{
	friend class AsyncTCPSender;
public:
	TCPConnection(ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL)
		:_Logger(log), _isConnected(false), _async(NULL), _tcpServer(tcpServer), _watchDog(NULL)
	{
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
	bool stop();
	const std::string& connId() const { return _connId; }
	const std::string& hint() const { return _Hint; }

	int AsyncSend(const std::string& msg);

	virtual void onError( int error,const char* errorDescription ) {}

	void setWatchDog(watchDogTimer* watchDog)	{ _watchDog = watchDog; }
	virtual void OnWatchDog() {}
	virtual void OnWatchDogClose() {}

protected:
	virtual bool onStart(){return true;}
	virtual bool onStop(){return true;}

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

protected:
	ZQ::common::Log&		_Logger;
	TCPServer*				_tcpServer;
	watchDogTimer*			_watchDog;
	std::string				_Hint;
	bool					_isConnected;
	std::string				_connId;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<std::string> _sendMsgList;
	AsyncTCPSender* _async;
};


// ---------------------------------------
// class TCPServer
// ---------------------------------------
class TCPServer//, public IWatchDog
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
			httpHeaderAvailbleTimeout = 5* 60 * 1000;
			keepalive_timeout		= -1;		//ms
			keepAliveIdleMin		= 5 * 60 * 1000;
			keepAliveIdleMax		= 10 * 60 *1000;
			maxConns 				= 100 * 1000;
			mode					= DEFAULT_MODE;
			threadCount				= 4;
			watchDogInterval		= 5;		//ms;
		}

		std::string serverName;
		std::string	host;
		int		    port;
		ServerMode	mode;
		int			threadCount;
		uint64		httpHeaderAvailbleTimeout;
		uint64		keepalive_timeout;
		uint64		keepAliveIdleMin;
		uint64		keepAliveIdleMax;
		uint64		maxConns;
		uint64		watchDogInterval;
	};

public:
	TCPServer( const ServerConfig& conf,ZQ::common::Log& logger)
	:_Config(conf),
	_Logger(logger),
	_isStart(false)
	{
#ifdef ZQ_OS_LINUX
		//Ignore SIGPIPE signal
		signal(SIGPIPE, SIG_IGN);
#endif
	}

	bool start();
	bool stop();

	virtual bool onStart(){ return true; }
	virtual bool onStop(){ return true; }


	void	addConn( TCPConnection* servant );
	void	delConn( TCPConnection* servant );
	TCPConnection*	findConn( const std::string& connId);

	int64 keepAliveTimeout() const
	{
		return _Config.keepalive_timeout;
	}

	void single();
	virtual TCPConnection* createPassiveConn();

	ServerConfig		_Config;
	ZQ::common::Log&	_Logger;

private:
	typedef std::map<std::string, TCPConnection*>	ConnMAP;

	ConnMAP					_PassiveConn;
	ZQ::common::Mutex		_connCountLock;
	SYS::SingleObject		_sysWakeUp;
	ITCPEngine*				_engine;
	bool					_isStart;
};

} }//namespace ZQ::eloop
#endif