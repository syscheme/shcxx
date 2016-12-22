#include "httpServer.h"


// ---------------------------------------
// class HttpProcessor
// ---------------------------------------
HttpProcessor::HttpProcessor(bool clientSide)
		:mParseType(clientSide?HTTP_RESPONSE:HTTP_REQUEST),
		mHttpParser(mParseType),
		mbOutgoingKeepAlive(true),
//		mbSending(false),
		mSendCount(0),
		mIncomingMsg(NULL)

{
	reset();
}


HttpProcessor::~HttpProcessor() {
}

void HttpProcessor::reset(ParserCallback* p ) {
	if(!p)
		p = dynamic_cast<ParserCallback*>(this);
	mHttpParser.reset(p);
}

int HttpProcessor::send(const char* buf,size_t len)
{
	mSendCount++;
	return write(buf,len);
}


void HttpProcessor::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		fprintf(stderr, "Read error %s\n",  Error(nread).err_name());
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

void HttpProcessor::OnWrote(ElpeError status)
{
//	mbSending = false;
	mSendCount--;
	if (status != elpeSuccess)
	{
		fprintf(stderr, "send error %s\n",  Error(status).str()); 
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
// class HttpServant
// ---------------------------------------
HttpServant::HttpServant(HttpServer* server,ZQ::common::Log& logger)
		:m_server(server),
		HttpProcessor(false),
		mbError(false),
		mHandler(0),
		mLastTouch(0),
		mLogger(logger)
{

}

HttpServant::~HttpServant()
{

}

bool HttpServant::start( )
{
	read_start();
	initHint();
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant,"%s start to receive data"),mHint.c_str());
	m_server->addServant(this);
	return true;
}

void HttpServant::initHint() {
	char ip[17] = {0};
	int port = 0;
	getpeerIpPort(ip,port);
	std::ostringstream oss;
	oss<<"["<<ip<<":"<<port<<"]";
	mHint = oss.str();
}

void HttpServant::clear( ) {
	m_server->removeServant(this);
	close();
	delete this;
}

void HttpServant::onSocketConnected() {
	if(!start() ) {
		mLogger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpServant,"%s failed to start receiving data"), mHint.c_str());
		return;
	}
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant,"%s start to receive data"),mHint.c_str());
}

void HttpServant::errorResponse( int code ) {
	mLogger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpServant,"errorResponse, code %d"), code);
	HttpMessage::Ptr msg = m_server->makeSimpleResponse(code);
	mbError = true;
	std::string response = msg->toRaw();
	send(response.c_str(),response.length());
}

void HttpServant::onHttpError( int err ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getlocaleIpPort(locip,peerport);

	mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant, "onHttpError [%p] [%s:%d => %s:%d], error[%s]"), 
		this, locip, locport, peerip, peerport, ErrorCodeToStr((ErrorCode)err));
	if(mHandler)
		mHandler->onHttpError(err);

	if(getSendCount() <= 0)
		clear();
}

void HttpServant::onWritable()
{
	if(!mHandler) {			
		return;
	}

	mHandler->onWritable();
}

void HttpServant::onHttpDataSent( bool keepAlive ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getlocaleIpPort(locip,peerport);
	mLogger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServant, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);

	if(mHandler != NULL) {			
		mHandler->onHttpDataSent(keepAlive);
	}

	if(!keepAlive && getSendCount() <= 0)
		clear();

}


void HttpServant::onHttpDataReceived( size_t size ) {
	// NOTE something here
	if(mHandler) {
		mHandler->onHttpDataReceived(size);
	}
	//start();//this may fail because a receiving call has been fired		
}

bool HttpServant::onHttpMessage( const HttpMessage::Ptr msg) {
	mHeaderComplete = true;
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpServant,"onHttpMessage, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
		errorResponse(505);
		return false;
	}
	mHandler = m_server->getHandler( msg->url(), this);
	if(!mHandler) {
		//should make a 404 response
		mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant,"onHttpMessage failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
		//close();
		//clear();
		return false;
	} else {
		if(! mHandler->onHttpMessage(msg) ) {
			mLogger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServant,"onHttpMessage, user code return false in onHttpMessage, may user code want to abort the procedure, url:%s"), msg->url().c_str());
			mHandler = NULL;
			return false;
		}
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

void HttpServer::OnConnection_cb(ElpeError status)
{
	if (status != ElpeError::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n", Error(status).str());
		return;
	}

	HttpServant* client = new HttpServant(this,mLogger);
	client->init(get_loop());

	if (accept((Stream*)client) == 0) {

		client->start();
		mLogger(ZQ::common::Log::L_INFO, CLOGFMT(httpServer,"comes a new connection from [%s]"),client->hint().c_str());
		
	}
	else {
		client->clear();
	}

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

IHttpHandler::Ptr HttpServer::getHandler( const std::string& uri, HttpProcessor* conn)
{
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

void HttpServer::addServant( HttpServant* servant )
{
	ZQ::common::MutexGuard gd(mLocker);
	mServants.insert( servant );
}

void HttpServer::removeServant( HttpServant* servant )
{
	ZQ::common::MutexGuard gd(mLocker);
	mServants.erase(servant);
}


HttpMessage::Ptr HttpServer::makeSimpleResponse( int code ) const {
	HttpMessage::Ptr msg = new HttpMessage(HTTP_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	msg->header("Server", mConfig.defaultServerName );
	msg->header("Date",HttpMessage::httpdate());
	return msg;
}