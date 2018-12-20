#ifndef __asynchttp_http_header_file_h__
#define __asynchttp_http_header_file_h__

#include <ZQ_common_conf.h>
#include <Locks.h>
#include <Log.h>
#include <boost/regex.hpp>
#include "eventloop.h"
#include "socket.h"
#include "httpparser.h"

namespace LibAsync {
	class EventLoopCenter : public LoopCenter {
	public:
		EventLoopCenter();
		virtual ~EventLoopCenter();
		bool	startCenter(ZQ::common::Log& log, const std::vector<int>& cpuIds);
		void	stopCenter();

		///��Center�����ȡһ��EventLoop����ǰ��ʵ�ְ汾��roundrobin
		virtual EventLoop&	getLoop();
		void				releaseLoop();
		static bool initLoops();
	protected:
		virtual void	addSocket(int id);
		virtual void	removeSocket(int id);

	private:
		struct LoopInfo {
			EventLoop*	loop;
			size_t		sockCount;
			LoopInfo() :loop(NULL),
				sockCount(0) {
			}
			bool operator<(const LoopInfo& rhs) const {
				return sockCount < rhs.sockCount;
			}
		};
		typedef std::vector<LoopInfo>	LOOPS;
		size_t				mIdxLoop;
		LOOPS				mLoops;
		ZQ::common::Mutex	mLocker;
	};

	EventLoopCenter& getLoopCenter();
	const AsyncBuffer& getBufForChunkTail();

	class ZQ_COMMON_API HttpProcessor : virtual public Socket, public ParserCallback{
	public:
		//////////////////////////////////////////////////////////////////////////		
		static bool	setup(ZQ::common::Log& log, const std::vector<int>& cpuIds);
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
		int 			sendChunk( BufferHelper& bh);
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
		AsyncBuffer				mHeadersBuf;
	
		enum ChunkSendingState {
			SENDING_CHUNK_NULL,
			SENDING_CHUNK_HEADER,
			SENDING_CHUNK_BODY,
			SENDING_CHUNK_TAIL,
			SENDING_CHUNK_ZERO
		};
		AsyncBufferS			mWritingBufs;
		AsyncBuffer				mChunkHeader;
		ChunkSendingState		mSendingChunkState;
		size_t					mLastUnsentBytes;
		size_t					mLastUnsentBodyBytes;
		bool					mbResentZeroChunk;

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

		///����Http Header�� Ҳ����HttpMessage������  
		bool		beginRequest( HttpMessagePtr msg, const std::string& ip, unsigned short port );

		/// ����HttpBody,���ʹ����chunkedģʽ�� HttpClient���Զ�Ϊ���ʽ����chunk��ʽ
		/// ��chunkģʽ���棬��Ҫ����0 chunk,ֱ�ӵ���endRequest����  
		bool		sendReqBody( const AsyncBuffer& buf );
		bool		sendReqBody( const AsyncBufferS& bufs );

		// ���Http����chunk���룬 ��ôonReqSent�ᱻ�������õ��� ��size==0
		// ���http��chunk���룬 ��ô����һ��0 chunk�����ݱ����ͳ�ȥ�� ���ͳɹ��Ժ�onReqSent�ᱻ���õ�  
		bool		endRequest( );

		//��ȡresponse��header
		//ʵ���������������ֱ����������Http Message������body
		bool		getResponse( );


		//��ȡresponse��body
		bool		recvRespBody( AsyncBuffer& buf);
		bool		recvRespBody( AsyncBufferS& bufs );
		
	protected:
		// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
		virtual void	onReqMsgSent( size_t size) {	}

		// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
		virtual void	onHttpDataReceived( size_t size ) { }

		virtual bool	onHttpMessage( const HttpMessagePtr msg) { 
			abort();
			return false; 
		}

		virtual bool	onHttpBody( const char* data, size_t size) { return false; }

		virtual void	onHttpComplete() { }

		virtual void	onHttpError( int error ) { }

	protected:

		void			reset( );
		void			clear( );

	private:

		/// ��ClientCenter�����ȡһ�����õ�Connection���������õ�ǰ�Ѿ����ڵ�Connection
		/// �����ǰû���κε�peer��connection����ôֻ���½�һ����
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
	 * FIXME: ����ͻ��˷�����һ���������message, �޷�֪ͨuser code
	 * 	      ��Ϊ���ʱ��, ���ǻ�û���õ��κ�user code��ʵ��, ���Կ�������Ҫ��
	 * 	      HttpServer�������һ���ӿ���ר�Ŵ����������b1.add(8);?
	 * */
	class HttpServant : public HttpProcessor {
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

	class ZQ_COMMON_API HttpServer : public Socket, public ZQ::common::NativeThread {
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

		void	stop( );

		HttpMessagePtr makeSimpleResponse( int code ) const;

		const HttpServerConfig&	config() const { return mConfig; }


	private:
		bool					startAt( const SocketAddrHelper& addr );

		int						run( );
		
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
		int						mSocket;
		bool					mbRunning;
	};

}//namespace LibAsync

#endif//__asynchttp_http_header_file_h__
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

