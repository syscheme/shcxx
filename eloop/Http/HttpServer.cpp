#include "httpServer.h"


// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(HttpServer& server,ZQ::common::Log& logger)
		:_server(server),
		HttpConnection(false),
		_bError(false),
		_Handler(0),
		_LastTouch(0),
		_Logger(logger)
{

}

HttpPassiveConn::~HttpPassiveConn()
{

}

bool HttpPassiveConn::start( )
{
	read_start();
	initHint();
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"%s start to receive data"),_Hint.c_str());
	_server.addConn(this);
	return true;
}

void HttpPassiveConn::initHint() {
	char ip[17] = {0};
	int port = 0;
	getpeerIpPort(ip,port);
	std::ostringstream oss;
	oss<<"["<<ip<<":"<<port<<"]";
	_Hint = oss.str();
}

void HttpPassiveConn::clear( ) {
	_server.delConn(this);
	close();
	delete this;
}

void HttpPassiveConn::errorResponse( int code ) {
	_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpPassiveConn,"errorResponse, code %d"), code);
	HttpMessage::Ptr msg = _server.makeSimpleResponse(code);
	_bError = true;
	std::string response = msg->toRaw();
	send(response.c_str(),response.length());
}

void HttpPassiveConn::onHttpError( int err ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(locip,peerport);

	//_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn, "onHttpError [%p] [%s:%d => %s:%d], error[%s]"), 
//		this, locip, locport, peerip, peerport, ErrorCodeToStr((ErrorCode)err));
	if(_Handler)
		_Handler->onHttpError(err);

	if(getSendCount() <= 0)
		clear();
}

void HttpPassiveConn::onWritable()
{
	if(!_Handler) {			
		return;
	}

	_Handler->onWritable();
}

void HttpPassiveConn::onHttpDataSent( bool keepAlive ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(locip,peerport);
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);

	if(_Handler != NULL) {			
		_Handler->onHttpDataSent(keepAlive);
	}

	if(!keepAlive && getSendCount() <= 0)
		clear();

}


void HttpPassiveConn::onHttpDataReceived( size_t size ) {
	// NOTE something here
	if(_Handler) {
		_Handler->onHttpDataReceived(size);
	}
	//start();//this may fail because a receiving call has been fired		
}

bool HttpPassiveConn::onHttpMessage( const HttpMessage::Ptr msg) {
	_HeaderComplete = true;
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHttpMessage, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
		errorResponse(505);
		return false;
	}
	_Handler = _server.getHandler( msg->url(), *this);
	if(!_Handler) {
		//should make a 404 response
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHttpMessage failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
		//close();
		//clear();
		return false;
	} else {
		if(! _Handler->onHttpMessage(msg) ) {
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHttpMessage, user code return false in onHttpMessage, may user code want to abort the procedure, url:%s"), msg->url().c_str());
			_Handler = NULL;
			return false;
		}
		return true;
	}		
}

bool HttpPassiveConn::onHttpBody( const char* data, size_t size) {

	if(!_Handler) {
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"http body received, but no handler is ready"));
		errorResponse(500);
		return false;
	}
	if(!_Handler->onHttpBody(data, size) ) {
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"handler refuse to continue after onHttpBody"));
		errorResponse(500);
		_Handler = NULL;
		return false;
	}
	return true;
}

void HttpPassiveConn::onHttpComplete() {
	if(!_Handler)
		return;
	_Handler->onHttpComplete();
}

// ---------------------------------------
// class HttpServer
// ---------------------------------------
HttpServer::HttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger)
		:_Config(conf),
		_Logger(logger)
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

	HttpPassiveConn* client = new HttpPassiveConn(*this,_Logger);
	client->init(get_loop());

	if (accept((Stream*)client) == 0) {

		client->start();
		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(httpServer,"comes a new connection from [%s]"),client->hint().c_str());
		
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
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"failed to add [%s] as url uriEx"), ruleStr.c_str());
		return false;
	}
	uriEx.uriEx = ruleStr;
	uriEx.app = app;
	_uriMounts.push_back(uriEx);
	return true;
}

IHttpHandler::Ptr HttpServer::getHandler( const std::string& uri, HttpConnection& conn)
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

	return app->create(conn);
}

void HttpServer::addConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(_Locker);
	_PassiveConn.insert( servant );
}

void HttpServer::delConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(_Locker);
	_PassiveConn.erase(servant);
}


HttpMessage::Ptr HttpServer::makeSimpleResponse( int code ) const {
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::HTTP_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	msg->header("Server", _Config.serverName );
	msg->header("Date",HttpMessage::httpdate());
	return msg;
}