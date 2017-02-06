#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "httpParser.h"
#include "eloop_net.h"
#include <boost/regex.hpp>
#include <set>
using namespace ZQ::eloop;

class HttpServer;

// ---------------------------------------
// class HttpConnection
// ---------------------------------------
// present an accepted incomming connection
class HttpConnection: public TCP, public ParserCallback
{
public:
	virtual ~HttpConnection();

	int send(const char* buf,size_t len);
	void setkeepAlive(bool alive){mbOutgoingKeepAlive = alive;}

protected:
	HttpConnection(bool clientSide);
	void			reset( ParserCallback* callback = NULL );
	int getSendCount() {return mSendCount;}
	
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);

private:
	HttpConnection( const HttpConnection&);
	HttpConnection& operator=( const HttpConnection&);

protected:

	/// error occured during data receiving or parsing stage
	virtual void onHttpError(int err) {}

	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void onHttpDataSent(bool keepAlive) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void onHttpDataReceived(size_t size) { }

private:
	http_parser_type		mParseType;
	HttpParser				mHttpParser;

	HttpMessage::Ptr		mIncomingMsg;

	int						mSendCount;
	bool					mbOutgoingKeepAlive;


//	bool					mbSending;
//	bool					mbOutgoingKeepAlive;
};


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
	virtual IHttpHandler::Ptr create( HttpConnection* processor ) { return NULL; }		
};

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
class HttpPassiveConn : public HttpConnection
{
public:
	HttpPassiveConn(HttpServer* server,ZQ::common::Log& logger);
	~HttpPassiveConn();

	bool			start();
	void			clear();
	const std::string& hint() const { return mHint; }

protected:

	// implementation of HttpConnection
	virtual void	onHttpError( int err );
	virtual void    onWritable();
	virtual void	onHttpDataSent( bool keepAlive );
	virtual void	onHttpDataReceived( size_t size );
	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);
	virtual bool	onHttpBody( const char* data, size_t size);
	virtual void	onHttpComplete();

	// implementation of TCP
	virtual void	onSocketConnected();

private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void 			errorResponse( int code );
	void initHint();

private:
	HttpServer*					_server;
	IHttpHandler::Ptr			mHandler;
	int64						mLastTouch;
	
	ZQ::common::Log&			mLogger;
	bool						mHeaderComplete;
	bool						mbError;

	std::string					mHint;
};

// ---------------------------------------
// class HttpServer
// ---------------------------------------
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

	IHttpHandler::Ptr getHandler( const std::string& uri, HttpConnection* conn);

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
	HttpServerConfig		mConfig;
	std::set<HttpPassiveConn*> mServants;
	ZQ::common::Mutex		mLocker;
	ZQ::common::Log&		mLogger;
};

#endif