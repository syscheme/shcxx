#include <assert.h>
#include <vector>
#include <Locks.h>
#include <TimeUtil.h>
#include <urlstr.h>
#include "http.h"

namespace LibAsync {
	
	
	EventLoopCenter::EventLoopCenter():mIdxLoop(0){
	}

	EventLoopCenter::~EventLoopCenter() {
		stopCenter();
	}

	bool EventLoopCenter::startCenter( ZQ::common::Log& log, const std::vector<int>& cpuIds) {
		if( cpuIds.size() <= 0) {
			return false;
		}
		ZQ::common::MutexGuard gd(mLocker);
		if( mLoops.size() > 0 )
			return true;
		for( size_t i = 0; i < cpuIds.size(); i ++ ) {
			log(ZQ::common::Log::L_INFO, CLOGFMT(EventLoopCenter,"start event loop at cpu core[%d]"), cpuIds[i]);
			EventLoop* l = new EventLoop(log, cpuIds[i], this, cpuIds[i]);
			if(!l->start()){
				delete l;
				return false;
			}
			LoopInfo info;
			info.loop = l;
			mLoops.push_back(info);
		}
		mTmpLoops.reserve( mLoops.size() );

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
			mLoops[i].loop->stop();
			delete mLoops[i].loop;
		}
		mLoops.clear();
	}

	void EventLoopCenter::addSocket(int id) {
		ZQ::common::MutexGuard gd(mLocker);
		mLoops[id].sockCount ++;
	}

	void EventLoopCenter::removeSocket( int id ) {
		ZQ::common::MutexGuard gd(mLocker);
		mLoops[id].sockCount --;
	}

	///从Center里面获取一个EventLoop，当前的实现版本是roundrobin
	EventLoop& EventLoopCenter::getLoop(){
		assert(mLoops.size() > 0);
		{
			ZQ::common::MutexGuard gd(mLocker);
			mTmpLoops = mLoops;
		}
		std::sort( mTmpLoops.begin(), mTmpLoops.end());
		EventLoop* loop = mTmpLoops[0].loop;
		loop->increateSockCount();
		return *loop;
	}

	
	bool HttpProcessor::setup(ZQ::common::Log&log, const std::vector<int>& cpuIds){
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
		return httpClientCenter.startCenter(log, cpuIds);
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

		BufferHelper bh(bufs);
		size_t sentSize = 0;
		int rc = 0;
		while (bh.size() > 0) {
			rc = sendChunk(bh);
			if (rc < 0) {
				break;
			}
			sentSize += (size_t)rc;
		}
		if (sentSize == 0)
			return rc;
		else
			return (int)sentSize;
	}

	size_t findPosInBufferS(const AsyncBufferS& bufs, size_t& size) {
		size_t idx = 0;
		for (; idx < bufs.size(); idx++) {
			if (size < bufs[idx].len)
				break;
			size -= bufs[idx].len;
		}
		return idx;
	}

	int HttpProcessor::sendChunk( BufferHelper& bh ) {
		AsyncBufferS bufs = bh.getBuffers();
		mWritingBufs.clear();

		switch (mSendingChunkState) {
			case SENDING_CHUNK_NULL:
				//fallthrough
			case SENDING_CHUNK_HEADER:
				{		
					if (mLastUnsentBytes == 0) {
						mWritingBufs = bh.getBuffers();
						mChunkHeader.len = sprintf(mChunkHeader.base, "%x\r\n", (unsigned int)bh.size());
						mWritingBufs.insert(mWritingBufs.begin(), mChunkHeader);
						mWritingBufs.push_back(chunkTail);
						mLastUnsentBodyBytes = bh.size();
					} else {
						AsyncBuffer chunkHeader(mChunkHeader);
						assert(mLastUnsentBytes <= chunkHeader.len);
						chunkHeader.base += (chunkHeader.len - mLastUnsentBytes);
						chunkHeader.len = mLastUnsentBytes;
						mWritingBufs.insert(mWritingBufs.begin(), chunkHeader);
						if (mLastUnsentBodyBytes <= bh.size()) {
							AsyncBufferS bodyBufs = bh.getAt(mLastUnsentBodyBytes);
							mWritingBufs.insert(mWritingBufs.end(), bodyBufs.begin(), bodyBufs.end());
							mWritingBufs.push_back(chunkTail);
						} else {
							mWritingBufs.insert(mWritingBufs.end(), bufs.begin(), bufs.end());
						}
					}
					break;
				}

			case SENDING_CHUNK_BODY:
				{
					assert(mLastUnsentBodyBytes != 0);
					assert(mLastUnsentBodyBytes == mLastUnsentBytes);
					if (mLastUnsentBodyBytes <= bh.size()) {
						mWritingBufs = bh.getAt(mLastUnsentBodyBytes);
						mWritingBufs.push_back(chunkTail);
					} else {
						mWritingBufs = bufs;
					}
					break;
				}

			case SENDING_CHUNK_TAIL:
				{
					assert(mLastUnsentBodyBytes == 0);
					assert(mLastUnsentBytes != 0 && mLastUnsentBytes <= chunkTail.len);
					AsyncBuffer bufTail(chunkTail);
					bufTail.base += (bufTail.len - mLastUnsentBytes);
					bufTail.len = mLastUnsentBytes;
					mWritingBufs.push_back(bufTail);
					break;
				}
			default:
				{
					assert(false && "bad logic");
					break;
				}
		}

		mbSending = true;
		int rc = sendDirect(mWritingBufs);
		mbSending = false;

		if (rc < 0) {
			return rc;
		} else if (rc == 0 )  {
			return ERR_BUFFERTOOBIG;
		}

		size_t res = (size_t)rc;
		size_t idx = findPosInBufferS(mWritingBufs, res);

		switch (mSendingChunkState) 
		{
			case SENDING_CHUNK_NULL:
			case SENDING_CHUNK_HEADER:
				{
					// size of mWritingBufs should be 2 or 3
					switch (idx)
					{
						case 0:
							{
								assert(res < mWritingBufs[0].len);
								mLastUnsentBytes = mWritingBufs[0].len - res;
								mSendingChunkState = SENDING_CHUNK_HEADER;
								rc = 0;
								break;
							}
						case 1:
							{
								if (res > 0) {
									assert(res < mWritingBufs[1].len);
									assert(mLastUnsentBodyBytes >= mWritingBufs[1].len);
								}
								mLastUnsentBodyBytes -= res;
								mLastUnsentBytes = mLastUnsentBodyBytes;
								mSendingChunkState = SENDING_CHUNK_BODY;
								rc -= (int)mWritingBufs[0].len;
								break;
							}
						case 2:
							{
								if (res > 0) {
									assert(res < mWritingBufs[2].len);
									assert(mLastUnsentBodyBytes == mWritingBufs[1].len);
								}
								mLastUnsentBodyBytes = 0;
								mLastUnsentBytes = mWritingBufs[2].len - res;
								mSendingChunkState = SENDING_CHUNK_TAIL;
								rc = (int)mWritingBufs[1].len;//body length
								break;
							}
						case 3:
							{
								assert(res == 0);
								mLastUnsentBodyBytes = 0;
								mLastUnsentBytes = 0;
								mSendingChunkState = SENDING_CHUNK_NULL;
								rc = (int)mWritingBufs[1].len;
								break;
							}
						default:
							assert(false && "bad logic");
					}
					break;
				}
			case SENDING_CHUNK_BODY: 
				{
					// size of mWritingBufs should be 1 or 2
					switch (idx) 
					{
						case 0: 
							{
								assert(mLastUnsentBodyBytes > res);
								mLastUnsentBodyBytes -= res;
								mLastUnsentBytes = mLastUnsentBodyBytes;
								mSendingChunkState = SENDING_CHUNK_BODY;
								rc = (int)res;
								break;
							}
						case 1:
							{
								if (res > 0) {
									assert(res < mWritingBufs[1].len);
									mLastUnsentBytes = mWritingBufs[1].len - res;
								} else {
									mLastUnsentBytes = chunkTail.len;
								}			
								mLastUnsentBodyBytes = 0;
								mSendingChunkState = SENDING_CHUNK_TAIL;
								rc = (int)mWritingBufs[0].len;
								break;
							}
						case 2: 
							{
								assert(res == 0);
								mLastUnsentBodyBytes = 0;
								mLastUnsentBytes = 0;
								mSendingChunkState = SENDING_CHUNK_NULL;
								rc = (int)mWritingBufs[0].len;
								break;
							}
						default:
							{
								assert(false && "bad logic");
							}
					}
					break;
				}
			case SENDING_CHUNK_TAIL: 
				{
					// size of mWritingBufs should be 1
					switch (idx) 
					{
						case 0:
							{
								assert(mWritingBufs[0].len > res);
								mLastUnsentBytes = mWritingBufs[0].len - res;
								mLastUnsentBodyBytes = 0;
								mSendingChunkState = SENDING_CHUNK_TAIL;
								rc = 0;
								break;
							}
						case 1:
							{
								assert(res == 0);
								mLastUnsentBodyBytes = 0;
								mLastUnsentBytes = 0;
								mSendingChunkState = SENDING_CHUNK_NULL;
								rc = 0;
								break;
							}
						default:
							{
								assert(false && "bad logic");
								break;
							}
					}
					break;

				}
			default:
				{
					assert(false && "bad logic");
					break;
				}
		}
		if (rc > 0)
			bh.adjust(rc);
		return rc;
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

		mWritingBufs.clear();
		switch(mSendingChunkState) {
			case SENDING_CHUNK_TAIL: {
				assert(mLastUnsentBytes <= 2);//must <= size of chunktail data
				mChunkHeader.len = sprintf(mChunkHeader.base ,"\r\n0\r\n");
				AsyncBuffer bufHeader(mChunkHeader);
				bufHeader.base += 2-mLastUnsentBytes;
				bufHeader.len -= (2-mLastUnsentBytes);
				mWritingBufs.push_back(bufHeader);
				mWritingBufs.push_back(chunkTail);
				break;
			 }
			case SENDING_CHUNK_NULL:
				assert( mLastUnsentBytes == 0 );
			case SENDING_CHUNK_ZERO: {
				if(mLastUnsentBytes == 0 ) {
					mChunkHeader.len = sprintf(mChunkHeader.base,"0\r\n");
					mWritingBufs.push_back(mChunkHeader);
					mWritingBufs.push_back(chunkTail);
				} 
				break;
			 }
			default:
				 return ERR_ERROR;
				break;
		}
		size_t expectSize = buffer_size(mWritingBufs);
        mbSending = true;
        int res = sendDirect(mWritingBufs);
        mbSending =  false;
		if( res < 0 )
			return res;
		else if (res == 0 )
			return ERR_BUFFERTOOBIG;
		assert( expectSize >= (size_t)res);
		if( expectSize == (size_t)res) {
			mSendingChunkState = SENDING_CHUNK_NULL;
			mLastUnsentBytes = 0;
			return res;
		} else {
			mLastUnsentBytes = expectSize - (size_t)res;
			mSendingChunkState = SENDING_CHUNK_ZERO;
			res = ERR_EAGAIN;
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
	:Socket(httpClientCenter.getLoop()),
	HttpProcessor(true){
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
	:Socket(httpClientCenter.getLoop()),
	 mBodySent(false){
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
	:Socket(httpClientCenter.getLoop(), socket),
	HttpProcessor(false,socket),
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
		//Socket::close();
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
		if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpServant,"onHttpMessage, unsupport http version[%u/%u], reject"),
					msg->versionMajor(), msg->versionMinor());
			errorResponse(505);
			return false;
		}
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
	mLogger(logger),
	mSocket(-1),
	mbRunning(false){
	}

	HttpServer::~HttpServer() {
		stop();
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

	bool HttpServer::startAt( const std::string& ip, unsigned short port ) {
		SocketAddrHelper addr(ip,port);
		return startAt( addr );
	}

	bool HttpServer::startAt( const std::string& ip, const std::string& port ) {
		SocketAddrHelper addr(ip,port);
		return startAt( addr );
	}

	bool HttpServer::startAt( const SocketAddrHelper& addr ) {
		const struct addrinfo * info = addr.info();
		if(!info)
			return false;
		mSocket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		int reuse_value = 1;
		if( setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) != 0 )
			return false;

		if( mSocket < 0 )
			return false;
		if( 0 != ::bind( mSocket, info->ai_addr, info->ai_addrlen) )
			return false;
		if(0 != ::listen(mSocket,10000) )
			return false;
		mbRunning = true;
		return ZQ::common::NativeThread::start();
	}

	int HttpServer::run( ) {
		mLogger(ZQ::common::Log::L_INFO, CLOGFMT(HttpServer,"server start"));
		struct sockaddr_storage addr;
		while( mbRunning ) {
			socklen_t size= (socklen_t)sizeof( addr );
			int sock = ::accept( mSocket, (struct sockaddr*)&addr, &size);
			if( sock < 0 )
				break;
			Socket::Ptr newSock = onSocketAccepted( sock );
			if( newSock ) {
				newSock->initialServerSocket();
			}
		}
		mLogger(ZQ::common::Log::L_INFO, CLOGFMT(HttpServer,"server quit"));
		return 0;
	}
	void HttpServer::stop() {
		mbRunning = false;
#ifdef ZQ_OS_MSWIN
		::closesocket(mSocket);
#else
		::close(mSocket);
#endif
		waitHandle(-1);
	}


	/**
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
	*/
	
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

