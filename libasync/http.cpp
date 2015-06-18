#include <assert.h>
#include <vector>
#include <Locks.h>
#include <TimeUtil.h>
#include <urlstr.h>
#include "http.h"

namespace LibAsync {
	
	class EventLoopCenter {
	public:
		EventLoopCenter();
		virtual ~EventLoopCenter();
		bool	startCenter(ZQ::common::Log& log, size_t count);
		void	stopCenter();
		
		///从Center里面获取一个EventLoop，当前的实现版本是roundrobin
		virtual EventLoop&	getLoop();

	private:
		typedef std::vector<EventLoop*>	LOOPS;
		size_t				mIdxLoop;
		LOOPS				mLoops;
		ZQ::common::Mutex	mLocker;
	};

	EventLoopCenter::EventLoopCenter():mIdxLoop(0){
	}

	EventLoopCenter::~EventLoopCenter() {
		stopCenter();
	}

	bool EventLoopCenter::startCenter( ZQ::common::Log& log, size_t count) {
		if( count <= 0)
			count =	1;
		ZQ::common::MutexGuard gd(mLocker);
		if( mLoops.size() > 0 )
			return true;
		for( size_t i = 0; i < count; i ++ ) {
			EventLoop* l = new EventLoop(log, i);
			if(!l->start()){
				delete l;
				return false;
			}
			mLoops.push_back(l);
		}

#ifdef ZQ_OS_MSWIN
        if (!Socket::getAcceptEx() || !Socket::getConnectEx()){
            return false;
        }
#endif			
		return true;
	}

	void EventLoopCenter::stopCenter() {
		ZQ::common::MutexGuard gd(mLocker);
		if( mLoops.size() == 0 )
			return;
		for( size_t i = 0 ; i < mLoops.size(); i ++ ) {
			mLoops[i]->stop();
			delete mLoops[i];
		}
		mLoops.clear();
	}

	///从Center里面获取一个EventLoop，当前的实现版本是roundrobin
	EventLoop& EventLoopCenter::getLoop(){
	    size_t idx = 0;
		assert(mLoops.size() > 0);
		{
			ZQ::common::MutexGuard gd(mLocker);
			idx = mIdxLoop++;
			if(mIdxLoop >= mLoops.size())
				mIdxLoop = 0;
		}
		return *mLoops[idx];
	}

	static EventLoopCenter httpClientCenter;
	static AsyncBuffer		chunkTail;

	bool HttpProcessor::setup(ZQ::common::Log&log, size_t loopCount){
#ifdef ZQ_OS_MSWIN
		//init WSA env
		WSADATA	wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			return false;
		}
#endif
		chunkTail.base = (char*)malloc( sizeof(char)*2);
		chunkTail.len = 2;
		memcpy(chunkTail.base,"\r\n",2);
		return httpClientCenter.startCenter(log, loopCount);
	}

	void HttpProcessor::teardown() {
		httpClientCenter.stopCenter();
		free(chunkTail.base);
		chunkTail.len = 0;
		chunkTail.base = NULL;
#ifdef ZQ_OS_MSWIN
		//clean WSA env
		WSACleanup(); 
#endif
	}

	HttpProcessor::HttpProcessor( bool clientSide )
	:Socket(httpClientCenter.getLoop()),
	mParseType(clientSide?HTTP_RESPONSE:HTTP_REQUEST),
	mHttpParser(mParseType),
	mSendingChunkState(SENDING_CHUNK_NULL),
	mLastUnsentBytes(0),
	mLastUnsentBodyBytes(0),
	mbResentZeroChunk(false),
	mbRecving(false),
	mbSending(false), 
	mbOutgoingKeepAlive(false) {
		mChunkHeader.len = CHUNKHEADER_BUF_LEN;
		mChunkHeader.base = mTempBuffer + HEADER_BUF_LEN;
		mHeadersBuf.len = HEADER_BUF_LEN;
		mHeadersBuf.base = mTempBuffer;
		reset();
	}
	HttpProcessor::HttpProcessor( bool clientSide, SOCKET socket )
		:Socket(httpClientCenter.getLoop(),socket),
		mParseType(clientSide?HTTP_RESPONSE:HTTP_REQUEST),
		mHttpParser(mParseType),
		mSendingChunkState(SENDING_CHUNK_NULL),
		mLastUnsentBytes(0),
		mLastUnsentBodyBytes(0),
		mbResentZeroChunk(false),
		mbRecving(false),
		mbSending(false),
		mbOutgoingKeepAlive(false) {
			mChunkHeader.len = CHUNKHEADER_BUF_LEN;
			mChunkHeader.base = mTempBuffer + HEADER_BUF_LEN;
			mHeadersBuf.len = HEADER_BUF_LEN;
			mHeadersBuf.base = mTempBuffer;
			reset();
	}

	HttpProcessor::~HttpProcessor() {
	}

	void HttpProcessor::resetHttp( ) {
		Socket::close();
		reset(NULL);
		mbRecving = mbSending = false;
		mbOutgoingKeepAlive = false;
		
		mLastUnsentBytes = 0;
		mLastUnsentBodyBytes = 0;
		mbResentZeroChunk = false;
		mSendingChunkState = SENDING_CHUNK_NULL;
		mOutgoingHeadersTemp.clear();
		mReadingBufs.clear();
		mWritingBufs.clear();
		mIncomingMsg = NULL;
		mOutgoingMsg = NULL;
	}

	void HttpProcessor::reset(ParserCallback* p ) {
		if(!p)
			p = dynamic_cast<ParserCallback*>(this);
		mHttpParser.reset(p);
	}

	bool HttpProcessor::canRecvHeader( ) const {
		return (mIncomingMsg == NULL) && (!mbRecving);
	}

	bool HttpProcessor::canRecvBody( ) const {
		return (mIncomingMsg != NULL) && (!mbRecving);
	}

	bool HttpProcessor::canSendHeader( ) const {
		return (mOutgoingMsg == NULL) && (!mbSending);
	}

	bool HttpProcessor::canSendBody( ) const {
		assert( (mOutgoingMsg != NULL) && (!mbSending) );
		return ((mOutgoingMsg != NULL) && (!mbSending));
	}

	bool HttpProcessor::beginRecv() {		
		if(!canRecvHeader()) {			
			return false;
		}
		AsyncBufferS bufs;
		bufs.push_back(mHeadersBuf);
		mbRecving = true;
		return  recv(bufs);		
	}

	bool HttpProcessor::recvBody( AsyncBuffer& buf) {
		AsyncBufferS bufs(1,buf);
		return recvBody(bufs);
	}

	bool HttpProcessor::recvBody( AsyncBufferS& bufs) {
		if(!canRecvBody()) {
			assert(false&&"invalid state");
			return false;
		}
		mReadingBufs = bufs;
		mbRecving = true;
		return recv(bufs);		
	}

	bool HttpProcessor::beginSend(LibAsync::HttpMessagePtr msg) {
		if(!canSendHeader()) {
			assert(false&&"invalid state");
			return false;
		}
		mOutgoingMsg = msg;
		assert(msg != NULL && "msg can not be NULL");
		mOutgoingHeadersTemp = mOutgoingMsg->toRaw();
		assert(!mOutgoingHeadersTemp.empty());
		AsyncBuffer buf;
		buf.base = const_cast<char*>(mOutgoingHeadersTemp.c_str());
		buf.len = mOutgoingHeadersTemp.length();
		mbSending = true;
		mbOutgoingKeepAlive = msg->keepAlive();
		return send(buf);
	}

	bool HttpProcessor::sendBody(const LibAsync::AsyncBuffer &buf) {
		LibAsync::AsyncBufferS bufs(1,buf);
		return sendBody( bufs );
	}

	bool HttpProcessor::sendBody(const LibAsync::AsyncBufferS &bufs) {
		if(!canSendBody()) {
			assert(false && "invalid state");
			return false;
		}
		if(!mOutgoingMsg->hasContentBody() ) {
			assert( false && "http message do not have a content body");
			return false;
		}
		mWritingBufs = bufs;
		if( mOutgoingMsg->chunked() ) {
			mChunkHeader.len =  sprintf( mChunkHeader.base, "%x\r\n", (unsigned int)buffer_size(bufs));// should not fail
			mWritingBufs.insert(mWritingBufs.begin(),mChunkHeader);
			mWritingBufs.push_back( chunkTail );
		}
		mbSending = true;
		return send( mWritingBufs );
	}

	bool HttpProcessor::endSend() {
		if( !canSendBody() ) {
			assert( false && "invalid state");
			return false;
		}
		if(!mOutgoingMsg->chunked()) {
			//TODO: invoking callback here
			onHttpDataSent(0);
			mOutgoingMsg = NULL;
			onHttpEndSent(mbOutgoingKeepAlive);
			return true;
		}
		mChunkHeader.len = sprintf(mChunkHeader.base,"0\r\n");
		mWritingBufs.clear();
		mWritingBufs.push_back(mChunkHeader);
		mWritingBufs.push_back(chunkTail);
		mOutgoingMsg = NULL;
		mbSending = true;
		return send(mWritingBufs);
	}

#ifdef ZQ_OS_LINUX
    int HttpProcessor::beginSend_direct(LibAsync::HttpMessagePtr msg) {
        if(!canSendHeader()) {
            assert(false&&"invalid state");
            return false;
        }
        mOutgoingMsg = msg;
        assert(msg != NULL && "msg can not be NULL");
        mOutgoingHeadersTemp = mOutgoingMsg->toRaw();
        assert(!mOutgoingHeadersTemp.empty());
        AsyncBuffer buf;
        buf.base = const_cast<char*>(mOutgoingHeadersTemp.c_str());
        buf.len = mOutgoingHeadersTemp.length();
        mbSending = true;
        mbOutgoingKeepAlive = msg->keepAlive();
       
        int res = sendDirect(buf);
        mbSending = false;

        return res;
    }

    int HttpProcessor::sendBody_direct(const LibAsync::AsyncBuffer &buf) {
        LibAsync::AsyncBufferS bufs(1,buf);
        return sendBody_direct( bufs );
    }

    int HttpProcessor::sendBody_direct(const LibAsync::AsyncBufferS &bufs) {
        if(!canSendBody()) {
            assert(false && "invalid state");
            return false;
        }
        if(!mOutgoingMsg->hasContentBody() ) {
            assert( false && "http message do not have a content body");
            return false;
        }

		if( !mOutgoingMsg->chunked() ) {
			return sendDirect( bufs);
		}

		size_t sentDataSize = 0;
		BufferHelper bh(bufs);
		int res = 0;
		while (bh.size() > 0) {
			if (mLastUnsentBytes == 0) {
				mWritingBufs = bh.getBuffers();

				mChunkHeader.len = sprintf(mChunkHeader.base, "%x\r\n", (unsigned int)bh.size());// should not fail
				mWritingBufs.insert(mWritingBufs.begin(), mChunkHeader);
				mWritingBufs.push_back(chunkTail);
				mLastUnsentBodyBytes = bh.size();
				mSendingChunkState = SENDING_CHUNK_NULL;
			} else {
				mWritingBufs.clear();

				switch (mSendingChunkState) {
					case SENDING_CHUNK_NULL:
						assert(false);//should not be here
						break;
					case SENDING_CHUNK_HEADER:
						{				
							AsyncBuffer buf(mChunkHeader);
							buf.base += (buf.len - mLastUnsentBytes);
							buf.len = mLastUnsentBytes;
							mWritingBufs.push_back(buf);
						}
						//fallthrough
					case SENDING_CHUNK_BODY:
						{
							if (mLastUnsentBodyBytes <= bh.size()) {
								AsyncBufferS newBufs = bh.getAt( mLastUnsentBodyBytes );					
								mWritingBufs.insert(mWritingBufs.end(), newBufs.begin(), newBufs.end());
								mWritingBufs.push_back(chunkTail);
							} else {
								AsyncBufferS newBufs = bh.getBuffers();					
								mWritingBufs.insert(mWritingBufs.end(), newBufs.begin(), newBufs.end());
								//mWritingBufs.push_back(chunkTail);
							}				
						}
						break;
					case SENDING_CHUNK_TAIL:
						{
							assert(mLastUnsentBytes <= chunkTail.len);
							AsyncBuffer buf(chunkTail);
							buf.base += (chunkTail.len - mLastUnsentBytes);
							buf.len = mLastUnsentBytes;
							mWritingBufs.push_back(buf);
						}
						break;
					default:
						assert(false && "ivnalid coding logic");
						break;			
				}
			}

			size_t expectSize = buffer_size(mWritingBufs);

			mbSending = true;
			res = sendDirect(mWritingBufs);
			mbSending = false;

			if (res >= 0 && (size_t)res < expectSize) {
				if (res == 0) {
					res = ERR_EAGAIN;
				} else {				
					assert(mWritingBufs.size() >= 1);
					switch (mSendingChunkState) {
					case SENDING_CHUNK_NULL:
						//fallthrough
					case SENDING_CHUNK_HEADER:
						{
							const AsyncBuffer& header = mWritingBufs[0];
							const AsyncBuffer& body = mWritingBufs[1];
							if ((size_t)res < header.len) {
								mLastUnsentBytes = header.len - (size_t)res;
								mSendingChunkState = SENDING_CHUNK_HEADER;
								res = 0;
							} else if ((size_t)res < (header.len + body.len)) {
								res -= (int)header.len;
								mLastUnsentBodyBytes -= (size_t)res;
								mLastUnsentBytes = mLastUnsentBodyBytes;
								mSendingChunkState = SENDING_CHUNK_BODY;
							} else {
								assert( (expectSize - res ) <= 2 );
								mLastUnsentBytes = expectSize - (size_t)res;
								mSendingChunkState = SENDING_CHUNK_TAIL;
								res = (int)mLastUnsentBodyBytes;
								mLastUnsentBodyBytes = 0;
							}
							break;
						}				
					case SENDING_CHUNK_BODY:
						{					
							const AsyncBuffer& body = mWritingBufs[0];
							if ((size_t)res <body.len) {						
								mLastUnsentBodyBytes -= (size_t)res;
								mLastUnsentBytes = mLastUnsentBodyBytes;
								mSendingChunkState = SENDING_CHUNK_BODY;
							} else {
								mLastUnsentBytes = expectSize - (size_t)res;
								assert( (expectSize - res ) <= 2 );
								mSendingChunkState = SENDING_CHUNK_TAIL;
								res = (int)mLastUnsentBodyBytes;
								mLastUnsentBodyBytes = 0;
							}
							break;
						}				
					case SENDING_CHUNK_TAIL:
						{
							mLastUnsentBytes = expectSize - (size_t)res;
							assert( (expectSize - res ) <= 2 );
							mSendingChunkState = SENDING_CHUNK_TAIL;
							res = (int)mLastUnsentBodyBytes;
							mLastUnsentBodyBytes = 0;
							break;
						}				
					default:
						break;
					}
				}
				sentDataSize += res;
				break;
			} else if( res < 0 ){
				break;
			} else if ( (size_t)res == expectSize ) {
				switch (mSendingChunkState) 
				{
					case SENDING_CHUNK_NULL:
						//fallthrough
					case SENDING_CHUNK_HEADER:
						bh.adjust(mWritingBufs[1].len);
						sentDataSize += mWritingBufs[1].len;
						mLastUnsentBodyBytes -= mWritingBufs[1].len;
						break;
					case SENDING_CHUNK_BODY:
						bh.adjust(mWritingBufs[0].len);
						sentDataSize += mWritingBufs[0].len;
						mLastUnsentBodyBytes -= mWritingBufs[0].len;				
						break;
					default:
						break;
				}			
				mLastUnsentBytes = 0;
				if ( mLastUnsentBodyBytes == 0)
					mSendingChunkState = SENDING_CHUNK_NULL;
				continue;
			} else {
				break;
			}
		}
		if (sentDataSize == 0)
			return res;
		else
			return (int)sentDataSize;

    }

    int HttpProcessor::endSend_direct() {
        if( !canSendBody() ) {
            assert( false && "invalid state");
            return ERR_ERROR;
        }
        if(!mOutgoingMsg->chunked()) {
            //TODO: invoking callback here
            onHttpDataSent(0);
            mOutgoingMsg = NULL;
            onHttpEndSent(mbOutgoingKeepAlive);
            return 0;
        }
		//FIXME: 这里还是有问题的，如果sendbody_direct没有完成tail的发送，那么需要在这个地方来完成这个事情
		
		AsyncBuffer headerBuf;
		if( !mbResentZeroChunk ) {
			if( mLastUnsentBytes == 0 ) {
				mChunkHeader.len = sprintf(mChunkHeader.base,"0\r\n");
			} else {
				mChunkHeader.len = sprintf(mChunkHeader.base,"\r\n0\r\n");
			} 
			mLastUnsentBytes += mChunkHeader.len;
			headerBuf = mChunkHeader;
			if( mLastUnsentBytes != 0 ) {
				//在极端情况下上，最后一次sendBody_direct可能没有成功发出上一次的chunktail,
				//那么就需要在本次一并发送
				headerBuf.base += headerBuf.len-mLastUnsentBytes;
				headerBuf.len = mLastUnsentBytes;
			}
			mWritingBufs.clear();
			mWritingBufs.push_back(headerBuf);
			mWritingBufs.push_back(chunkTail);
		}

		size_t expectSize = buffer_size(mWritingBufs);

        mbSending = true;
        int res = sendDirect(mWritingBufs);
        mbSending =  false;
		if( res >= 0 && (size_t)res < expectSize ) {
			//assume the next call would send out all data
			if( (size_t)res < mWritingBufs[0].len ) {
				mLastUnsentBytes -= res;
				mWritingBufs[0].base += res;
				mWritingBufs[0].len -= res;
			} else if((size_t)res >= mWritingBufs[0].len)	{
				res -= mWritingBufs[0].len;
				mWritingBufs.clear();
				AsyncBuffer buf(chunkTail);
				buf.base += (3-res);
				buf.len -= (3-res);
				mWritingBufs.push_back(buf);
			}
			mbResentZeroChunk = true;
			res = ERR_EAGAIN;
		} else {
			mbResentZeroChunk = false;
			mLastUnsentBytes = 0;
			mOutgoingMsg = NULL;
		}

        return res;
    }
#endif
	
	void HttpProcessor::onSocketError(int err) {
		mbSending = mbRecving = false; // Is this OK ?
		onHttpError(err);
	}

	void HttpProcessor::onSocketRecved(size_t size ) {
		assert( size != 0 && "we did not recv for a buffer of size 0");
		mbRecving = false;
		if( size == 0 )
			return;
		AsyncBufferS bufs;
		if( mIncomingMsg == NULL ) {
			bufs.push_back(mHeadersBuf);
		} else {
			bufs.swap(mReadingBufs);
		}
		size_t tempSize = size;
		AsyncBufferS::const_iterator itBuf = bufs.begin();
		for( ; itBuf != bufs.end() ; itBuf ++ ) {
			size_t dataSize = tempSize > itBuf->len ? itBuf->len : tempSize;
			if ( dataSize <= 0 )
				break;
			size_t nparsed = mHttpParser.parse(itBuf->base, dataSize);
			if(nparsed != dataSize){
				onHttpError(mHttpParser.lastError());
				return;
			}
			tempSize -= dataSize;
		}		
		if(!mHttpParser.headerComplete()){
			bool bOK = beginRecv();
			assert(bOK);// Is this OK?
			return;
		} 
		mIncomingMsg = mHttpParser.currentHttpMessage();		

		if( mHttpParser.httpComplete()) {
			reset();
		}
		onHttpDataReceived(size);	
	}

	void HttpProcessor::onSocketSent( size_t size ) {
		mbSending = false;
		mOutgoingHeadersTemp.clear();
		// if whole http raw message has been sent
		// mOutgoingMsg should be NULL
		onHttpDataSent(size);
		if( mOutgoingMsg == NULL ) {
			onHttpEndSent(mbOutgoingKeepAlive);
		}
	}

	HttpClient::HttpClient( )
		:HttpProcessor(true){
	}

	HttpClient::~HttpClient(){
	}

	HttpClient::Ptr HttpClient::create(){
		return new HttpClient();
	}

	bool HttpClient::beginRequest( HttpMessagePtr msg, const std::string& url) {
		if(alive()) {
			//one HttpClient connect to two or more peer address is not allowed
			// so if current socket is alive, we just return as connecting procedure is done
			onSocketConnected();
			return true;
		}

		ZQ::common::URLStr urlstr(url.c_str());
		const char* host = urlstr.getHost();
		SocketAddrHelper helper(std::string(host),urlstr.getPort());
		if(!helper.info() )
			return false;
		char ip[17] = {0};
		const struct addrinfo* addr = helper.info();
		if(getnameinfo( addr->ai_addr, (socklen_t)addr->ai_addrlen,
			ip, sizeof(ip), NULL,0,NI_NUMERICHOST ) != 0 ) {
			return false;
		}
		//change uri, host in msg
        msg->url( urlstr.getPathAndParam() );
        if(msg->url().empty() ) 
            msg->url("/");
		
		msg->header("Host",host);
		return beginRequest( msg, ip, urlstr.getPort());
	}

	bool HttpClient::makeConnection(const std::string& ip, unsigned short port) {
		if(alive()){
			//dummy cb call
			onSocketConnected();
			return true;
		}
		return Socket::connect(ip,port);
	}

	void HttpClient::setHttpProxy(const std::string& urlProxy, ProxyAuthType pat) {
		mProxyUrl = urlProxy;
		mPat = pat;
	}

	std::string HttpClient::getProxy() const {
		return mProxyUrl;
	}

	bool HttpClient::beginRequest( HttpMessagePtr msg, const std::string& ip, unsigned short port ) {
        std::string connIP = ip;
        unsigned short connPort = port;

        if (!getProxy().empty())
        {
            ZQ::common::URLStr urlstr(mProxyUrl.c_str());
            const char* host = urlstr.getHost();
            int proxyPort = urlstr.getPort();
            SocketAddrHelper helper(std::string(host), proxyPort);
            if(!helper.info() )
                return false;
            char proxyIP[17] = {0};
            const struct addrinfo* addr = helper.info();
            if(getnameinfo( addr->ai_addr, (socklen_t)addr->ai_addrlen,
                proxyIP, sizeof(proxyIP), NULL,0,NI_NUMERICHOST ) != 0 ) {
                    return false;
            }

            connIP   = proxyIP;
            connPort = proxyPort;

            boost::regex reg("^https?://.+");
            if (!boost::regex_match(msg->url(), reg))
            { // relative path
                std::ostringstream oss;
                oss<<port;
                std::string url = "http://" + ip + ":" + oss.str();
                if (msg->url().at(0) == '/')
                {
                    url += msg->url();
                }else{
                    url += "/" + msg->url();
                }
                msg->url(url);
            }

            msg->header("Proxy-Connection", "Keep-Alive");
            if (mPat == LibAsync::HttpClient::PAT_basic)
            {
                //Proxy-Authorization: Basic base64(user:passwd);
                msg->header("Proxy-Authorization", "Basic base64(user:passwd)");
            }
        }

		mRequest = msg;
		if(!makeConnection(connIP,connPort)) {
			mRequest = NULL;
			return false;
		}
		return true;
	}

	bool HttpClient::sendReqBody( const AsyncBuffer& buf ) {
		AsyncBufferS bufs;
		bufs.push_back(buf);
		return sendReqBody(bufs);
	}

	bool HttpClient::sendReqBody( const AsyncBufferS& bufs ) {
        return sendBody(bufs);
	}

	bool HttpClient::endRequest( ) {
        return endSend();
	}

	bool HttpClient::getResponse( ) {
		return beginRecv();
	}

	bool HttpClient::recvRespBody( AsyncBuffer& buf) {
		AsyncBufferS bufs(1,buf);
		return recvRespBody( bufs );
	}

	bool HttpClient::recvRespBody( AsyncBufferS& bufs ) {
		return recvBody(bufs);
	}

	void HttpClient::onSocketConnected() {
		if(!mRequest)
			return;
		HttpMessagePtr msg = mRequest;
		mRequest = NULL;
        beginSend(msg);
	}

	//////////////////////////////////////////////////////////////////////////
	//SimpleHttpClient
	SimpleHttpClient::SimpleHttpClient()
	:mBodySent(false){
		mBodyBuffer.len = 8192;
		mBodyBuffer.base = (char*)malloc(sizeof(char) * mBodyBuffer.len);		
		mResponseComplete = false;
		mResponseError = false;
	}

	SimpleHttpClient::~SimpleHttpClient(){
		free(mBodyBuffer.base);
		mBodyBuffer.len = 0;
	}
	 
	bool SimpleHttpClient::doHttp( HttpMessagePtr msg, const std::string& body, const std::string& ip, unsigned short port ) {
		msg->contentLength(body.length());
		if(!beginRequest(msg, ip,port))
			return false;
		mReqBody = body;
		return true;
	}

	void SimpleHttpClient::onReqMsgSent( size_t size) {
		if(!mBodySent && !mReqBody.empty()) {
			AsyncBuffer buf;
			buf.base = (char*)mReqBody.c_str();
			buf.len = mReqBody.length();
			mBodySent = true;
			sendReqBody(buf);
		} else if(mBodySent && !mReqBody.empty()) {
			mReqBody.clear();
			mRequest = NULL;
			endRequest();
		} else {
			getResponse();
		}
	}

	void SimpleHttpClient::onHttpDataReceived( size_t size ) {
		if( mResponseComplete || mResponseError ) {
			return;
		}
		recvRespBody(mBodyBuffer);
	}

	bool SimpleHttpClient::onHttpMessage( const HttpMessagePtr msg) {
		mResponse = msg;
		return true;
	}

	bool SimpleHttpClient::onHttpBody( const char* data, size_t size) {
		mRespBody.append(data,size);
		return true;
	}

	void SimpleHttpClient::onHttpComplete() {
		mResponseComplete = true;
		onSimpleHttpResponse(mResponse, mRespBody);
	}


	void SimpleHttpClient::onHttpError( int error ) {
		mResponseError = true;
		onSimpleHttpError(error);
	}

	SimpleHttpClientPtr SimpleHttpClient::create(){
		return new SimpleHttpClient();
	}

	//////////////////////////////////////////////////////////////////////////
	
	HttpServant::HttpServant( HttpServer& server, SOCKET socket, ZQ::common::Log& logger)
	:HttpProcessor(false,socket),
	Timer(Socket::getLoop()),
	mServer(server),
	mHandler(0),
	mLastTouch(0),
	mOutstandingRequests(0),
	mLogger(logger){
		mHeaderComplete = false;
	}

	HttpServant::~HttpServant() {
		int64 tsStart = ZQ::common::now();
		Socket::close();
		int64 delta = ZQ::common::now() - tsStart;
		mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant, "took [%ld]ms to close socket"), delta);
	}

	void HttpServant::reset( IHttpHandler::Ptr	handler) {
		mHandler = handler;
	}

	void HttpServant::clear( ) {
		mServer.removeServant(this);
		//FIXME: close socket here
		//        FIX these code if we want to support keep-alive or pipeline
		// Socket::close();
	}

	void HttpServant::initHint() {
		std::string ip;
		unsigned short port = 0;
		getPeerAddress(ip,port);
		std::ostringstream oss;
		oss<<"["<<ip<<":"<<port<<"]";
		mHint = oss.str();
	}

	bool HttpServant::start( ) {
		if(!beginRecv())
			return false;
		return true;
	}
	
	void HttpServant::onSocketConnected() {
		if(!start() ) {
			mLogger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpServant,"%s failed to start receiving data"), mHint.c_str());
			return;
		}
		mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant,"%s start to receive data"),mHint.c_str());
		mServer.updateServant(this);
	}

	void HttpServant::errorResponse( int code ) {
		mLogger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpServant,"errorResponse, code %d"), code);
		HttpMessagePtr msg = mServer.makeSimpleResponse(code);
#ifdef ZQ_OS_LINUX
        beginSend_direct(msg);
#else
        beginSend( msg );
#endif
		Socket::close();
	}
	
	void HttpServant::onHttpError( int err ) {
		std::string locip="";
		unsigned short locport=0;
		getLocalAddress(locip, locport);
		std::string peerip="";
		unsigned short  peerport=0;
		getPeerAddress(peerip, peerport);
        mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant, "onHttpError [%p] [%s:%hu => %s:%hu], error[%s]"), 
				this, locip.c_str(), locport, peerip.c_str(), peerport, ErrorCodeToStr((ErrorCode)err));
		if(mHandler)
			mHandler->onHttpError(err);
		clear();
	}

    void HttpServant::onWritable()
    {
        if(!mHandler) {			
            return;
        }

        mHandler->onWritable();
    }

	void HttpServant::onHttpDataSent( size_t size) {
		if(!mHandler) {			
			return;
		}
		std::string locip="";
		unsigned short locport=0;
		getLocalAddress(locip, locport);
		std::string peerip="";
		unsigned short  peerport=0;
		getPeerAddress(peerip, peerport);
        mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant, "onHttpDataSent [%p] [%s:%hu==>%s:%hu] size[%d]."), this, locip.c_str(), locport, peerip.c_str(), peerport, (int)size);

		mHandler->onHttpDataSent(size);
	}

	void HttpServant::onHttpEndSent( bool keepAlive ) {
		if( --mOutstandingRequests > 0 )
			return; //ignore keepAlive ?

		if(!keepAlive)
			clear();
	}

	void HttpServant::onHttpDataReceived( size_t size ) {
		// NOTE something here
		if(mHandler) {
			mHandler->onHttpDataReceived(size);
		}
		start();//this may fail because a receiving call has been fired		
	}

	bool HttpServant::onHttpMessage( const HttpMessagePtr msg) {
		mHeaderComplete = true;
		mHandler = mServer.getHandler( msg->url(), HttpProcessor::Ptr::dynamicCast(this) );
		if(!mHandler) {
			//should make a 404 response
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant,"onHttpMessage failed to find a suitable handle to process url: %s"),
					msg->url().c_str() );
			errorResponse(404);
			close();
			clear();
			return false;
		} else {
			if(! mHandler->onHttpMessage(msg) ) {
				mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant,"onHttpMessage, user code return false in onHttpMessage, may user code want to abort the procedure, url:%s"), msg->url().c_str());
				mHandler = NULL;
				return false;
			}
			mOutstandingRequests ++;
			return true;
		}		
	}

	bool HttpServant::onHttpBody( const char* data, size_t size) {
		if(!mHandler) {
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant,"http body received, but no handler is ready"));
			errorResponse(500);
			return false;
		}
		if(!mHandler->onHttpBody(data, size) ) {
			mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant,"handler refuse to continue after onHttpBody"));
			errorResponse(500);
			mHandler = NULL;
			return false;
		}
		return true;
	}

	void HttpServant::onHttpComplete() {
		if(!mHandler)
			return;
		mHandler->onHttpComplete();
	}

	void HttpServant::onTimer() {

	}


	///Http Server part
	HttpServer::HttpServer( const HttpServerConfig& conf, ZQ::common::Log& logger )
	:Socket(httpClientCenter.getLoop()),
	mConfig(conf),
	mLogger(logger) {
	}

	HttpServer::~HttpServer() {
	}

	HttpServer::Ptr	HttpServer::create( const HttpServerConfig& conf, ZQ::common::Log& logger ) {
		return new HttpServer(conf,logger);
	}

	bool HttpServer::addRule( const std::string& ruleStr, IHttpHandlerFactory::Ptr factory ) {
		UrlRule rule;		
 		try {
 			rule.re.assign(ruleStr);
 		}
 		catch( const boost::regex_error& ) {
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"failed to add [%s] as url rule"), ruleStr.c_str());
 			return false;
 		}
		rule.rule = ruleStr;
		rule.factory = factory;
		mUrlRules.push_back(rule);
		return true;
	}

	IHttpHandler::Ptr HttpServer::getHandler( const std::string& uri, HttpProcessor::Ptr conn ) {
		IHttpHandlerFactory::Ptr factory = NULL;
		std::vector<UrlRule>::const_iterator it = mUrlRules.begin();
		for( ; it != mUrlRules.end(); it ++ ) {
			if(boost::regex_match(uri,it->re))  {
				factory = it->factory;
				break;
			}
		}
		if(!factory) {
			return NULL;
		}
		return factory->create(conn );
	}

	bool HttpServer::startAt( const std::string& ip, unsigned short port) {
		if(!bind(ip,port)) {
			mLogger(ZQ::common::Log::L_ERROR,CLOGFMT(HttpServer,"failed to start server at[%s:%hu]"),
					ip.c_str(), port );
			return false;
		}
		if(!setReuseAddr(true))
			return false;
		return accept();
	}

	bool HttpServer::startAt( const std::string& ip, const std::string& port) {
		unsigned short usport = 0;
		sscanf(port.c_str(),"%hu", &usport);
		return startAt(ip,usport);
	}
	
	void HttpServer::onSocketError(int err) {
		// should I ignore this ?
		// or restart listen socket ?
		mLogger(ZQ::common::Log::L_ERROR,CLOGFMT(HttpServer,"onSocketError, listen socket encounter an error [%d]"), err);
	}

	HttpMessagePtr HttpServer::makeSimpleResponse( int code ) const {
		HttpMessagePtr msg = new HttpMessage(HTTP_RESPONSE);
		msg->code(code);
		msg->status(HttpMessage::code2status(code));
		msg->header("Server", mConfig.defaultServerName );
		msg->header("Date",HttpMessage::httpdate());
		return msg;
	}

	Socket::Ptr HttpServer::onSocketAccepted( SOCKET sock ) {
		HttpServant::Ptr s = new HttpServant(*this, sock, mLogger);
		s->initHint();
		mLogger(ZQ::common::Log::L_INFO, CLOGFMT(httpServer,"comes a new connection from [%s]"),s->hint().c_str());
		return s;
	}
	
	void HttpServer::updateServant( HttpServant::Ptr servant ) {
		ZQ::common::MutexGuard gd(mLocker);
		mServants.insert( servant );
	}

	void HttpServer::removeServant( HttpServant::Ptr servant ) {
		ZQ::common::MutexGuard gd(mLocker);
		mServants.erase(servant);
	}

}//namespace LibAsync
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

