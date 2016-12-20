#ifndef __HTTP_SERVER_h__
#define __HTTP_SERVER_h__

#include "httpParser.h"
#include "eloop_net.h"
#include <boost/regex.hpp>
#include <set>
using namespace ZQ::eloop;


// ---------------------------------------
// class HttpProcessor
// ---------------------------------------
class HttpProcessor:public TCP,public ParserCallback
{
public:
	virtual ~HttpProcessor();

	bool			canSendHeader( ) const;
	bool			canSendBody( ) const;
	bool beginSend(HttpMessage::Ptr msg);
	bool sendBody(const char* buf,size_t len);
	bool endSend();

protected:
	HttpProcessor( bool clientSide );
	void			reset( ParserCallback* callback = NULL );
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);

private:
	HttpProcessor( const HttpProcessor&);
	HttpProcessor& operator=( const HttpProcessor&);

protected:

	virtual void onHttpError(int err) {}

	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void onHttpDataSent(size_t size) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void onHttpDataReceived(size_t size) { }

	// this callback is used by HttpServant
	virtual void onHttpEndSent(bool keepAlive) { }

private:
	http_parser_type		mParseType;
	HttpParser				mHttpParser;

	HttpMessage::Ptr		mIncomingMsg;
	HttpMessage::Ptr		mOutgoingMsg;

	std::string				mOutgoingHeadersTemp;
	std::string				mWritingBufs;
	bool					mbSending;
	bool					mbOutgoingKeepAlive;
};


// ---------------------------------------
// interface IHttpHandler
// ---------------------------------------
class IHttpHandler: public ParserCallback, public virtual ZQ::common::SharedObject {
public:
	typedef ZQ::common::Pointer<IHttpHandler> Ptr;
	virtual ~IHttpHandler() { }

	virtual void	onHttpDataSent(size_t size) = 0;

	virtual void	onHttpDataReceived( size_t size ) = 0;

	virtual void 	onWritable() = 0;
};

// ---------------------------------------
// interface IHttpHandlerFactory
// ---------------------------------------
class IHttpHandlerFactory:public ZQ::common::SharedObject {
public:
	typedef ZQ::common::Pointer<IHttpHandlerFactory> Ptr;
	virtual ~IHttpHandlerFactory() {}
	// NOTE: this method may be accessed by multi threads concurrently
	virtual IHttpHandler::Ptr create( HttpProcessor* processor ) = 0;		
};

// ---------------------------------------
// class HttpServant
// ---------------------------------------
class HttpServer;
class HttpServant:public HttpProcessor
{
public:
	HttpServant(HttpServer* server,ZQ::common::Log& logger);
	~HttpServant();

	bool			start();
	void			clear();
	const std::string& hint() const { return mHint; }

private:

	virtual void	onHttpError( int err );

	virtual void    onWritable();

	virtual void	onHttpDataSent(size_t size);

	virtual void 	onHttpEndSent( bool keepAlive );

	virtual void	onHttpDataReceived( size_t size );

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg);

	virtual bool	onHttpBody( const char* data, size_t size);

	virtual void	onHttpComplete();

	virtual void	onSocketConnected();



private:
	// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
	void 			errorResponse( int code );
	void initHint();


private:
	HttpServer*					m_server;
	IHttpHandler::Ptr			mHandler;
	int64						mLastTouch;
	int							mOutstandingRequests;
	ZQ::common::Log&			mLogger;
	bool						mHeaderComplete;

	std::string					mHint;
};




// ---------------------------------------
// class HttpServer
// ---------------------------------------
class HttpServer:public TCP
{
public:
	struct HttpServerConfig {
		HttpServerConfig() {
			defaultServerName		= "LibAsYnc HtTp SeRVer";
			httpHeaderAvailbleTimeout = 5* 60 * 1000;
			keepAliveIdleMin		= 5 * 60 * 1000;
			keepAliveIdleMax		= 10 * 60 *1000;
			maxConns 				= 100 * 1000;
		}

		std::string defaultServerName;
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

	bool addRule( const std::string& rule, IHttpHandlerFactory::Ptr factory );

	IHttpHandler::Ptr getHandler( const std::string& uri, HttpProcessor* conn);

	void	addServant( HttpServant* servant );
	void	removeServant( HttpServant* servant );

	HttpMessage::Ptr makeSimpleResponse( int code ) const;

protected:
	virtual void OnConnection_cb(ElpeError status);

private:
	struct UrlRule {
		std::string					rule;
		boost::regex				re;
		IHttpHandlerFactory::Ptr	factory;	
	};
	std::vector<UrlRule>	mUrlRules;
	HttpServerConfig		mConfig;
	std::set<HttpServant*> mServants;
	ZQ::common::Mutex		mLocker;
	ZQ::common::Log&		mLogger;
};

#endif