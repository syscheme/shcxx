#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "ZQ_common_conf.h"
#include "Guid.h"
#include "FileLog.h"
#include "NativeThread.h"
#include "SystemUtils.h"

#include "eloop_net.h"

#include <queue>
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

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API TCPServer;
class ZQ_ELOOP_HTTP_API TCPConnection;
class ZQ_ELOOP_HTTP_API InterruptibleLoop;
class ZQ_ELOOP_HTTP_API ServiceSocket;
class ZQ_ELOOP_HTTP_API TCPSvcLoop;

#define LOOP_HEARTBEAT_MIN      (200)   // 200msec
#define LOOP_HEARTBEAT_DEFAULT  (500)   // 500msec
#define LOOP_HEARTBEAT_MAX      (5000)  // 5sec

// ---------------------------------------
// class InterruptibleLoop
// ---------------------------------------
// Single event loop
class InterruptibleLoop : public ZQ::common::NativeThread, public ZQ::eloop::Loop, public ZQ::eloop::Interruptor
{
public:
	//	interface Osbervee
	//-------------------------------------
	class IPing
	{
	public:
		virtual std::string ident() =0;
		virtual void OnPing(bool isHeartbeat) = 0;
	};

public:
	InterruptibleLoop(int msStep, int cpuId=-1);
	virtual ~InterruptibleLoop() { quit(); }

	virtual bool start();
	void quit();

	int affinitedId() { return _cpuId; }

	// about the watched handle in the loop
	void watch(IPing* observee);
	void unwatch(const std::string& ident);
	void unwatch(IPing* obj) { if (obj) unwatch(obj->ident()); }
	void ping(const std::string& ident);
	void ping(IPing* obj) { if (obj) ping(obj->ident()); }

protected: // new entries
	virtual void OnLoopStart() {} // 		_server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "TCPSvcLoop starts affinite core[%d]"), _cpuId);
	virtual void OnLoopQuit(int ret) {} //		_server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "LoopThread quit!"));

	// the only loop entry
	virtual void poll(bool isHeartbeat=false); 

private: 
	// impl of NativeThread
	int run(void);
	// impl of Interruptor
	void OnWakedUp() { poll(false); if (_bQuit) Loop::stop(); }
	void OnDeactived();

	bool inLoop();

	friend class Heartbeat;
	// subclass Heartbeat
	//-------------------------------------
	class Heartbeat : public ZQ::eloop::Timer
	{
	public:
		Heartbeat(InterruptibleLoop& loop) : Timer(loop), _owner(loop) {}
		void OnTimer() { _owner.poll(true); if (_owner._bQuit) Timer::stop(); }

		InterruptibleLoop& _owner;
	};

	typedef std::map < std::string, IPing* > PingMap; // ident to WatchNode map
	typedef std::vector< std::string > Wakee;
	PingMap            _pingMap;
	Wakee              _wakees;
	ZQ::common::Mutex  _lkLoop;

	bool      _bQuit;
	int       _cpuId, _msHeartbeat;
	Heartbeat _heartbeat;
	ZQ::common::AtomicInt _loopDepth;
};

// ---------------------------------------
// class TCPConnection
// ---------------------------------------
// extend TCP intend to provide thread-safe send/recv by include bufferring
class TCPConnection : public TCP, public InterruptibleLoop::IPing 
{
public:
	TCPConnection(InterruptibleLoop& loop, ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL);
	virtual ~TCPConnection();

	bool disconnect(bool isShutdown = false); // used named stop()
	std::string ident() { return _connId; } // impl of IPing // const std::string& connId() const { return _connId; }
	const char* linkstr() const { return _linkstr.c_str(); }

	virtual bool isPassive() const { return NULL != _tcpServer; }
	void suspendReceiving(bool suspend=true) { if (suspend) read_stop(); else read_start(); }

	uint lastCSeq();

	int enqueueSend(const std::string& msg) { return enqueueSend((const uint8*) msg.c_str(), msg.length()); }
	int enqueueSend(const uint8* data, size_t len);

public: // tempraorly public // overwrite of TCP
	virtual void OnConnected(ElpeError status);

protected: // overwrite of TCP
	// called after buffer has been read from the stream
	virtual void OnRead(ssize_t nread, const char *buf) {} // TODO: uv_buf_t is unacceptable to appear here, must take a new manner known in this C++ wrapper level
	// called after buffer has been written into the stream
	virtual void OnWrote(int status);

	virtual void OnDeactived();

// new entry points introduced
// ------------------------------
public:
	virtual void OnConnectionError(int error, const char* errorDescription ) {}

protected:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void _sendNext(size_t maxlen =16*1024);
	int  _enqueueSend(const uint8* data, size_t len); // thread-unsafe methods

	void OnShutdown(ElpeError status);

	// impl of IPing
	virtual void OnPing(bool isHeartbeat); // void OnSendEnqueued();

public:
	ZQ::common::Log&		_logger;
	static	uint			_enableHexDump;

protected:
	InterruptibleLoop&      _intLoop;
	TCPServer*				_tcpServer;

	std::string				_linkstr;
	bool					_isConnected;
	std::string				_connId;
	ZQ::common::AtomicInt   _lastCSeq;

	class Buffer: public ZQ::common::SharedObject, protected std::string 
	{
	public:
		typedef ZQ::common::Pointer <Buffer> Ptr;
		typedef std::queue <Ptr> Queue;

		Buffer(const uint8* data, size_t len) : std::string((const char*)data, len) {}
		const uint8* data() const { return (const uint8*) std::string::c_str(); }
		size_t len() const { return std::string::size(); }
	};

	ZQ::common::Mutex      _lkSend;
	Buffer::Queue          _queSend;
	int64                  _stampBusySend;

	std::string _peerIp, _localIp;
	int _peerPort, _localPort;

private: // TCPConnection stop export the following method in order to keep thread safe
	int write(const eloop_buf_t bufs[],unsigned int nbufs,Handle *send_handle = NULL) { return TCP::write(bufs, nbufs, send_handle); }
//	int write(const char *buf, size_t length);
	int write(const char *buf, size_t length,Handle *send_handle = NULL)  { return TCP::write(buf, length, send_handle); }
	int try_write(const char *buf, size_t length) { return TCP::try_write(buf, length); }
};

// ---------------------------------------
// class TCPServer
// ---------------------------------------
class TCPServer : public InterruptibleLoop::IPing
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
		uint		procTimeout;
		uint		maxPendings;
		uint		keepalive_timeout;
		uint		keepAliveIdleMin;
		uint		keepAliveIdleMax;
		uint		maxConns;
		uint		watchDogInterval;
	};

public:
	TCPServer(const ServerConfig& conf, ZQ::common::Log& logger)
	: _config(conf), _logger(logger), _soService(NULL) // ,_isStart(false),_isOnStart(false)
	{
#ifdef ZQ_OS_LINUX
		//Ignore SIGPIPE signal
		::signal(SIGPIPE, SIG_IGN);
#endif
	}

	virtual bool start();
	virtual bool stop();

	// impl of IPing
	virtual std::string ident();
	virtual void OnPing(bool isHeartbeat);

	// virtual bool onStart(ZQ::eloop::Loop& loop) { return true; }
	// virtual bool onStop(){ return true; }

	TCPConnection*	findConn( const std::string& connId);
	void	addConn(TCPConnection* conn);
	void	delConn(TCPConnection* conn);

	int keepAliveTimeout() const { return _config.keepalive_timeout; }

	void onLoopThreadStart(InterruptibleLoop& loop);
	// void signal();
	virtual TCPConnection* createPassiveConn(InterruptibleLoop& loop) { return new TCPConnection(loop, _logger, NULL, this); }

	ServerConfig		_config;
	ZQ::common::Log&	_logger;

private:

	friend class ServiceSocket;
	friend class TCPSvcLoop;
	ServiceSocket* _soService;

	typedef std::map<std::string, TCPConnection*>	ConnMAP;
	ConnMAP				 _connMap;
	std::vector<TCPSvcLoop* > _thrdLoops;
	size_t               _idxLoop;
	ZQ::common::Mutex	 _connCountLock;
};

} }//namespace ZQ::eloop

#endif
