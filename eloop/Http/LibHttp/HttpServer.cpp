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
		conn->stop();
}

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(ZQ::common::Log& logger,TCPServer* tcpServer)
		:HttpConnection(false,logger, NULL,tcpServer),
		_Handler(NULL),
		_keepAlive_Timeout(tcpServer->keepAliveTimeout()),
		_startTime(0),
		_keepAlive(false)
{
}

HttpPassiveConn::~HttpPassiveConn()
{
	_Handler = NULL;
}

bool HttpPassiveConn::onStart()
{
	if (_keepAlive_Timeout > 0)
	{
		_watchDog.init(this->get_loop());
		_watchDog.data = this;
	}
	return true;
}

bool HttpPassiveConn::onStop()
{
	if (_keepAlive_Timeout>0 && _watchDog.is_active()!=0)
	{
		_watchDog.stop();
		_watchDog.close();
	}
}

void HttpPassiveConn::errorResponse( int code ) 
{
	_Logger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpPassiveConn,"errorResponse, code %d"), code);

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	if (_tcpServer)
		msg->header("Server", _tcpServer->_Config.serverName );
	msg->header("Date",HttpMessage::httpdate());

	std::string response = msg->toRaw();
	write(response.c_str(), response.length());

	_keepAlive = false;
	stop();
}

void HttpPassiveConn::onError(int error,const char* errorDescription)
{
	if (elpuEOF != error)
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

void HttpPassiveConn::onHttpDataSent(size_t size) 
{

//  char locip[17] = { 0 };
//	int  locport = 0;
//	getlocaleIpPort(locip,locport);
//
//	char peerip[17] = { 0 };
//	int  peerport = 0;
//	getpeerIpPort(peerip,peerport);
//	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);

	if(NULL == _Handler)
		return;

	_Handler->onHttpDataSent(size);
}

void HttpPassiveConn::onRespComplete()
{
	HttpConnection::onRespComplete();
	if (!_keepAlive || _keepAlive_Timeout <= 0)
	{
		_listpipe.clear();
		stop();
		return;
	}

	if (_startTime <= 0)
		_startTime = ZQ::common::now();

	int took = (int) (ZQ::common::now() - _startTime);
	_watchDog.start(_keepAlive_Timeout+took,0);
}


void HttpPassiveConn::onHttpDataReceived( size_t size )
{
	// NOTE something here
	if(_Handler)
		_Handler->onHttpDataReceived(size);

	//start();//this may fail because a receiving call has been fired		
}

bool HttpPassiveConn::onHeadersEnd( const HttpMessage::Ptr msg)
{
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHeadersEnd, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
	
		errorResponse(505);
		return false;
	}

	 HttpServer* pSev = NULL;
	 pSev = dynamic_cast<HttpServer*>(_tcpServer);
	 if (pSev == NULL)
	 {
		 //should make a 404 response
		 _Logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn,"onHeadersEnd not found HttpServer."));
		 errorResponse(503);
		 return false;
	 }

	_Handler = pSev->createHandler( msg->url(), *this);

	if(!_Handler)
	{
		//should make a 404 response
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd failed to find a suitable handle to process url: %s"), msg->url().c_str() );
		errorResponse(404);
		return false;
	}

	_keepAlive = msg->keepAlive();
	if(!_Handler->onHeadersEnd(msg) )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd, user code return false in onHeadersEnd, may user code want to abort the procedure, url:%s"), msg->url().c_str());
		_Handler = NULL;
		return false;
	}

	return true;
}

bool HttpPassiveConn::onBodyData( const char* data, size_t size)
{
	if(!_Handler)
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"http body received, but no handler is ready"));
		errorResponse(500);
		return false;
	}

	if(!_Handler->onBodyData(data, size) )
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"handler refuse to continue after onBodyData"));
		errorResponse(500);
		_Handler = NULL;
		return false;
	}

	return true;
}

void HttpPassiveConn::onMessageCompleted()
{
	if(!_Handler)
		return;

	_Handler->onMessageCompleted();
}
/*
void HttpPassiveConn::OnClose()
{
	HttpServer* pSev = dynamic_cast<HttpServer*>(_tcpServer);
	if (pSev != NULL)
		pSev->delConn(this);
	_Handler = NULL;
	delete this;
}
*/
// ---------------------------------------
// class HttpServer
// ---------------------------------------
TCPConnection* HttpServer::createPassiveConn()
{
	return new HttpPassiveConn(_Logger,this);
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
	_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServer, "quit Monitor thread"));
	return 0;
}*/

bool HttpServer::mount(const std::string& uriEx, HttpHandler::AppPtr app, const HttpHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;
	try {
		dir.re.assign(uriEx);
	}
	catch( const boost::regex_error& )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), vsite.c_str(), uriEx.c_str());
		return false;
	}

	dir.uriEx = uriEx;
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
	HttpHandler::AppPtr app = NULL;

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
		if (boost::regex_match(uriWithnoParams, it->re))
		{
			if (it->app)
				handler = it->app->create(conn, it->props);
			break;
		}
	}

	return handler;
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

void HttpStatistics::addCounter(HttpMessage::HttpMethod mtd, int32 errCode, int32 latencyHeader, int32 latencyBody )
{
	ZQ::common::MutexGuard gd(_locker);
	CountersOfMethod& counter = _getCounter(mtd);
	counter.totalCount ++;
	counter.respCount[errCodeToRespCode(errCode)] ++;

	if (counter.maxLatencyInMs_Body < latencyBody)
		counter.maxLatencyInMs_Body = latencyBody;
	if (counter.maxLatencyInMs_Header < latencyHeader)
		counter.maxLatencyInMs_Header = latencyHeader;

	counter.subtotalLatencyInMs_Body += latencyBody;
	counter.subtotalLatencyInMs_Header += latencyHeader;

	counter.avgLatencyInMs_Body = counter.totalCount ? (int)(counter.subtotalLatencyInMs_Body /counter.totalCount) :0;
	counter.avgLatencyInMs_Header = counter.totalCount ? (int)(counter.subtotalLatencyInMs_Header /counter.totalCount) :0;
}

const char* HttpStatistics::nameOfMethod(int mtd)
{
	switch(mtd)
	{
	case METHOD_GET:    return "GET";
	case METHOD_POST:   return "POST";
	case METHOD_PUT:    return "PUT";
	case METHOD_DELETE: return "DELETE";
	}

	return "UNKOWN";
}

HttpStatistics::RespCode HttpStatistics::errCodeToRespCode( int32 errCode )
{
	if( errCode/100 == 2)
		return RESP_2XX;

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

HttpStatistics::CountersOfMethod& HttpStatistics::_getCounter(HttpMessage::HttpMethod mtd)
{
	switch(mtd)
	{
	case HttpMessage::GET:        return _counters[METHOD_GET];
	case HttpMessage::POST:       return _counters[METHOD_POST];
	case HttpMessage::PUT:        return _counters[METHOD_PUT];
	case HttpMessage::HTTPDELETE: return _counters[METHOD_DELETE];
	}

	return _counters[METHOD_UNKNOWN];
}

} }//namespace ZQ::eloop
