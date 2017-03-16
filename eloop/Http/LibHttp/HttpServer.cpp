#include <ZQ_common_conf.h>
#include "HttpServer.h"
#include <fcntl.h>
#ifdef ZQ_OS_LINUX
#include <signal.h>
#endif

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(HttpServer& server,ZQ::common::Log& logger)
		:_server(server),
		HttpConnection(false,logger),
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

void HttpPassiveConn::errorResponse( int code ) {
	_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpPassiveConn,"errorResponse, code %d"), code);
	HttpMessage::Ptr msg = _server.makeSimpleResponse(code);
	_bError = true;
	std::string response = msg->toRaw();
	write(response.c_str(),response.length());
}

void HttpPassiveConn::onError(int error,const char* errorDescription) {

	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(peerip,peerport);

	_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn, "onError [%p] [%s:%d => %s:%d], errorCode[%d],Description[%s]"), 
		this, locip, locport, peerip, peerport,error,errorDescription);
	if(_Handler)
		_Handler->onError(error,errorDescription);

	shutdown();
}

void HttpPassiveConn::OnClose()
{
	_server.delConn(this);
	_Handler = NULL;
	delete this;
}

void HttpPassiveConn::onHttpDataSent() {

/*	char locip[17] = { 0 };
	int  locport = 0;
	getlocaleIpPort(locip,locport);

	char peerip[17] = { 0 };
	int  peerport = 0;
	getpeerIpPort(peerip,peerport);
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);*/

	if(_Handler != NULL) {			
		_Handler->onHttpDataSent();
	}
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
	_Handler = _server.createHandler( msg->url(), *this);
	if(!_Handler) {
		//should make a 404 response
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
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

bool HttpServer::mount(const std::string& ruleStr, HttpBaseApplication::Ptr app, const HttpHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;
	try {
		dir.re.assign(ruleStr);
	}
	catch( const boost::regex_error& )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), vsite.c_str(), ruleStr.c_str());
		return false;
	}

	dir.uriEx = ruleStr;
	dir.app = app;
	dir.props = props;

	// address the virtual site
	VSites::iterator itSite = _vsites.find(vsite);
	if (_vsites.end() == itSite)
	{
		_vsites.insert(VSites::value_type(vsite, MountDirs()));
		itSite = _vsites.find(vsite);
	}

	itSite->second.push_back(dir);
	return true;
}

HttpHandler::Ptr HttpServer::createHandler(const std::string& uri, HttpConnection& conn, const std::string& virtualSite)
{
	HttpBaseApplication::Ptr app = NULL;

	// cut off the paramesters
	std::string uriWithnoParams = uri;
	size_t pos = uriWithnoParams.find_first_of("?#");
	if (std::string::npos != pos)
		uriWithnoParams = uriWithnoParams.substr(0, pos);

	// address the virtual site
	VSites::const_iterator itSite = _vsites.find(virtualSite);
	if (_vsites.end() == itSite)
		itSite = _vsites.find(DEFAULT_SITE); // the default site

	HttpHandler::Ptr handler;
	MountDirs::const_iterator it = itSite->second.begin();

	for( ; it != itSite->second.end(); it++)
	{
		if(boost::regex_match(uriWithnoParams, it->re))
		{
			if (it->app)
				handler = it->app->create(conn,_Logger,it->props);
			break;
		}
	}
	return handler;
}


void HttpServer::addConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(_Locker);
	_PassiveConn.insert( servant );
	printf("add connect count = %d\n",_PassiveConn.size());
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"add connect count = %d"),_PassiveConn.size());
}

void HttpServer::delConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(_Locker);
	_PassiveConn.erase(servant);
	printf("delete connect count = %d\n",_PassiveConn.size());
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"delete connect count = %d"),_PassiveConn.size());
}


HttpMessage::Ptr HttpServer::makeSimpleResponse( int code ) const {
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_RESPONSE);
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
	_loop.close();
}
bool SingleLoopHttpServer::startAt()
{
	if (bind4(_Config.host.c_str(),_Config.port) < 0)
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
		client->shutdown();
	}
}

// ---------------------------------------
// class MultipleLoopHttpServer
// ---------------------------------------
//Multiple event loop
MultipleLoopHttpServer::MultipleLoopHttpServer( const HttpServerConfig& conf,ZQ::common::Log& logger)
			:HttpServer(conf,logger),
			_bRunning(false),
			_roundCount(0),
			_socket(0)
{
	CpuInfo cpu;
	int cpuCount = cpu.getCpuCount();
	int cpuId = 0;
	setCpuAffinity(cpuId);
	for (int i = 0;i < conf.threadCount;i++)
	{
		ServantThread *pthread = new ServantThread(*this,_Logger);
		
		cpuId++;
		cpuId = cpuId % cpuCount;
		printf("cpuId = %d,cpuCount = %d,mask = %d\n",cpuId,cpuCount,1<<cpuId);
		pthread->setCpuAffinity(cpuId);
		
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
		_vecThread.erase(iter++);
	}
	stop();
}

bool MultipleLoopHttpServer::startAt()
{

#ifdef ZQ_OS_MSWIN
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
	{
		return 0;
	}
#else
	//Ignore SIGPIPE signal
	signal(SIGPIPE, SIG_IGN);
#endif


	_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	int reuse_value = 1;
	if( setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) != 0 )
		return false;

	if( _socket < 0 )
		return false;

	struct sockaddr_in serv;
	serv.sin_family=AF_INET;
	serv.sin_port=htons(_Config.port);
	serv.sin_addr.s_addr=inet_addr(_Config.host.c_str());
	if(bind(_socket,(struct sockaddr*)&serv,sizeof(serv)) == -1)
	{
		printf("socket bind error!");
		return false;
	}
	if (0 != listen(_socket,_Config.maxConns))
	{
		return false;
	}
	_bRunning = true;
	return ZQ::common::NativeThread::start();
}

void MultipleLoopHttpServer::stop()
{
	_bRunning = false;
#ifdef ZQ_OS_MSWIN
	closesocket(_socket);
	//clean WSA env
	WSACleanup(); 
#else
	// stop a blocking call
	shutdown(_socket, SHUT_RDWR);
	close(_socket);
#endif
	_socket = 0;
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

		/*int flags = fcntl(sock, F_GETFL, 0);
		if ( -1 == flags)
		{
			closesocket(sock);
			continue;
		}
		if( -1 == fcntl(sock, F_SETFL, flags | O_NONBLOCK) )
		{
			closesocket(sock);
			continue;
		}*/

		ServantThread* pthread = _vecThread[_roundCount];
		pthread->addSocket(sock);
		pthread->send();
		printf("send sock = %d\n", sock);
		_roundCount = (_roundCount + 1) % _Config.threadCount;
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
	_loop->close();
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
	Async::init(*_loop);
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"thread start run! ThreadId = %d"),id());
	_loop->run(Loop::Default);
	_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer,"thread quit! ThreadId = %d"),id());
	return 0;
}

void ServantThread::OnAsync()
{
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