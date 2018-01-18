#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__


#include "FileLog.h"
#include "eloop_net.h"
#include <NativeThread.h>
#include <SystemUtils.h>
#include <boost/regex.hpp>
#include <set>
#include <string>

namespace ZQ {
namespace eloop {

class ITCPEngine;
class TCPServer;
// ---------------------------------------
// class TCPConnection
// ---------------------------------------
class TCPConnection : public TCP
{
public:
	TCPConnection(ZQ::common::Log& log, TCPServer* tcpServer = NULL):_Logger(log),_tcpServer(tcpServer){}

	bool start();
	bool stop();
	const std::string& hint() const { return _Hint; }

	virtual bool onStart(){return true;}
	virtual bool onStop(){return true;}

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void initHint();

	virtual void OnClose();
	virtual void OnShutdown(ElpeError status);

protected:
	ZQ::common::Log&		_Logger;
	TCPServer*				_tcpServer;
	std::string				_Hint;
};


// ---------------------------------------
// class TCPServer
// ---------------------------------------
class TCPServer
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

	int64 keepAliveTimeout() const
	{
		return _Config.keepalive_timeout;
	}

	virtual TCPConnection* createPassiveConn();

	void single();

public:
	ServerConfig		_Config;

protected:
	ZQ::common::Log&		_Logger;

private:
	typedef std::set<TCPConnection*>	ConnList;

	ConnList				_PassiveConn;
	ZQ::common::Mutex		_connCountLock;
	SYS::SingleObject		_sysWakeUp;
	ITCPEngine*				_engine;
	bool					_isStart;
};

} }//namespace ZQ::eloop
#endif