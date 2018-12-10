#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "HttpMessage.h"
#include "eloop_net.h"
#include "HttpConnection.h"
#include <NativeThread.h>
#include <SystemUtils.h>
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

	virtual void OnConnectionError( int error,const char* errorDescription ) {} /// most handlers do not care this event

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

////---------------------------------------
////class HttpMonitorTimer
////----------------------------------------
//class HttpMonitorTimer : public Timer
//{
//public:
//	//	~HttpMonitorTimer(){printf("~HttpMonitorTimer\n");}
//	virtual void OnTimer();
//	//	virtual void OnClose(){printf("HttpMonitorTimer onclose!\n");}
//};
//

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
// present an accepted incomming connection
class HttpPassiveConn : public HttpConnection
{
public:
	HttpPassiveConn(HttpServer& server);
	~HttpPassiveConn();

	bool			keepAlive() const { return _keepAlive_Timeout>0; }
	void 			errorResponse( int code );
	virtual void    onRespComplete(bool isShutdown = false);

protected:

	// implementation of HttpConnection
	virtual void	OnConnectionError( int error,const char* errorDescription );
	virtual void	onHttpDataSent(size_t size);
	virtual void	onHttpDataReceived( size_t size );
	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg);
	virtual bool	onBodyData( const char* data, size_t size);
	virtual void	onMessageCompleted();
	//virtual void	OnClose();

	virtual void OnTimer();


private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void initHint();

protected:
	HttpHandler::Ptr			_handler;
	HttpServer&					_server;

	bool						_keepAlive;
	int64						_keepAlive_Timeout;
	int64						_startTime;
};

// ---------------------------------------
// class HttpServer
// ---------------------------------------
class IHttpEngine;
class HttpServer: public TCPServer
{
public:
	HttpServer( const TCPServer::ServerConfig& conf,ZQ::common::Log& logger)
		:TCPServer(conf,logger){}

	virtual bool onStart(ZQ::eloop::Loop& loop){ return true; }
	virtual bool onStop(){ return true; }

	// register an application to uri
	//@param uriEx - the regular expression of uri
	bool mount(const std::string& uriEx, HttpHandler::AppPtr app, const HttpHandler::Properties& props=HttpHandler::Properties(), const char* virtualSite =DEFAULT_SITE);

	HttpHandler::Ptr createHandler( const std::string& uri, HttpPassiveConn& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));

	virtual TCPConnection* createPassiveConn();

private:
	typedef struct _MountDir
	{
		std::string					uriEx;
		HttpHandler::AppPtr	app;
		HttpHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;
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
	int64		_measureSince;

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