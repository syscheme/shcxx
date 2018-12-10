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

	uint lastCSeq();

	int enqueueSend(const std::string& msg) { return enqueueSend((const uint8*) msg.c_str(), msg.length()); }
	int enqueueSend(const uint8* data, size_t len);

	void setWatchDog(WatchDog* watchDog)	{ _watchDog = watchDog; }
	virtual void OnTimer() {}
	virtual void OnUnwatch() {}

public: // tempraorly public // overwrite of TCP
	virtual void OnConnected(ElpeError status);

protected: // overwrite of TCP
	// called after buffer has been read from the stream
	virtual void OnRead(ssize_t nread, const char *buf) {} // TODO: uv_buf_t is unacceptable to appear here, must take a new manner known in this C++ wrapper level
	virtual void OnWrote(int status);

// new entry points introduced
// ------------------------------
public:
	virtual void OnConnectionError(int error, const char* errorDescription ) {}

protected:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void _sendNext(size_t maxlen =16*1024);
	int  _enqueueSend(const uint8* data, size_t len); // thread-unsafe methods

	// called after buffer has been written into the stream
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
		virtual void OnClose() {} // to avoid trigger Handle 'delete this'
		TCPConnection& _conn;
	};

	virtual void OnSendEnqueued();
	// void OnCloseAsync();

public:
	ZQ::common::Log&		_logger;
	TCPServer*				_tcpServer;
	static	uint			_enableHexDump;

protected:
	WatchDog*			    _watchDog;
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
	WakeUp				   _wakeup;
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

	int keepAliveTimeout() const { return _config.keepalive_timeout; }

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
