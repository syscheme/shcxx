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

class HttpServer;
class ServantThread;
// ---------------------------------------
// interface HttpHandler
// ---------------------------------------
class HttpHandler: public IHttpParseSink, public virtual ZQ::common::SharedObject
{
	friend class HttpPassiveConn;
public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<HttpHandler> Ptr;
	virtual ~HttpHandler() { }

protected: // hatched by HttpApplication
	HttpHandler(HttpConnection& conn,ZQ::common::Log& logger, const Properties& dirProps, const Properties& appProps)
		: _conn(conn),_Logger(logger),_dirProps(dirProps), _appProps(appProps) {}
	virtual void	onHttpDataSent(){}

	virtual void	onHttpDataReceived( size_t size ){}

	HttpConnection& _conn;
	Properties _dirProps, _appProps;
	ZQ::common::Log&			_Logger;
};

// ---------------------------------------
// class HttpBaseApplication
// ---------------------------------------
// present an HTTP application that respond a URI mount
// the major funcation of the application is to instantiaze a HttpHandler
class HttpBaseApplication : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<HttpBaseApplication> Ptr;
public:
	HttpBaseApplication(const HttpHandler::Properties& appProps = HttpHandler::Properties()): _appProps(appProps) {}
	virtual ~HttpBaseApplication() {}
	// NOTE: this method may be accessed by multi threads concurrently
	virtual HttpHandler::Ptr create( HttpConnection& conn,ZQ::common::Log& logger,const HttpHandler::Properties& dirProps) { return NULL; }		
protected:
	HttpHandler::Properties _appProps;
};

template <class Handler>
class HttpApplication: public HttpBaseApplication
{
public:
	typedef ZQ::common::Pointer<HttpApplication> Ptr;
	typedef Handler HandlerT;

public:
	HttpApplication(const HttpHandler::Properties& appProps = HttpHandler::Properties()): _appProps(appProps) {}
	virtual ~HttpApplication() {}

	virtual HttpHandler::Ptr create( HttpConnection& conn,ZQ::common::Log& logger, const HttpHandler::Properties& dirProps) { return new HandlerT(conn,logger,dirProps, _appProps); }

protected:
	HttpHandler::Properties _appProps;
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

protected:

	// implementation of HttpConnection
	virtual void	onParseError( int error,const char* errorDescription );
	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );
	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool	onBodyData( const char* data, size_t size);
	virtual void	onMessageCompleted();
	virtual void	OnClose();

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void 			errorResponse( int code );
	void initHint();

private:
	HttpServer&					_server;
	HttpHandler::Ptr			_Handler;
	int64						_LastTouch;
	
	ZQ::common::Log&			_Logger;
	bool						_HeaderComplete;
	bool						_bError;

	std::string					_Hint;
};
#define DEFAULT_SITE "."

// ---------------------------------------
// class HttpServer
// ---------------------------------------
class HttpServer
{
public:
	struct HttpServerConfig
	{
		HttpServerConfig() {
			serverName		= "Eloop Http Server";
			httpHeaderAvailbleTimeout = 5* 60 * 1000;
			keepAliveIdleMin		= 5 * 60 * 1000;
			keepAliveIdleMax		= 10 * 60 *1000;
			maxConns 				= 100 * 1000;
		}

		std::string serverName;
		uint64		httpHeaderAvailbleTimeout;
		uint64		keepAliveIdleMin;
		uint64		keepAliveIdleMax;
		uint64		maxConns;
	};

public:
	HttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger);
	~HttpServer();

	virtual bool startAt( const char* ip, int port) = 0;
	virtual void stop() = 0;
	// register an application to uri
	//@param uriEx - the regular expression of uri

	bool mount(const std::string& uriEx, HttpBaseApplication::Ptr app, const HttpHandler::Properties& props=HttpHandler::Properties(), const char* virtualSite =DEFAULT_SITE);

	HttpHandler::Ptr createHandler( const std::string& uri, HttpConnection& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));


	void	addConn( HttpPassiveConn* servant );
	void	delConn( HttpPassiveConn* servant );

	HttpMessage::Ptr makeSimpleResponse( int code ) const;

protected:
	ZQ::common::Log&		_Logger;

private:
	typedef struct _MountDir
	{
		std::string					uriEx;
		boost::regex				re;
		HttpBaseApplication::Ptr	app;
		HttpHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;

	HttpServerConfig		_Config;
	std::set<HttpPassiveConn*> _PassiveConn;
	ZQ::common::Mutex		_Locker;
	SYS::SingleObject		_single;
};

// ---------------------------------------
// class SingleLoopHttpServer
// ---------------------------------------
// Single event loop
class SingleLoopHttpServer:public TCP,public HttpServer
{
public:
	SingleLoopHttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger,SYS::SingleObject& signal);
	~SingleLoopHttpServer();

public:
	virtual bool startAt( const char* ip, int port);
	virtual void stop();
	virtual void doAccept(ElpeError status);

private:
	ZQ::eloop::Loop _loop;
	SYS::SingleObject& _signal;
};

// ---------------------------------------
// class MultipleLoopHttpServer
// ---------------------------------------
//Multiple event loops
class MultipleLoopHttpServer:public HttpServer,public ZQ::common::NativeThread
{
public:
	MultipleLoopHttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger,int threadCount,SYS::SingleObject& signal);
	~MultipleLoopHttpServer();

public:
	virtual bool startAt( const char* ip, int port);
	virtual void stop();
	virtual int run(void);

private:
	bool	_bRunning;
	int		_socket;
	std::vector<ServantThread*> _vecThread;
	int		_roundCount;
	int		_threadCount;
	SYS::SingleObject& _signal;
};

// ---------------------------------------
// class ServantThread
// ---------------------------------------
class ServantThread:public ZQ::common::NativeThread,public Async
{
public:
	ServantThread(HttpServer& server,ZQ::common::Log& logger);
	~ServantThread();
	virtual int run(void);
	Loop& getLoop();
	void addSocket(int sock);
	virtual void OnAsync();
	HttpServer&			_server;
	std::list<int>    _ListSocket;
	ZQ::common::Mutex	_LockSocket;

private:
	ZQ::common::Log&	_Logger;
	Loop				*_loop;
//	int					_Count;
//	ZQ::common::Mutex	_LockerCount;
};



} }//namespace ZQ::eloop

#endif