#include "httpServer.h"


namespace ZQ {
namespace eloop {
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

void HttpPassiveConn::onParseError( int err ) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(locip,peerport);

	//_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn, "onParseError [%p] [%s:%d => %s:%d], error[%s]"), 
//		this, locip, locport, peerip, peerport, ErrorCodeToStr((ErrorCode)err));
	if(_Handler)
		_Handler->onParseError(err);

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

bool HttpPassiveConn::onHeadersEnd( const HttpMessage::Ptr msg) {
	_HeaderComplete = true;
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHeadersEnd, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
		errorResponse(505);
		return false;
	}
	_Handler = _server.getHandler( msg->url(), *this);
	if(!_Handler) {
		//should make a 404 response
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
		//close();
		//clear();
		return false;
	} else {
		if(! _Handler->onHeadersEnd(msg) ) {
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd, user code return false in onHeadersEnd, may user code want to abort the procedure, url:%s"), msg->url().c_str());
			_Handler = NULL;
			return false;
		}
		return true;
	}		
}

bool HttpPassiveConn::onBodyData( const char* data, size_t size) {

	if(!_Handler) {
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"http body received, but no handler is ready"));
		errorResponse(500);
		return false;
	}
	if(!_Handler->onBodyData(data, size) ) {
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"handler refuse to continue after onBodyData"));
		errorResponse(500);
		_Handler = NULL;
		return false;
	}
	return true;
}

void HttpPassiveConn::onMessageCompleted() {
	if(!_Handler)
		return;
	_Handler->onMessageCompleted();
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
	//stop();
}
/*
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
*/
bool HttpServer::registerApp( const std::string& ruleStr, HttpBaseApplication::Ptr app ) {
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
	HttpBaseApplication::Ptr app = NULL;

	// cut off the paramesters
	std::string uriWithnoParams = uri;
	size_t pos = uriWithnoParams.find_first_of("?#");
	if (std::string::npos != pos)
		uriWithnoParams = uriWithnoParams.substr(0, pos);

	std::vector<UriMount>::const_iterator it = _uriMounts.begin();
	for( ; it != _uriMounts.end(); it ++ )
	{
		if(boost::regex_match(uriWithnoParams, it->re))
		{
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



// ---------------------------------------
// class SingleLoopHttpServer
// ---------------------------------------
// Single event loop

SingleLoopHttpServer::SingleLoopHttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger)
			:HttpServer(conf,logger)
{
	init(_loop);
}
SingleLoopHttpServer::~SingleLoopHttpServer()
{

}
bool SingleLoopHttpServer::startAt( const char* ip, int port)
{
	if (bind4(ip,port) < 0)
		return false;

	if (listen() < 0)
		return false;
	return _loop.run(ZQ::eloop::Loop::Default);
}
void SingleLoopHttpServer::stop()
{
	close();
}

void SingleLoopHttpServer::doAccept(ElpeError status)
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
		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(SingleLoopHttpServer,"comes a new connection from [%s]"),client->hint().c_str());

	}
	else {
		client->clear();
	}
}

// ---------------------------------------
// class MultipleLoopHttpServer
// ---------------------------------------
//Multiple event loop
MultipleLoopHttpServer::MultipleLoopHttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger,int threadCount)
			:HttpServer(conf,logger),
			_bRunning(false),
			_threadCount(threadCount),
			_roundCount(0),
			_socket(0)
{
	CpuInfo cpu;
	int cpuCount = cpu.getCpuCount();
	int j = 0;
	setCpuAffinity(j);
	for (int i = 0;i < _threadCount;i++)
	{
		ServantThread *pthread = new ServantThread(*this,_Logger);
		j = (j+1) % cpuCount;
		pthread->setCpuAffinity(j);
		pthread->start();
		_vecThread.push_back(pthread);
	}
}

MultipleLoopHttpServer::~MultipleLoopHttpServer()
{
	std::vector<ServantThread*>::iterator iter;
	for (iter=_vecThread.begin();iter!=_vecThread.end();iter++)  
	{  
		(*iter)->close();
		delete *iter;
		_vecThread.erase(iter++);
	}
	_bRunning = false;
}

bool MultipleLoopHttpServer::startAt( const char* ip, int port)
{
	struct sockaddr_in serv;

	serv.sin_family=AF_INET;
	serv.sin_port=htons(port);
	serv.sin_addr.S_un.S_addr=inet_addr(ip);

	WORD sockVersion = MAKEWORD(2,2);
	WSADATA wsaData;
	if(WSAStartup(sockVersion, &wsaData)!=0)
	{
		return 0;
	}

	_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	int reuse_value = 1;
	if( setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) != 0 )
		return false;

	if( _socket < 0 )
		return false;


	if(bind(_socket,(struct sockaddr*)&serv,sizeof(serv)) == -1)
	{
		printf("socket bind error!");
		return false;
	}
	if (0 != listen(_socket,10000))
	{
		return false;
	}
	_bRunning = true;
	return ZQ::common::NativeThread::start();
}

void MultipleLoopHttpServer::stop()
{
	closesocket(_socket);
}

int MultipleLoopHttpServer::run()
{
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpServer,"server start"));
	struct sockaddr_storage addr;
	while( _bRunning ) {
		socklen_t size= (socklen_t)sizeof( addr );
		int sock = accept( _socket, (struct sockaddr*)&addr, &size);
		if( sock < 0 )
			break;

		ServantThread* pthread = _vecThread[_roundCount];
		pthread->addSocket(sock);
		pthread->send();
		_roundCount = (_roundCount + 1) % _threadCount;
	}
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpServer,"server quit"));
	return 0;
}

// ------------------------------------------------
// class ServantThread
// ------------------------------------------------
ServantThread::ServantThread(HttpServer& server,ZQ::common::Log& logger)
		:_Logger(logger),
		_server(server)
{
	_loop = new Loop(false);
}

ServantThread::~ServantThread()
{
	if (_loop != NULL)
	{
		delete _loop;
	}
}

Loop& ServantThread::getLoop()
{
	return *_loop;
}

void ServantThread::addSocket(int sock)
{

	ZQ::common::MutexGuard gd(_LockSocket);
	_ListSocket.push_back(sock);
}


int ServantThread::run(void)
{
	ZQ::eloop::Async::init(*_loop);
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"thread start run! ThreadId = %d"),id());
	_loop->run(Loop::Default);
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"thread quit! ThreadId = %d"),id());
	return 0;
}

void ServantThread::OnAsync(void* data)
{
	printf("Async test\n");
	ZQ::common::MutexGuard gd(_LockSocket);
	while( !_ListSocket.empty())
	{
		HttpPassiveConn* client = new HttpPassiveConn(_server,_Logger);
		int r = client->init(get_loop());
		if (r != 0)
		{
			//printf("tcp init error:%s,name :%s\n", uv_strerror(r), uv_err_name(r));
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(ServantThread,"OnAsync tcp init error:%s"),Handle::errDesc(r));
		}

		int sock = _ListSocket.back();
		_ListSocket.pop_back();
		printf("OnAsync sock = %d\n", sock);
		r = client->connected_open(sock);
		if (r != 0)
		{
			printf("open error! r = %d\n", r);
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(ServantThread,"OnAsync open socke error: %s"),Handle::errDesc(r));
		}

		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(httpServer,"comes a new connection from [%s]"),client->hint().c_str());
		client->start();
	}
}


} }//namespace ZQ::eloop
