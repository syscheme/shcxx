#ifndef __asynchttp_http_header_file_h__
#define __asynchttp_http_header_file_h__

#include <ZQ_common_conf.h>
#include <Locks.h>
#include <Log.h>
#include <boost/regex.hpp>
#include "eventloop.h"
#include "socket.h"
#include "httpparser.h"

namespace LibAsync{

	class ZQ_COMMON_API HttpProcessor : public Socket, public ParserCallback{
	public:
		//////////////////////////////////////////////////////////////////////////		
		static bool	setup(ZQ::common::Log& log, size_t loopCount);
		static void	teardown();
		typedef ZQ::common::Pointer<HttpProcessor> Ptr;
		virtual ~HttpProcessor();
		bool			beginRecv();
		bool			recvBody( AsyncBuffer& buf);
		bool			recvBody( AsyncBufferS& bufs);
		
		bool			beginSend( HttpMessagePtr msg );
		bool			sendBody( const AsyncBuffer& buf );
		bool			sendBody( const AsyncBufferS& bufs);
		bool			endSend( );
#ifdef ZQ_OS_LINUX
        int			beginSend_direct( HttpMessagePtr msg );
        int			sendBody_direct( const AsyncBuffer& buf );
        int			sendBody_direct( const AsyncBufferS& bufs);
        int			endSend_direct( );
#endif

		void			resetHttp( ); // reset current all pending flag

	protected:
		HttpProcessor( bool clientSide );
		HttpProcessor( bool clientSide, SOCKET socket);
		void			reset( ParserCallback* callback = NULL );
	private:
		HttpProcessor( const HttpProcessor&);
		HttpProcessor& operator=( const HttpProcessor&);

		bool			canRecvHeader( ) const;
		bool			canRecvBody( ) const;
		bool			canSendHeader( ) const;
		bool			canSendBody( ) const;		

		virtual	void	onSocketError(int err);
		virtual	void	onSocketRecved(size_t size);
		virtual void	onSocketSent(size_t size);

	protected:

		virtual void	onHttpError( int err ) {}
		
		// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataSent( size_t size) {	}

		// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataReceived( size_t size ) { }

		// this callback is used by HttpServant
		virtual void 	onHttpEndSent( bool keepAlive ) { }
	private:
		http_parser_type		mParseType;
		HttpParser				mHttpParser;
		HttpMessagePtr			mIncomingMsg;
		HttpMessagePtr			mOutgoingMsg;
		AsyncBufferS			mReadingBufs;
		AsyncBufferS			mWritingBufs;
		AsyncBuffer				mChunkHeader;
		AsyncBuffer				mHeadersBuf;
        BufferHelper::Ptr       mBufferHelper;
		std::string				mOutgoingHeadersTemp;
		bool					mbRecving;
		bool					mbSending;
		bool					mbOutgoingKeepAlive;
#define HEADER_BUF_LEN	4096
#define CHUNKHEADER_BUF_LEN 32
		char					mTempBuffer[ HEADER_BUF_LEN + CHUNKHEADER_BUF_LEN ];
	};

	class ZQ_COMMON_API HttpClient;
	class HttpClient : public HttpProcessor {
	protected:
		HttpClient( );
	private:
		//disallow copy construction
		HttpClient( const HttpClient& );
		HttpClient& operator=( const HttpClient&);

	public:
		typedef ZQ::common::Pointer<HttpClient>		Ptr;
		
		virtual ~HttpClient(void);
		
		//////////////////////////////////////////////////////////////////////////
		
		//make HttpClient instance
		static HttpClient::Ptr	create();

		typedef enum _ProxyAuthType
		{
			  PAT_none, PAT_basic
		} ProxyAuthType;
		void		setHttpProxy(const std::string& urlProxy, ProxyAuthType pat=PAT_none);
		std::string getProxy() const;
		void		appendHttpProxyAuth(void) {}

		/// begin a http request, connect to peer if socket is disconnected.
		/// NOTE, uri and header host will be changed if you use this method
		bool		beginRequest( HttpMessagePtr msg, const std::string& url);

		///发送Http Header， 也就是HttpMessage的内容  
		bool		beginRequest( HttpMessagePtr msg, const std::string& ip, unsigned short port );

		/// 发送HttpBody,如果使用了chunked模式， HttpClient会自动为你格式化成chunk方式
		/// 在chunk模式下面，不要发送0 chunk,直接调用endRequest即可  
		bool		sendReqBody( const AsyncBuffer& buf );
		bool		sendReqBody( const AsyncBufferS& bufs );

		// 如果Http不是chunk编码， 那么onReqSent会被立即调用到， 且size==0
		// 如果http是chunk编码， 那么会有一个0 chunk的数据被发送出去， 发送成功以后onReqSent会被调用到  
		bool		endRequest( );

		//获取response的header
		//实际上这个函数可能直接拿完整个Http Message，包括body
		bool		getResponse( );


		//获取response的body
		bool		recvRespBody( AsyncBuffer& buf);
		bool		recvRespBody( AsyncBufferS& bufs );
		
	protected:
		// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
		virtual void	onReqMsgSent( size_t size) {	}

		// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataReceived( size_t size ) { }

		virtual bool	onHttpMessage( const HttpMessagePtr msg) { return false; }

		virtual bool	onHttpBody( const char* data, size_t size) { return false; }

		virtual void	onHttpComplete() { }

		virtual void	onHttpError( int error ) { }

	protected:

		void			reset( );
		void			clear( );

	private:

		/// 从ClientCenter里面获取一个可用的Connection，尽量复用当前已经存在的Connection
		/// 如果当前没有任何到peer的connection，那么只能新建一个了
		bool			makeConnection( const std::string& ip, unsigned short port);

		virtual void	onSocketConnected();
		//onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any more
		virtual void	onHttpDataSent( size_t size) {
			onReqMsgSent(size);
		}

	protected:
		std::string		mProxyUrl;
		ProxyAuthType	mPat;
		HttpMessagePtr	mRequest;
	};
	
	class SimpleHttpClient;
	typedef ZQ::common::Pointer<SimpleHttpClient> SimpleHttpClientPtr;
	class ZQ_COMMON_API SimpleHttpClient : public HttpClient {
	protected:
		SimpleHttpClient();

	private:
		SimpleHttpClient( const SimpleHttpClient& );
		SimpleHttpClient& operator=( const SimpleHttpClient& );

	public:
		virtual ~SimpleHttpClient();

		static	SimpleHttpClientPtr create();

		bool	doHttp( HttpMessagePtr msg, const std::string& body, const std::string& ip, unsigned short port );

	protected:

		const std::string&	getBody() const {
			return mRespBody;
		}

		HttpMessagePtr		getMessage( ) const {
			return mResponse;
		}

		virtual void		onSimpleHttpResponse( HttpMessagePtr msg, const std::string& body) { }
		virtual void		onSimpleHttpError( int error ) { }

	private:		
		virtual void	onReqMsgSent( size_t size);
		virtual void	onHttpDataReceived( size_t size );
		virtual bool	onHttpMessage( const HttpMessagePtr msg);
		virtual bool	onHttpBody( const char* data, size_t size);
		virtual void	onHttpComplete();
		virtual void	onHttpError( int error );

	private:
		HttpMessagePtr	mRequest;
		HttpMessagePtr	mResponse;
		std::string		mReqBody;
		bool			mBodySent;
		std::string		mRespBody;
		AsyncBuffer		mBodyBuffer;
		bool			mResponseComplete;
		bool			mResponseError;
	};	


	class IHttpHandler: public ParserCallback, public virtual ZQ::common::SharedObject {
	public:
		typedef ZQ::common::Pointer<IHttpHandler> Ptr;
		virtual ~IHttpHandler() { }

		virtual void	onHttpDataSent( size_t size) = 0;

		virtual void	onHttpDataReceived( size_t size ) = 0;

        virtual void 	onWritable() = 0;
	};

	class HttpServer;
	class IHttpHandlerFactory:public ZQ::common::SharedObject {
	public:
		typedef ZQ::common::Pointer<IHttpHandlerFactory> Ptr;
		virtual ~IHttpHandlerFactory() {}
		// NOTE: this method may be accessed by multi threads concurrently
		virtual IHttpHandler::Ptr create( HttpProcessor::Ptr processor ) = 0;		
	};

	/*
	 * Pipelining is not supported now
	 * FIXME: 如果客户端发送了一个有问题的message, 无法通知user code
	 * 	      因为这个时候, 我们还没有拿到任何user code的实例, 所以看起来需要在
	 * 	      HttpServer里面加上一个接口来专门处理这种情况b1.add(8);?
	 * */
	class HttpServant : public virtual HttpProcessor, public virtual Timer  {
	public:
		HttpServant( HttpServer& server, SOCKET socket, ZQ::common::Log& logger );
		virtual ~HttpServant();

		typedef ZQ::common::Pointer<HttpServant> Ptr;

		bool	start( );

		void	reset( IHttpHandler::Ptr handler );

		void	initHint();

		const std::string& hint() const { return mHint; }

	private:

		virtual void	onHttpError( int err );

        virtual void    onWritable();

		virtual void	onHttpDataSent( size_t size);

		virtual void 	onHttpEndSent( bool keepAlive );

		virtual void	onHttpDataReceived( size_t size );

		virtual bool	onHttpMessage( const HttpMessagePtr msg);

		virtual bool	onHttpBody( const char* data, size_t size);

		virtual void	onHttpComplete();

		virtual void	onSocketConnected();

		virtual void 	onTimer();

		void			clear();

	private:
		// NOTE: DO NOT INVOKE THIS METHOD unless you known what you are doing
		void 			errorResponse( int code );

	private:

		HttpServer&					mServer;
		IHttpHandler::Ptr			mHandler;
		int64						mLastTouch;
		int							mOutstandingRequests;
		ZQ::common::Log&			mLogger;
		bool						mHeaderComplete;

		std::string					mHint;
	};

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

	class ZQ_COMMON_API HttpServer : public Socket {
	protected:
		HttpServer( const HttpServerConfig& conf, ZQ::common::Log& logger );
	public:	
		typedef ZQ::common::Pointer<HttpServer> Ptr;
		static HttpServer::Ptr	create( const HttpServerConfig& conf, ZQ::common::Log& logger );
		virtual ~HttpServer();

		bool	addRule( const std::string& rule, IHttpHandlerFactory::Ptr factory );

		IHttpHandler::Ptr getHandler( const std::string& uri, HttpProcessor::Ptr conn );

		void	updateServant( HttpServant::Ptr servant );

		void	removeServant( HttpServant::Ptr servant );

		bool	startAt( const std::string& ip, unsigned short port);
		bool	startAt( const std::string& ip, const std::string& port);

		HttpMessagePtr makeSimpleResponse( int code ) const;

		const HttpServerConfig&	config() const { return mConfig; }
	private:
		
		virtual	void			onSocketError(int err);

		virtual Socket::Ptr		onSocketAccepted( SOCKET sock ) ;

	private:
		struct UrlRule {
			std::string					rule;
			boost::regex				re;
			IHttpHandlerFactory::Ptr	factory;	
		};
		std::vector<UrlRule>	mUrlRules;
		HttpServerConfig		mConfig;
		std::set<HttpServant::Ptr> mServants;
		ZQ::common::Mutex		mLocker;
		ZQ::common::Log&		mLogger;
	};

}//namespace LibAsync

#endif//__asynchttp_http_header_file_h__
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

