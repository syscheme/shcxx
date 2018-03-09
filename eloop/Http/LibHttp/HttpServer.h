#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "HttpMessage.h"
#include "eloop_net.h"
#include "HttpConnection.h"
#include <NativeThread.h>
#include <SystemUtils.h>
#include <boost/regex.hpp>
#include <set>


namespace ZQ {
namespace eloop {

#define DEFAULT_SITE "."
class HttpServer;
class HttpPassiveConn;
class ServantThread;

// ---------------------------------------
// interface HttpHandler
// ---------------------------------------
class HttpHandler: public IHttpParseSink, public virtual ZQ::common::SharedObject
{
	friend class HttpPassiveConn;
	friend class IBaseApplication;

public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<HttpHandler> Ptr;
	virtual ~HttpHandler() {}

	HttpPassiveConn& conn() { return _conn; }

	// ---------------------------------------
	// interface IBaseApplication
	// ---------------------------------------
	// present an HTTP application that respond a URI mount
	// the major funcation of the application is to instantiaze a HttpHandler
	class IBaseApplication : public ZQ::common::SharedObject
	{
	public:
		IBaseApplication(ZQ::common::Log& logger, const HttpHandler::Properties& appProps = HttpHandler::Properties())
			: _log(logger), _appProps(appProps)
		{}

		virtual ~IBaseApplication() {}

		HttpHandler::Properties getProps() const { return _appProps; }
		ZQ::common::Log& log() { return _log; }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual HttpHandler::Ptr create(HttpPassiveConn& conn, const HttpHandler::Properties& dirProps) =0;

	protected:
		HttpHandler::Properties _appProps;
		ZQ::common::Log&        _log;
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;

protected: // hatched by HttpApplication
	HttpHandler(IBaseApplication& app, HttpPassiveConn& conn, const HttpHandler::Properties& dirProps = HttpHandler::Properties())
		: _conn(conn), _app(app), _dirProps(dirProps)
	{}

	virtual void	onHttpDataSent(size_t size) {}
	virtual void	onHttpDataReceived( size_t size ) {}

	HttpPassiveConn& _conn;
	IBaseApplication& _app;
	HttpHandler::Properties _dirProps;
};

template <class Handler>
class HttpApplication: public HttpHandler::IBaseApplication
{
public:
	typedef ZQ::common::Pointer<HttpApplication> Ptr;
	typedef Handler HandlerT;

public:
	HttpApplication(ZQ::common::Log& logger, const HttpHandler::Properties& appProps = HttpHandler::Properties())
		: IBaseApplication(logger, appProps) {}
	virtual ~HttpApplication() {}

	virtual HttpHandler::Ptr create(HttpPassiveConn& conn, const HttpHandler::Properties& dirProps)
	{ 
		return new HandlerT(*this, conn, dirProps);
	}
};

//---------------------------------------
//class HttpMonitorTimer
//----------------------------------------
class HttpMonitorTimer : public Timer
{
public:
	//	~HttpMonitorTimer(){printf("~HttpMonitorTimer\n");}
	virtual void OnTimer();
	//	virtual void OnClose(){printf("HttpMonitorTimer onclose!\n");}
};

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
// present an accepted incomming connection
class HttpPassiveConn : public HttpConnection
{
public:
	HttpPassiveConn(HttpServer& server,ZQ::common::Log& logger);
	~HttpPassiveConn();

	bool			start();
	const std::string& hint() const { return _Hint; }
	void			stop();

	bool			keepAlive(){return _keepAlive_Timeout>0;}
	void 			errorResponse( int code );
	virtual void    onRespComplete();

protected:

	// implementation of HttpConnection
	virtual void	onError( int error,const char* errorDescription );
	virtual void	onHttpDataSent(size_t size);
	virtual void	onHttpDataReceived( size_t size );
	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool	onBodyData( const char* data, size_t size);
	virtual void	onMessageCompleted();
	virtual void	OnClose();

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void initHint();

private:
	HttpServer&					_server;
	HttpHandler::Ptr			_Handler;

//	typedef std::map<std::string,HttpHandler::Ptr> MapHandler;
//	MapHandler					_mapHandler;
	
	ZQ::common::Log&			_Logger;
//	bool						_HeaderComplete;
//	bool						_bError;

	bool						_keepAlive;
	int64						_keepAlive_Timeout;
	int64						_startTime;
	HttpMonitorTimer			_timer;

	std::string					_Hint;
// 	int64						_lastRespTime;
};

// ---------------------------------------
// class HTTPResponse
// ---------------------------------------
class HTTPResponse : public HttpMessage
{
public:
	typedef ZQ::common::Pointer<HTTPResponse> Ptr;

	HTTPResponse(HttpHandler::Ptr handler)
		: _handler(handler), HttpMessage(HttpMessage::MSG_RESPONSE)
	{
	}

	virtual ~HTTPResponse() {}

	void post(int statusCode) 
	{
		if (statusCode>100)
			code(statusCode);

		std::string respMsg = toRaw();
		// TODO: _conn._logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), (int)respMsg.size(), _conn.hint().c_str(),true);

		if (_handler)
			_handler->conn().write(respMsg.c_str(), respMsg.size());
	}

private:
	HttpHandler::Ptr _handler;
};

// ---------------------------------------
// class HttpServer
// ---------------------------------------
class IHttpEngine;
class HttpServer
{
public:
	enum ServerMode
	{
		SINGLE_LOOP_MODE,
		MULTIPE_LOOP_MODE,
		DEFAULT_MODE = MULTIPE_LOOP_MODE
	};

	struct HttpServerConfig
	{
		HttpServerConfig()
		{
			serverName		= "Eloop Http Server";
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
	HttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger);
	~HttpServer();

	bool startAt();
	void stop();

	// register an application to uri
	//@param uriEx - the regular expression of uri
	bool mount(const std::string& uriEx, HttpHandler::AppPtr app, const HttpHandler::Properties& props=HttpHandler::Properties(), const char* virtualSite =DEFAULT_SITE);

	HttpHandler::Ptr createHandler( const std::string& uri, HttpPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));

	void	addConn( HttpPassiveConn* servant );
	void	delConn( HttpPassiveConn* servant );

	HttpMessage::Ptr makeSimpleResponse( int code ) const;

	int64    keepAliveTimeout() const;

	void single();

private:
	HttpServerConfig		_Config;
	ZQ::common::Log&		_Logger;

	typedef struct _MountDir
	{
		std::string					uriEx;
		boost::regex				re;
		HttpHandler::AppPtr	app;
		HttpHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;

	std::set<HttpPassiveConn*> _PassiveConn;
	ZQ::common::Mutex		_connCountLock;
	SYS::SingleObject		_sysWakeUp;
	IHttpEngine*			_engine;
	bool					_isStart;
};

//------------------------------------------
//IHttpEngine
//------------------------------------------
class IHttpEngine
{
public:
	IHttpEngine(const std::string& ip,int port,ZQ::common::Log& logger,HttpServer& server)
		:_ip(ip),
		_port(port),
		_Logger(logger),
		_server(server)
	{}

	virtual ~IHttpEngine(){}

public:
	virtual bool startAt() = 0;
	virtual void stop() = 0;

	ZQ::common::Log&		_Logger;
	const std::string&		_ip;
	int						_port;
	HttpServer&				_server;

};

// ---------------------------------------
// class AsyncQuit
// ---------------------------------------
class AsyncQuit:public Async
{
public:
	virtual void OnAsync();
	virtual void OnClose();
};
// ---------------------------------------
// class SingleLoopHttpEngine
// ---------------------------------------
// Single event loop
class SingleLoopHttpEngine:public TCP,public IHttpEngine,public ZQ::common::NativeThread
{
public:
	SingleLoopHttpEngine(const std::string& ip,int port,ZQ::common::Log& logger,HttpServer& server);
	~SingleLoopHttpEngine();

public:
	virtual bool startAt();
	virtual void stop();
	virtual int run(void);
	virtual void doAccept(ElpeError status);
	virtual void OnClose();

private:
	ZQ::eloop::Loop _loop;
	AsyncQuit		_async;
};

// ---------------------------------------
// class MultipleLoopHttpEngine
// ---------------------------------------
//Multiple event loops
class MultipleLoopHttpEngine:public ZQ::common::NativeThread,public IHttpEngine
{
public:
	MultipleLoopHttpEngine(const std::string& ip,int port,ZQ::common::Log& logger,HttpServer& server);
	~MultipleLoopHttpEngine();

public:
	virtual bool startAt();
	virtual void stop();
	virtual int run(void);
	void QuitNotify(ServantThread* sev);

private:
	bool	_bRunning;
	int		_socket;
	std::vector<ServantThread*> _vecThread;
	int		_roundCount;
	int		_threadCount;
	ZQ::common::Mutex _Lock;
	int		_quitCount;
};

// ---------------------------------------
// class ServantThread
// ---------------------------------------
class ServantThread:public ZQ::common::NativeThread,public Async
{
public:
	ServantThread(HttpServer& server,MultipleLoopHttpEngine& engine,ZQ::common::Log& logger);
	~ServantThread();
	virtual int run(void);
	Loop& getLoop();
	void addSocket(int sock);
	virtual void OnAsync();
	virtual void OnClose();
	void quit();

private:
	ZQ::common::Log&			_Logger;
	Loop						*_loop;
	MultipleLoopHttpEngine&		_engine;
	HttpServer&					_server;
	std::list<int>				_ListSocket;
	ZQ::common::Mutex			_LockSocket;
	bool						_quit;
//	int					_Count;
//	ZQ::common::Mutex	_LockerCount;
};


// ---------------------------------------
// class HttpStatistics
// ---------------------------------------
class HttpStatistics 
{
public:
	HttpStatistics();
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

	enum Method
	{
		METHOD_UNKNOWN,
		METHOD_GET,
		METHOD_POST,
		METHOD_PUT,
		METHOD_DELETE,
		METHOD_MAX
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
	void addCounter(HttpMessage::HttpMethod mtd, int32 errCode, int32 latencyHeader, int32 latencyBody);
	static const char* nameOfMethod(int mtd);

private:
	RespCode	errCodeToRespCode( int32 errCode );
	CountersOfMethod& _getCounter(HttpMessage::HttpMethod mtd);

	// RPSTATUSMAP _rpStatus;
	ZQ::common::Mutex _locker;
};

extern HttpStatistics& getHttpStatistics();

} }//namespace ZQ::eloop

#endif