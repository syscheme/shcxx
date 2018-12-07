#include <ZQ_common_conf.h>
#include <boost/regex.hpp>
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
// void HttpMonitorTimer::OnTimer()
// {
// 	HttpPassiveConn* conn = (HttpPassiveConn*)data;
// 	if (conn != NULL)
// 		conn->stop();
// }


// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
HttpPassiveConn::HttpPassiveConn(HttpServer& server)
	: HttpConnection(server._logger, NULL, &server), _server(server),
	_handler(NULL), _keepAlive_Timeout(server.keepAliveTimeout()),
	_startTime(0), _keepAlive(false)
{
}

HttpPassiveConn::~HttpPassiveConn()
{
	//_handler = NULL;
}

void HttpPassiveConn::errorResponse( int code ) 
{
	_logger(ZQ::common::Log::L_DEBUG,CLOGFMT(HttpPassiveConn,"errorResponse, code %d"), code);

	HttpMessage::Ptr msg = new HttpMessage(HttpMessage::MSG_RESPONSE);
	msg->code(code);
	msg->status(HttpMessage::code2status(code));
	if (_tcpServer)
		msg->header("Server", _tcpServer->_config.serverName );
	msg->header("Date", HttpMessage::httpdate());

	std::string response = msg->toRaw();
	write(response.c_str(), response.length());

	_keepAlive = false;
	disconnect(true);
}

void HttpPassiveConn::OnConnectionError(int error, const char* errorDescription)
{
	if (elpuEOF != error)
	{
		char locip[17] = { 0 };
		int  locport = 0;
		getlocaleIpPort(locip,locport);

		char peerip[17] = { 0 };
		int  peerport = 0;
		getpeerIpPort(peerip,peerport);
		
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn, "onError [%p] [%s:%d => %s:%d], errorCode[%d],Description[%s]"), 
			this, locip, locport, peerip, peerport,error,errorDescription);
	}

	if (error > 0)		//Parse error message
	{
		errorResponse(400);
		return;
	}
	
	if(_handler)
		_handler->OnConnectionError(error, errorDescription);

	disconnect(true);
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
//	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn, "onHttpDataSent [%p] [%s:%d==>%s:%d]."), this, locip, locport, peerip, peerport);

	if(NULL == _handler)
		return;

	_handler->onHttpDataSent(size);
}

void HttpPassiveConn::onRespComplete(bool isShutdown)
{
	HttpConnection::onRespComplete();
	if (!_keepAlive || _keepAlive_Timeout <= 0)
	{
		_listpipe.clear();
		disconnect(isShutdown);
		return;
	}

	if (_startTime <= 0)
		_startTime = ZQ::common::now();
}

void HttpPassiveConn::OnTimer()
{
	if (_keepAlive_Timeout>0 && _startTime>0 &&(ZQ::common::now() - _startTime > _keepAlive_Timeout))
		disconnect(false);
}

void HttpPassiveConn::onHttpDataReceived( size_t size )
{
	// NOTE something here
	if(_handler)
		_handler->onHttpDataReceived(size);

	//start();//this may fail because a receiving call has been fired		
}

bool HttpPassiveConn::onHeadersEnd( const HttpMessage::Ptr msg)
{
	if( msg->versionMajor() != 1 && msg->versionMinor() != 1 )
	{
		_logger(ZQ::common::Log::L_WARNING, CLOGFMT( HttpPassiveConn,"onHeadersEnd, unsupport http version[%u/%u], reject"),
			msg->versionMajor(), msg->versionMinor());
	
		errorResponse(505);
		return false;
	}

	 HttpServer* pSev = NULL;
	 pSev = dynamic_cast<HttpServer*>(_tcpServer);
	 if (pSev == NULL)
	 {
		 //should make a 404 response
		 _logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpPassiveConn,"onHeadersEnd not found HttpServer."));
		 errorResponse(503);
		 return false;
	 }

	_handler = _server.createHandler( msg->url(), *this);

	if(!_handler)
	{
		//should make a 404 response
		_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd failed to find a suitable handle to process url:[%s], msg[%s]"), msg->url().c_str(), msg->toRaw().c_str() );
		errorResponse(404);
		return false;
	}

	_keepAlive = msg->keepAlive();
	if(!_handler->onHeadersEnd(msg) )
	{
		_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"onHeadersEnd, user code return false in onHeadersEnd, may user code want to abort the procedure, url:%s"), msg->url().c_str());
		_handler = NULL;
		return false;
	}

	return true;
}

bool HttpPassiveConn::onBodyData( const char* data, size_t size)
{
	if(!_handler)
	{
		_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn,"http body received, but no handler is ready"));
		errorResponse(500);
		return false;
	}

	if(!_handler->onBodyData(data, size) )
	{
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpPassiveConn,"handler refuse to continue after onBodyData"));
		errorResponse(500);
		_handler = NULL;
		return false;
	}

	return true;
}

void HttpPassiveConn::onMessageCompleted()
{
	if(!_handler)
		return;

	_handler->onMessageCompleted();
}
/*
void HttpPassiveConn::OnClose()
{
	HttpServer* pSev = dynamic_cast<HttpServer*>(_tcpServer);
	if (pSev != NULL)
		_server.delConn(this);
	_handler = NULL;
	delete this;
}
*/

// ---------------------------------------
// class HttpServer
// ---------------------------------------
TCPConnection* HttpServer::createPassiveConn()
{
	return new HttpPassiveConn(*this);
}

/*
int HttpServer::run()
{
	int64 timeLimit = 0,waitTime = _config.keepalive_timeout;
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
			for(itconn = _connMap.begin();itconn != _connMap.end();itconn++)
			{
				timeLimit = ZQ::common::now() - (*itconn)->lastRespTime();
				if (timeLimit >= _config.keepalive_timeout)
				{
					(*itconn)->shutdown();
				}
				if (waitTime > timeLimit)
					waitTime = timeLimit;
			}
			if (_connMap.empty())
				waitTime = _config.keepalive_timeout;
			
		}
	}
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpServer, "quit Monitor thread"));
	return 0;
}*/

bool HttpServer::mount(const std::string& uriEx, HttpHandler::AppPtr app, const HttpHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;

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
		boost::regex re;
		try {
			re.assign(it->uriEx);
		}
		catch( const boost::regex_error& )
		{
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), virtualSite.c_str(), it->uriEx.c_str());
			continue;
		}

		if (boost::regex_match(uriWithnoParams, re))
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
	_measureSince = ZQ::common::now();
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
