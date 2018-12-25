#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "ZQ_common_conf.h"
#include "Guid.h"
<<<<<<< HEAD
#include "FileLog.h"
#include "NativeThread.h"
#include "SystemUtils.h"

#include "eloop_net.h"

#include <queue>
#include <map>
#include <string>

=======

#include "FileLog.h"
#include "eloop_net.h"
#include <NativeThread.h>
#include <SystemUtils.h>
#include <set>
#include <string>

#undef max
#include <boost/regex.hpp>

>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
// ---------------------------------------
// class TCPConnection
// ---------------------------------------
// extend TCP intend to provide thread-safe send/recv by include bufferring
class TCPConnection : public TCP, public WatchDog::IObservee 
{
public:
	TCPConnection(ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL);
	virtual ~TCPConnection() {}

	int init(Loop &loop);

	//// the access to TCP is mostly protected, but we do need some to export
	//Loop& get_loop() { return TCP::get_loop(); }
	//int fileno(fd_t *fd) { return TCP::fileno(fd); }
	//void getlocaleIpPort(char* ip,int& port) { TCP::getlocaleIpPort(ip, port); }
	//int getpeername(struct sockaddr *name, int *namelen) { return TCP::getpeername(name, namelen); }
	//void getpeerIpPort(char* ip, int& port) { TCP::getpeerIpPort(ip, port); }

	// bool start();
	bool disconnect(bool isShutdown = false); // used named stop()
	const std::string& connId() const { return _connId; }
	const char* linkstr() const { return _linkstr.c_str(); }

	virtual bool isPassive() const { return NULL != _tcpServer; }
	void suspendReceiving(bool suspend=true) { if (suspend) read_stop(); else read_start(); }

	uint lastCSeq();

	int enqueueSend(const std::string& msg) { return enqueueSend((const uint8*) msg.c_str(), msg.length()); }
	int enqueueSend(const uint8* data, size_t len);
=======
class AsyncTCPSender;
// ---------------------------------------
// class TCPConnection
// ---------------------------------------
class TCPConnection : public TCP, public WatchDog::IObservee 
{
	friend class AsyncTCPSender;

public:
	TCPConnection(ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL)
		:_logger(log), _isConnected(false), _async(NULL), _tcpServer(tcpServer), _watchDog(NULL),_isShutdown(false),_isStop(false)
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
			return _reverseHint;
		return _Hint; 
	}
	virtual bool isPassive() const { return NULL != _tcpServer; }
	uint lastCSeq();

	int AsyncSend(const std::string& msg);

	virtual void onError( int error,const char* errorDescription ) {}
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	void setWatchDog(WatchDog* watchDog)	{ _watchDog = watchDog; }
	virtual void OnTimer() {}
	virtual void OnUnwatch() {}

<<<<<<< HEAD
public: // tempraorly public // overwrite of TCP
	virtual void OnConnected(ElpeError status);

protected: // overwrite of TCP
	// called after buffer has been read from the stream
	virtual void OnRead(ssize_t nread, const char *buf) {} // TODO: uv_buf_t is unacceptable to appear here, must take a new manner known in this C++ wrapper level
	// called after buffer has been written into the stream
	virtual void OnWrote(int status);

// new entry points introduced
// ------------------------------
public:
	virtual void OnConnectionError(int error, const char* errorDescription ) {}

protected:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void _sendNext(size_t maxlen =16*1024);
	int  _enqueueSend(const uint8* data, size_t len); // thread-unsafe methods

	void OnClose();
	void OnShutdown(ElpeError status);

private:
	// subclass AsyncSender
	// ------------------------------------------------
	class WakeUp : public ZQ::eloop::Async
	{
	public:
		WakeUp(TCPConnection& conn):_conn(conn) {}

	protected:
		virtual void OnAsync() {_conn.OnSendEnqueued();}
		//virtual void OnClose() {} // to avoid trigger Handle 'delete this'
		TCPConnection& _conn;
	};

	virtual void OnSendEnqueued();
	// void OnCloseAsync();
=======
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
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

public:
	ZQ::common::Log&		_logger;
	TCPServer*				_tcpServer;
	static	uint			_enableHexDump;

protected:
	WatchDog*			    _watchDog;
<<<<<<< HEAD
	std::string				_linkstr;
	bool					_isConnected;
//	bool					_isShutdown;
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
	WakeUp*				   _pWakeup;
	//WakeUp				   _wakeup;
	int64                  _stampBusySend;

	std::string _peerIp, _localIp;
	int _peerPort, _localPort;

private: // TCPConnection stop export the following method in order to keep thread safe
	int write(const eloop_buf_t bufs[],unsigned int nbufs,Handle *send_handle = NULL) { return TCP::write(bufs, nbufs, send_handle); }
//	int write(const char *buf, size_t length);
	int write(const char *buf, size_t length,Handle *send_handle = NULL)  { return TCP::write(buf, length, send_handle); }
	int try_write(const char *buf, size_t length) { return TCP::try_write(buf, length); }
=======
	std::string				_Hint;
	std::string				_reverseHint;
	bool					_isConnected;
	bool					_isShutdown;
	bool					_isStop;
	std::string				_connId;
	ZQ::common::AtomicInt _lastCSeq;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<std::string> _sendMsgList;
	AsyncTCPSender* _async;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
		uint		procTimeout;
		uint		maxPendings;
		uint		keepalive_timeout;
		uint		keepAliveIdleMin;
		uint		keepAliveIdleMax;
		uint		maxConns;
		uint		watchDogInterval;
=======
		uint64		procTimeout;
		uint64		maxPendings;
		uint64		keepalive_timeout;
		uint64		keepAliveIdleMin;
		uint64		keepAliveIdleMax;
		uint64		maxConns;
		uint64		watchDogInterval;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
	virtual bool start();
	virtual bool stop();
=======
	bool start();
	bool stop();
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	virtual void OnTimer() {}
	virtual void OnUnwatch() {}

	virtual bool onStart(ZQ::eloop::Loop& loop){ return true; }
	virtual bool onStop(){ return true; }

	void	addConn( TCPConnection* servant );
	void	delConn( TCPConnection* servant );
	TCPConnection*	findConn( const std::string& connId);

<<<<<<< HEAD
	int keepAliveTimeout() const { return _config.keepalive_timeout; }
=======
	int64 keepAliveTimeout() const
	{
		return _config.keepalive_timeout;
	}
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

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

<<<<<<< HEAD
#endif
=======
#endif
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
