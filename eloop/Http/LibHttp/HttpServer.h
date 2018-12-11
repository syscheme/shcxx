#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "HttpConnection.h"
#include "eloop_net.h"

#include <NativeThread.h>
#include <SystemUtils.h>
#include <list>


namespace ZQ {
namespace eloop {

#define DUMMY_PROCESS_TIMEOUT (60*1000) // 1min a dummy big time
#define DEFAULT_SITE "."

class ZQ_ELOOP_HTTP_API HttpServer;
class ZQ_ELOOP_HTTP_API HttpHandler;

// ---------------------------------------
// interface HttpHandler
// ---------------------------------------
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

public:
	typedef std::map<std::string, std::string> Properties;
	typedef ZQ::common::Pointer<HttpHandler> Ptr;

	virtual ~HttpHandler()
	{
		(_app.log())(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpHandler, "~HttpHandler"));
	}

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
		int OngoingSize() { return _cOngoings.get(); }
		ZQ::common::Log& log() { return _log; }

		// NOTE: this method may be accessed by multi threads concurrently
		virtual HttpHandler::Ptr create(HttpServer& server, const HttpMessage::Ptr& req, const HttpHandler::Properties& dirProps) =0;

	protected:
		HttpHandler::Properties _appProps;
		ZQ::common::Log&        _log;

	private:
		friend class HttpHandler;
		ZQ::common::AtomicInt _cOngoings;
	};

	typedef ZQ::common::Pointer<IBaseApplication> AppPtr;

protected: // hatched by HttpApplication
	// the handler is created when all HTTP headers are received
	HttpHandler(const HttpMessage::Ptr& req, IBaseApplication& app, HttpServer& server, const HttpHandler::Properties& dirProps = HttpHandler::Properties())
		: _req(req), _app(app), _dirProps(dirProps)
	{}

protected: // forwarded from HttpConnection
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnRequestHeaders(const Response::Ptr resp) { _resp=resp; return HttpMessage::errAsyncInProgress; } // maps to RTSP::OnRequest-1
	virtual HttpMessage::StatusCodeEx OnRequestChunk(const char* data, size_t size) { return HttpMessage::errAsyncInProgress; } // maps to RTSP::OnRequest-2.2
	virtual HttpMessage::StatusCodeEx OnRequestCompleted(); // maps to RTSP::OnRequest-3

	// forwarded from HTTPConnection
	virtual void	OnMessagingError(int error, const char* errorDescription) {}
	virtual void    OnResponsePayloadSubmitted(size_t bytes, uint64 totalPayloadOffset) {}

	HttpMessage::Ptr  _req;
	HttpHandler::Response::Ptr  _resp;
	IBaseApplication& _app;
	HttpHandler::Properties _dirProps;
};

class HttpPassiveConn;

// ---------------------------------------
// template HttpApplication
// ---------------------------------------
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

private:
	HttpHandler::Response::List	_awaits;
	ZQ::common::Mutex			_lkAwaits;

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
