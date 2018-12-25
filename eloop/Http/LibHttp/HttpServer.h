#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

<<<<<<< HEAD
#include "HttpConnection.h"
#include "eloop_net.h"

#include <NativeThread.h>
#include <SystemUtils.h>
#include <list>
=======
#include "HttpMessage.h"
#include "eloop_net.h"
#include "HttpConnection.h"
#include <NativeThread.h>
#include <SystemUtils.h>
#include <boost/regex.hpp>
#include <set>
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534


namespace ZQ {
namespace eloop {

<<<<<<< HEAD
#define DUMMY_PROCESS_TIMEOUT (60*1000) // 1min a dummy big time
#define DEFAULT_SITE "."

class ZQ_ELOOP_HTTP_API HttpServer;
class ZQ_ELOOP_HTTP_API HttpHandler;
=======
#define DEFAULT_SITE "."
class HttpServer;
class HttpPassiveConn;
class ServantThread;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

// ---------------------------------------
// interface HttpHandler
// ---------------------------------------
<<<<<<< HEAD
class HttpHandler: public virtual ZQ::common::SharedObject
{
	friend class HttpPassiveConn;
	friend class HttpServer;

public:
	// ---------------------------------------
	// class HTTP Response
	// ---------------------------------------
	class Response : public HttpMessage
	{
	public:
		typedef ZQ::common::Pointer<Response> Ptr;
		typedef std::list<Ptr>	List;

		Response(HttpServer& server, const HttpMessage::Ptr& req);

		// a generic post
		StatusCodeEx post(int statusCode, const std::string& body="", const char* errMsg = NULL); 

		// seperate steps of post
		StatusCodeEx post() { return post(_statusCode); }

		TCPConnection* getConn();
		int getTimeLeft();

		int64 declareContentLength(int64 contentLen, const char* contentType=NULL, bool chunked=false);
		StatusCodeEx pushBody(const uint8* data, size_t len, bool chunkedEnd=false);

	private:
		HttpServer& _server;
		HttpMessage::Ptr _req;
		int64 _stampPosted;
		std::string _txnId;
		int64 _bodyBytesPushed;
	};
=======
class HttpHandler: public IHttpParseSink, public virtual ZQ::common::SharedObject
{
	friend class HttpPassiveConn;
	friend class IBaseApplication;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<HttpHandler> Ptr;
<<<<<<< HEAD

	virtual ~HttpHandler();
=======
	virtual ~HttpHandler() {

		(_app.log())(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpHandler, "~HttpHandler"));

	}

	HttpPassiveConn& conn() { return _conn; }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

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
<<<<<<< HEAD
		int OngoingSize() { return _cOngoings.get(); }
		ZQ::common::Log& log() { return _log; }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual HttpHandler::Ptr create(HttpServer& server, const HttpMessage::Ptr& req, const HttpHandler::Properties& dirProps) =0;
=======
		ZQ::common::Log& log() { return _log; }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual HttpHandler::Ptr create(HttpPassiveConn& conn, const HttpHandler::Properties& dirProps) =0;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	protected:
		HttpHandler::Properties _appProps;
		ZQ::common::Log&        _log;
<<<<<<< HEAD

	private:
		friend class HttpHandler;
		ZQ::common::AtomicInt _cOngoings;
=======
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;

protected: // hatched by HttpApplication
<<<<<<< HEAD
	// the handler is created when all HTTP headers are received
	HttpHandler(const HttpMessage::Ptr& req, IBaseApplication& app, HttpServer& server, const HttpHandler::Properties& dirProps = HttpHandler::Properties());

protected: // forwarded from HttpConnection
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnRequestHeaders(const Response::Ptr resp) { _resp=resp; return HttpMessage::errAsyncInProgress; } // maps to RTSP::OnRequest-1
	virtual HttpMessage::StatusCodeEx OnRequestPayload(const char* data, size_t size) { return HttpMessage::errAsyncInProgress; } // maps to RTSP::OnRequest-2.2
	virtual HttpMessage::StatusCodeEx OnRequestCompleted(){} // maps to RTSP::OnRequest-3

	// forwarded from HTTPConnection
	virtual void	OnMessagingError(int error, const char* errorDescription) {}
	virtual void    OnResponsePayloadSubmitted(size_t bytes, uint64 totalPayloadOffset) {}

	HttpMessage::Ptr  _req;
	HttpHandler::Response::Ptr  _resp;
=======
	HttpHandler(IBaseApplication& app, HttpPassiveConn& conn, const HttpHandler::Properties& dirProps = HttpHandler::Properties())
		: _conn(conn), _app(app), _dirProps(dirProps)
	{}

	virtual void	onHttpDataSent(size_t size) {}
	virtual void	onHttpDataReceived( size_t size ) {}

	HttpPassiveConn& _conn;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	IBaseApplication& _app;
	HttpHandler::Properties _dirProps;
};

<<<<<<< HEAD
class HttpPassiveConn;

// ---------------------------------------
// template HttpApplication
// ---------------------------------------
=======
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
	virtual HttpHandler::Ptr create(HttpServer& server, const HttpMessage::Ptr& req, const HttpHandler::Properties& dirProps)
	{
		return new HandlerT(req, *this, server, dirProps);
	}
};

// ---------------------------------------
// class HttpServer
// ---------------------------------------
class HttpServer: public TCPServer
{
public:
	HttpServer( const TCPServer::ServerConfig& conf, ZQ::common::Log& logger);
	virtual ~HttpServer();

	// virtual bool onStart(ZQ::eloop::Loop& loop){ return true; }
	// virtual bool onStop(){ return true; }

public:
	// register an application to uri
	//@param uriEx - the regular expression of uri
	bool mount(const std::string& uriEx, HttpHandler::AppPtr app, const HttpHandler::Properties& props=HttpHandler::Properties(), const char* virtualSite =DEFAULT_SITE);
	bool unmount(const std::string& uriEx, const char* virtualSite =DEFAULT_SITE);
	void clearMounts() { _vsites.clear(); }

	HttpHandler::Ptr createHandler(const HttpMessage::Ptr& req, HttpConnection& conn, const std::string& virtualSite = std::string(DEFAULT_SITE));

	virtual TCPConnection* createPassiveConn();

public: // about the await responses
	void checkReqStatus();
	void addAwait(HttpHandler::Response::Ptr resp);
	void removeAwait(HttpHandler::Response::Ptr resp);
	int getPendingSize();

=======
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
	virtual void	onError( int error,const char* errorDescription );
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

>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
private:
	typedef struct _MountDir
	{
		std::string					uriEx;
<<<<<<< HEAD
=======
		boost::regex				re;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
		HttpHandler::AppPtr	app;
		HttpHandler::Properties     props;
	} MountDir;

	typedef std::vector<MountDir> MountDirs;
	typedef std::map<std::string, MountDirs> VSites;

	VSites _vsites;
<<<<<<< HEAD

private:
	HttpHandler::Response::List	_awaits;
	ZQ::common::Mutex			_lkAwaits;

=======
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
#endif
=======
#endif
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
