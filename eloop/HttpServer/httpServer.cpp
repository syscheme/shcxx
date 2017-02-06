#include "httpServer.h"


// ---------------------------------------
// class HttpConnection
// ---------------------------------------
HttpConnection::HttpConnection(bool clientSide)
		:mParseType(clientSide?HTTP_RESPONSE:HTTP_REQUEST),
		mHttpParser(mParseType),
		mbOutgoingKeepAlive(true),
//		mbSending(false),
		mSendCount(0),
		mIncomingMsg(NULL)

{
	reset();
}


HttpConnection::~HttpConnection() {
}

void HttpConnection::reset(ParserCallback* p ) {
	if(!p)
		p = dynamic_cast<ParserCallback*>(this);
	mHttpParser.reset(p);
}

int HttpConnection::send(const char* buf,size_t len)
{
	mSendCount++;
	return write(buf,len);
}


void HttpConnection::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		fprintf(stderr, "OnRead() error %s\n",  errName((int)nread));
		onHttpError(ERR_RECVFAIL);
		return;
	}

	std::string str = buf;
	printf("recv data:%s,len = %d,size = %d\n", buf,nread,str.size());

	size_t nparsed = mHttpParser.parse(buf, nread);
	if(nparsed != nread){
		onHttpError(mHttpParser.lastError());
		return;
	}
	if(!mHttpParser.headerComplete()){
		if(mIncomingMsg != NULL)
			assert(false);
		return;
	} 
	mIncomingMsg = mHttpParser.currentHttpMessage();		

	if( mHttpParser.httpComplete()) {
		reset();
	}
	onHttpDataReceived(nread);	
}

void HttpConnection::OnWrote(ElpeError status)
{
//	mbSending = false;
	mSendCount--;
	if (status != elpeSuccess)
	{
		fprintf(stderr, "send error %s\n",  errDesc(status)); 
		onHttpError(ERR_SENDFAIL);
		return;
	}
	
	// if whole http raw message has been sent
	// mOutgoingMsg should be NULL
	onHttpDataSent(mbOutgoingKeepAlive);
/*
	if( mOutgoingMsg == NULL ) {
		onHttpEndSent(mbOutgoingKeepAlive);
	}*/
}

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(HttpServer* server,ZQ::common::Log& logger)
		:_server(server),
		HttpConnection(false),
		mbError(false),
		mHandler(0),
		mLastTouch(0),
		mLogger(logger)
{

}

HttpPassiveConn::~HttpPassiveConn()
{

}

bool HttpPassiveConn::start( )
{
	read_start();
	initHint();
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"%s start to receive data"),mHint.c_str());
	_server->addConn(this);
	return true;
}

void HttpPassiveConn::initHint() {
	char ip[17] = {0};
	int port = 0;
	getpeerIpPort(ip,port);
	std::ostringstream oss;
	oss<<"["<<ip<<":"<<port<<"]";
	mHint = oss.str();
}

void HttpPassiveConn::clear( ) {
	_server->delConn(this);
	close();
	delete this;
}

void HttpPassiveConn::onSocketConnected() {
	if(!start() ) {
		mLogger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn,"%s failed to start receiving data"), mHint.c_str());
		return;
	}
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"%s start to receive data"),mHint.c_str());
}

void HttpPassiveConn::errorResponse( int code ) {
	mLogger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpPassiveConn,"errorResponse, code %d"), code);
	HttpMessage::Ptr msg = _server->makeSimpleResponse(code);
	mbError = true;
	std::string response = msg->toRaw();
	send(response.c_str(),response.length());
}

void HttpPassiveConn::onHttpError( int err ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getlocaleIpPort(locip,peerport);

	mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn, "onHttpError [%p] [%s:%d => %s:%d], error[%s]"), 
		this, locip, locport, peerip, peerport, ErrorCodeToStr((ErrorCode)err));
	if(mHandler)
		mHandler->onHttpError(err);

	if(getSendCount() <= 0)
		clear();
}

void HttpPassiveConn::onWritable()
{
	if(!mHandler) {			
		return;
	}

	mHandler->onWritable();
}

void HttpPassiveConn::onHttpDataSent( bool keepAlive ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getlocaleIpPort(locip,peerport);
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);

	if(mHandler != NULL) {			
		mHandler->onHttpDataSent(keepAlive);
	}

	if(!keepAlive && getSendCount() <= 0)
		clear();

}


void HttpPassiveConn::onHttpDataReceived( size_t size ) {
	// NOTE something here
	if(mHandler) {
		mHandler->onHttpDataReceived(size);
	}
	//start();//this may fail because a receiving call has been fired		
}

bool HttpPassiveConn::onHttpMessage( const HttpMessage::Ptr msg) {
	mHeaderComplete = true;
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHttpMessage, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
		errorResponse(505);
		return false;
	}
	mHandler = _server->getHandler( msg->url(), this);
	if(!mHandler) {
		//should make a 404 response
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHttpMessage failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
		//close();
		//clear();
		return false;
	} else {
		if(! mHandler->onHttpMessage(msg) ) {
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHttpMessage, user code return false in onHttpMessage, may user code want to abort the procedure, url:%s"), msg->url().c_str());
			mHandler = NULL;
			return false;
		}
		return true;
	}		
}

bool HttpPassiveConn::onHttpBody( const char* data, size_t size) {

	if(!mHandler) {
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"http body received, but no handler is ready"));
		errorResponse(500);
		return false;
	}
	if(!mHandler->onHttpBody(data, size) ) {
		mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"handler refuse to continue after onHttpBody"));
		errorResponse(500);
		mHandler = NULL;
		return false;
	}
	return true;
}

void HttpPassiveConn::onHttpComplete() {
	if(!mHandler)
		return;
	mHandler->onHttpComplete();
}

// ---------------------------------------
// class HttpServer
// ---------------------------------------
HttpServer::HttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger)
		:mConfig(conf),
		mLogger(logger)
{

}

HttpServer::~HttpServer()
{
	stop();
}

bool HttpServer::startAt( const char* ip, int port)
{
	if (bind4(ip,port) < 0)
		return false;
	
	if (listen() < 0)
		return false;
	return true;
}

void HttpServer::stop( )
{
	close();
}

void HttpServer::doAccept(ElpeError status)
{
	if (status != elpeSuccess)
	{
		fprintf(stderr, "doAccept()error %s\n", errDesc(status));
		return;
	}

	HttpPassiveConn* client = new HttpPassiveConn(this,mLogger);
	client->init(get_loop());

	if (accept((Stream*)client) == 0) {

		client->start();
		mLogger(ZQ::common::Log::L_INFO, CLOGFMT(httpServer,"comes a new connection from [%s]"),client->hint().c_str());
		
	}
	else {
		client->clear();
	}

}

bool HttpServer::registerApp( const std::string& ruleStr, HttpApplication::Ptr app ) {
	UriMount uriEx;		
	try {
		uriEx.re.assign(ruleStr);
	}
	catch( const boost::regex_error& ) {
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"failed to add [%s] as url uriEx"), ruleStr.c_str());
		return false;
	}
	uriEx.uriEx = ruleStr;
	uriEx.app = app;
	_uriMounts.push_back(uriEx);
	return true;
}

IHttpHandler::Ptr HttpServer::getHandler( const std::string& uri, HttpConnection* conn)
{
	HttpApplication::Ptr app = NULL;
	std::vector<UriMount>::const_iterator it = _uriMounts.begin();
	for( ; it != _uriMounts.end(); it ++ ) {
		if(boost::regex_match(uri,it->re))  {
			app = it->app;
			break;
		}
	}

	if (!app)
		return NULL;

	return app->create(conn );
}

void HttpServer::addConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(mLocker);
	mServants.insert( servant );
}

void HttpServer::delConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(mLocker);
	mServants.erase(servant);
}


HttpMessage::Ptr HttpServer::makeSimpleResponse( int code ) const {
	HttpMessage::Ptr msg = new HttpMessage(HTTP_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	msg->header("Server", mConfig.serverName );
	msg->header("Date",HttpMessage::httpdate());
	return msg;
}