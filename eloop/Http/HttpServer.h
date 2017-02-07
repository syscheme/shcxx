#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "httpMessage.h"
#include "eloop_net.h"
#include "HttpConnection.h"
#include <boost/regex.hpp>
#include <set>
using namespace ZQ::eloop;

class HttpServer;

// ---------------------------------------
// interface IHttpHandler
// ---------------------------------------
class IHttpHandler: public ParserCallback, public virtual ZQ::common::SharedObject {
public:
	typedef ZQ::common::Pointer<IHttpHandler> Ptr;
	virtual ~IHttpHandler() { }

	virtual void	onHttpDataSent(bool keepAlive) = 0;

	virtual void	onHttpDataReceived( size_t size ) = 0;

	virtual void 	onWritable() = 0;
};

// ---------------------------------------
// class HttpApplication
// ---------------------------------------
// present an HTTP application that respond a URI mount
// the major funcation of the application is to instantiaze a IHttpHandler
class HttpApplication : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<HttpApplication> Ptr;
	virtual ~HttpApplication() {}
	// NOTE: this method may be accessed by multi threads concurrently
	virtual IHttpHandler::Ptr create( HttpConnection& processor ) { return NULL; }		
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
	void			clear();
	const std::string& hint() const { return _Hint; }

protected:

	// implementation of HttpConnection
	virtual void	onHttpError( int err );
	virtual void    onWritable();
	virtual void	onHttpDataSent( bool keepAlive );
	virtual void	onHttpDataReceived( size_t size );
	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);
	virtual bool	onHttpBody( const char* data, size_t size);
	virtual void	onHttpComplete();

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void 			errorResponse( int code );
	void initHint();

private:
	HttpServer&					_server;
	IHttpHandler::Ptr			_Handler;
	int64						_LastTouch;
	
	ZQ::common::Log&			_Logger;
	bool						_HeaderComplete;
	bool						_bError;

	std::string					_Hint;
};

// ---------------------------------------
// class HttpServer
// ---------------------------------------
// Single event loop
class HttpServer:public TCP
{
public:
	struct HttpServerConfig
	{
		HttpServerConfig() {
			serverName		= "LibAsYnc HtTp SeRVer";
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

	bool startAt( const char* ip, int port);

	void stop( );

	// register an application to uri
	//@param uriEx - the regular expression of uri
	bool registerApp( const std::string& uriEx, HttpApplication::Ptr app);

	IHttpHandler::Ptr getHandler( const std::string& uri, HttpConnection& conn);

	void	addConn( HttpPassiveConn* servant );
	void	delConn( HttpPassiveConn* servant );

	HttpMessage::Ptr makeSimpleResponse( int code ) const;

protected:
	virtual void doAccept(ElpeError status);

private:
	typedef struct _UriMount
	{
		std::string					uriEx;
		boost::regex				re;
		HttpApplication::Ptr	app;	
	} UriMount;

	std::vector<UriMount>	_uriMounts;
	HttpServerConfig		_Config;
	std::set<HttpPassiveConn*> _PassiveConn;
	ZQ::common::Mutex		_Locker;
	ZQ::common::Log&		_Logger;
};

#endif