#include <ZQ_common_conf.h>
#include "HttpServer.h"
#include "TimeUtil.h"
#include <fcntl.h>
#ifdef ZQ_OS_LINUX
#include <signal.h>
#endif

namespace ZQ {
namespace eloop {

//---------------------------------------
//class HttpMonitorTimer
//----------------------------------------
void HttpMonitorTimer::OnTimer()
{
	HttpPassiveConn* conn = (HttpPassiveConn*)data;
	if (conn != NULL)
	{
		conn->stop();
	}
}
// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(HttpServer& server,ZQ::common::Log& logger)
		:_server(server),
		HttpConnection(false,logger),
//		_bError(false),
		_Handler(0),
		_keepAlive_Timeout(_server.keepAliveTimeout()),
		_startTime(0),
		_keepAlive(false),
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
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(httpServer,"new connection from [%s]"),_Hint.c_str());
	_server.addConn(this);
	if (_keepAlive_Timeout > 0)
	{
		_timer.init(this->get_loop());
		_timer.data = this;
	}
	return true;
}

void HttpPassiveConn::stop()
{
	if (_keepAlive_Timeout>0 && _timer.is_active()!=0)
	{
		_timer.stop();
		_timer.close();
	}
	shutdown();
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
//	_bError = true;
	std::string response = msg->toRaw();
	write(response.c_str(),response.length());
	_keepAlive = false;
	stop();
}

void HttpPassiveConn::onError(int error,const char* errorDescription) {
	
	if (error != elpe__EOF)
	{
		char locip[17] = { 0 };
		int  locport = 0;
		getlocaleIpPort(locip,locport);

		char peerip[17] = { 0 };
		int  peerport = 0;
		getpeerIpPort(peerip,peerport);
		
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn, "onError [%p] [%s:%d => %s:%d], errorCode[%d],Description[%s]"), 
			this, locip, locport, peerip, peerport,error,errorDescription);
	}
	if (error > 0)		//Parse error message
	{
		errorResponse(400);
		return;
	}
	
	if(_Handler)
		_Handler->onError(error,errorDescription);

	stop();
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

void HttpPassiveConn::onRespComplete()
{
	HttpConnection::onRespComplete();
	if (_keepAlive && _keepAlive_Timeout > 0)
	{
		if (_startTime <= 0)
			_startTime = ZQ::common::now();
		int took = ZQ::common::now() - _startTime;
		_timer.start(_keepAlive_Timeout+took,0);
	}
	else
	{
		_listpipe.clear();
		stop();
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
		
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 ) {
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHeadersEnd, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
		errorResponse(505);
		return false;
	}
//	printf("create handle url=%s\n",msg->url().c_str());
	_Handler = _server.createHandler( msg->url(), *this);

	if(!_Handler) {
		//should make a 404 response
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd failed to find a suitable handle to process url: %s"),
			msg->url().c_str() );
		errorResponse(404);
		return false;
	} else {
		_keepAlive = msg->keepAlive();
		if(!_Handler->onHeadersEnd(msg) ) {
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
		_Logger(logger),
		_isStart(false)
{
#ifdef ZQ_OS_LINUX
	//Ignore SIGPIPE signal
	signal(SIGPIPE, SIG_IGN);
#endif
}

HttpServer::~HttpServer()
{

}

bool HttpServer::startAt()
{
	if (_isStart)
		return true;
	
	if (_Config.mode == MULTIPE_LOOP_MODE)
	{
		_engine = new MultipleLoopHttpEngine(_Config.host,_Config.port,_Logger,*this);
	}
	else
		_engine = new SingleLoopHttpEngine(_Config.host,_Config.port,_Logger,*this);

	_isStart = true;
	return _engine->startAt();
}

void HttpServer::stop()
{
	if (_PassiveConn.empty())
	{
		_engine->stop();
	}
	else
	{
		std::set<HttpPassiveConn*>::iterator itconn;
		for(itconn = _PassiveConn.begin();itconn != _PassiveConn.end();itconn++)
		{
			(*itconn)->shutdown();
		}
	}
	_sysWakeUp.wait(-1);
	if (_engine != NULL)
	{
		delete _engine;
		_engine = NULL;
	}
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServer, "quit HttpServer!"));
}
/*
int HttpServer::run()
{
	int64 timeLimit = 0,waitTime = _Config.keepalive_timeout;
	while(!_Quit)
	{
		SYS::SingleObject::STATE state = _sysWakeUp.wait(waitTime);
		if (_Quit)
			break;
		waitTime = 5000;
		if(SYS::SingleObject::SIGNALED == state || SYS::SingleObject::TIMEDOUT == state)
		{
			ZQ::common::MutexGuard gd(_connCountLock);
			std::set<HttpPassiveConn*>::iterator itconn;
			for(itconn = _PassiveConn.begin();itconn != _PassiveConn.end();itconn++)
			{
				timeLimit = ZQ::common::now() - (*itconn)->lastRespTime();
				if (timeLimit >= _Config.keepalive_timeout)
				{
					(*itconn)->shutdown();
				}
				if (waitTime > timeLimit)
					waitTime = timeLimit;
			}
			if (_PassiveConn.empty())
				waitTime = _Config.keepalive_timeout;
			
		}
	}
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServer, "quit Monitor thread!"));
	return 0;
}*/


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

HttpHandler::Ptr HttpServer::createHandler(const std::string& uri, HttpPassiveConn& conn, const std::string& virtualSite)
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
	ZQ::common::MutexGuard gd(_connCountLock);
	_PassiveConn.insert(servant);
}

void HttpServer::delConn( HttpPassiveConn* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_PassiveConn.erase(servant);
	if (_PassiveConn.empty())
		_engine->stop();
}


HttpMessage::Ptr HttpServer::makeSimpleResponse( int code ) const {
	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	msg->header("Server", _Config.serverName );
	msg->header("Date",HttpMessage::httpdate());
	return msg;
}

int64 HttpServer::keepAliveTimeout() const
{
	return _Config.keepalive_timeout;
}

void HttpServer::single()
{
	_sysWakeUp.signal();
}

// ---------------------------------------
// class AsyncQuit
// ---------------------------------------
void AsyncQuit::OnAsync()
{
	SingleLoopHttpEngine* eng = (SingleLoopHttpEngine*)data;
	eng->close();
}

// ---------------------------------------
// class SingleLoopHttpEngine
// ---------------------------------------
// Single event loop
SingleLoopHttpEngine::SingleLoopHttpEngine(const std::string& ip,int port,ZQ::common::Log& logger,HttpServer& server)
:IHttpEngine(ip,port,logger,server)
{
}
SingleLoopHttpEngine::~SingleLoopHttpEngine()
{
	
}
bool SingleLoopHttpEngine::startAt()
{
	return ZQ::common::NativeThread::start();
}

int SingleLoopHttpEngine::run(void)
{
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpEngine,"SingleLoopHttpEngine start"));
	init(_loop);
	_async.data = this;
	_async.init(_loop);
	if (bind4(_ip.c_str(),_port) < 0)
		return false;

	if (listen() < 0)
		return false;

	int r=_loop.run(ZQ::eloop::Loop::Default);
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(SingleLoopHttpEngine,"SingleLoopHttpEngine quit!"));
	return r;
}

void SingleLoopHttpEngine::stop()
{
	_async.send();
}

void SingleLoopHttpEngine::OnClose()
{
	_loop.close();
	_server.single();
}

void SingleLoopHttpEngine::doAccept(ElpeError status)
{
	if (status != elpeSuccess)
	{
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(SingleLoopHttpEngine,"doAccept() error code[%d] desc[%s]"),status,errDesc(status));
		return;
	}

	HttpPassiveConn* client = new HttpPassiveConn(_server,_Logger);
	client->init(get_loop());

	if (accept((Stream*)client) == 0) {
		client->start();
	}
	else {
		client->shutdown();
	}
}

// ---------------------------------------
// class MultipleLoopHttpEngine
// ---------------------------------------
//Multiple event loop
MultipleLoopHttpEngine::MultipleLoopHttpEngine(const std::string& ip,int port,ZQ::common::Log& logger,HttpServer& server)
			:IHttpEngine(ip,port,logger,server),
			_bRunning(false),
			_roundCount(0),
			_quitCount(0),
			_socket(0)
{
/*
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
*/

	CpuInfo cpu;
	_threadCount = cpu.getCpuCount();

	for (int i = 0;i < _threadCount;i++)
	{
		ServantThread *pthread = new ServantThread(_server,*this,_Logger);

		//printf("cpuId = %d,cpuCount = %d,mask = %d\n",i,_threadCount,1<<i);
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopHttpEngine,"cpuId:%d,cpuCount:%d,mask:%d"),i,_threadCount,1<<i);
		pthread->setCpuAffinity(i);

		pthread->start();
		_vecThread.push_back(pthread);
	}
}

MultipleLoopHttpEngine::~MultipleLoopHttpEngine()
{
	std::vector<ServantThread*>::iterator it = _vecThread.begin();
	while(it != _vecThread.end())
	{
		delete *it;
		*it = NULL;
		it = _vecThread.erase(it);
	}
	_vecThread.clear();
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpEngine,"MultipleLoopHttpEngine quit!"));
}

bool MultipleLoopHttpEngine::startAt()
{

#ifdef ZQ_OS_MSWIN
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
	{
		return 0;
	}
#endif


	_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	int reuse_value = 1;
	if( setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) != 0 )
		return false;

	if( _socket < 0 )
		return false;

	struct sockaddr_in serv;
	serv.sin_family=AF_INET;
	serv.sin_port=htons(_port);
	serv.sin_addr.s_addr=inet_addr(_ip.c_str());
	if(bind(_socket,(struct sockaddr*)&serv,sizeof(serv)) == -1)
	{
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopHttpEngine,"socket bind error hint[%s:%d]"),_ip.c_str(),_port);
		return false;
	}
	if (0 != listen(_socket,10000))
	{
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopHttpEngine,"socket listen error hint[%s:%d]"),_ip.c_str(),_port);
		return false;
	}
	_bRunning = true;
	return ZQ::common::NativeThread::start();
}

void MultipleLoopHttpEngine::stop()
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
	std::vector<ServantThread*>::iterator iter;
	for (iter=_vecThread.begin();iter!=_vecThread.end();iter++)  
	{  
		(*iter)->quit();
		(*iter)->send();
	}
}

void MultipleLoopHttpEngine::QuitNotify(ServantThread* sev)
{
	ZQ::common::MutexGuard gd(_Lock);
	_quitCount++;
	if (_quitCount == _threadCount)
		_server.single();
}

int MultipleLoopHttpEngine::run()
{
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpEngine,"MultipleLoopHttpEngine start"));
	struct sockaddr_storage addr;
	while( _bRunning ) {
		socklen_t size= (socklen_t)sizeof( addr );
		int sock = accept( _socket, (struct sockaddr*)&addr, &size);
		if( sock < 0 )
			break;
/*
		int flags = fcntl(sock, F_GETFL, 0);
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
		if (!_bRunning)
			break;
		
		ServantThread* pthread = _vecThread[_roundCount];
		pthread->addSocket(sock);
		pthread->send();
		_roundCount = (_roundCount + 1) % _threadCount;
	}
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(MultipleLoopHttpEngine,"MultipleLoopHttpEngine quit"));
	return 0;
}

// ------------------------------------------------
// class ServantThread
// ------------------------------------------------
ServantThread::ServantThread(HttpServer& server,MultipleLoopHttpEngine& engine,ZQ::common::Log& logger)
		:_Logger(logger),
		_engine(engine),
		_quit(false),
		_server(server)
{
	_loop = new Loop(false);
}

ServantThread::~ServantThread()
{
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread destructor!"));
}

void ServantThread::quit()
{
	_quit = true;
}

void ServantThread::OnClose()
{
	_loop->close();
	if (_loop != NULL)
	{
		delete _loop;
	}
	_engine.QuitNotify(this);
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
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread start run!"));
	int r = _loop->run(Loop::Default);
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread quit!"));
	return r;
}

void ServantThread::OnAsync()
{
	ZQ::common::MutexGuard gd(_LockSocket);
	if (_quit)
	{
		close();
		return;
	}
	while( !_ListSocket.empty())
	{
		HttpPassiveConn* client = new HttpPassiveConn(_server,_Logger);
		int r = client->init(get_loop());
		if (r != 0)
		{
			//printf("tcp init error:%s,name :%s\n", uv_strerror(r), uv_err_name(r));
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServantThread,"OnAsync tcp init errorcode[%d] describe[%s]"),r,Handle::errDesc(r));
		}

		int sock = _ListSocket.back();
		_ListSocket.pop_back();
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ServantThread,"OnAsync recv sock = %d"),sock);
		r = client->connected_open(sock);
		if (r != 0)
		{
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServantThread,"OnAsync open socke errorcode[%d] describe[%s]"),r,Handle::errDesc(r));
		}
		client->start();
	}
}

// ---------------------------------------
// class HttpStatistics
// ---------------------------------------
static HttpStatistics _gHttpStatistics;
HttpStatistics& getHttpStatistics()
{
	return _gHttpStatistics;
}

HttpStatistics::HttpStatistics()
{
	reset();
}
void HttpStatistics::reset()
{
	ZQ::common::MutexGuard gd(_locker);
	memset(&_counters, 0x00, sizeof(_counters));
	_mesureSince = ZQ::common::now();
}

void HttpStatistics::addCounter(HttpMessage::HttpMethod mtd, int32 errCode, int64 latencyHeader, int64 latencyBody )
{
	Method method = httpMethodToMethod(mtd);

	ZQ::common::MutexGuard gd(_locker);
	_counters[method].totalCount ++;
	_counters[method].respCount[errCodeToRespCode(errCode)] ++;

	if (_counters[method].maxLatencyInMs_Body < latencyBody)
		_counters[method].maxLatencyInMs_Body = latencyBody;
	if (_counters[method].maxLatencyInMs_Header < latencyHeader)
		_counters[method].maxLatencyInMs_Header = latencyHeader;

	_counters[method].subtotalLatencyInMs_Body += latencyBody;
	_counters[method].subtotalLatencyInMs_Header += latencyHeader;

	_counters[method].avgLatencyInMs_Body = _counters[method].totalCount ? (_counters[method].subtotalLatencyInMs_Body /_counters[method].totalCount) :0;
	_counters[method].avgLatencyInMs_Header = _counters[method].totalCount ? (_counters[method].subtotalLatencyInMs_Header /_counters[method].totalCount) :0;
}

const char* HttpStatistics::nameOfMethod(int mtd)
{
	switch(mtd)
	{
	case METHOD_GET: return "GET";
	case METHOD_POST: return "POST";
	case METHOD_PUT: return "PUT";
	case METHOD_DELETE: return "DELETE";
	}

	return "UNKOWN";
}

HttpStatistics::RespCode HttpStatistics::errCodeToRespCode( int32 errCode )
{
	if( errCode/100 == 2)
	{
		return RESP_2XX;
	}
	switch( errCode )
	{
	case 400:	return RESP_400;
	case 403:	return RESP_403;
	case 404:	return RESP_404;
	case 405:	return RESP_405;
	case 500:	return RESP_500;
	case 501:	return RESP_501;
	case 503:	return RESP_503;
	default:	return RESP_OTHER;
	}
}

HttpStatistics::Method HttpStatistics::httpMethodToMethod(HttpMessage::HttpMethod mtd)
{
	switch(mtd)
	{
	case HttpMessage::GET: return METHOD_GET;
	case HttpMessage::POST: return METHOD_POST;
	case HttpMessage::PUT: return METHOD_PUT;
	case HttpMessage::HTTPDELETE: return METHOD_DELETE;
	}
	return METHOD_UNKNOWN;
}

} }//namespace ZQ::eloop
