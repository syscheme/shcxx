#include <ZQ_common_conf.h>
#include <boost/regex.hpp>
#include "HttpServer.h"
#include "TimeUtil.h"
#include <fcntl.h>
#ifdef ZQ_OS_LINUX
#include <signal.h>
#endif

#define	Header_RequestId			"X-Request-ID"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class HttpPassiveConn
// ---------------------------------------
// present an accepted incomming connection
class HttpPassiveConn : public HttpConnection
{
public:
	HttpPassiveConn(HttpServer& server);
	virtual ~HttpPassiveConn() {}

	bool	keepAlive() const { return _keepAlive && (_idleTimeout >0); }

protected: // implementation of HttpConnection
	virtual void OnTimer();

	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnHeadersReceived(const HttpMessage::Ptr& req); // maps to RTSP::OnRequest-1
	virtual void OnMessageReceived(const HttpMessage::Ptr& req); // maps to RTSP::OnRequest-3

	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnBodyPayloadReceived(const uint8* data, size_t size); // maps to RTSP::OnRequest-2.2
	virtual void OnBodyPayloadSubmitted(size_t bytesPushed, uint64 offsetBodyPayload);
	virtual void OnMessageSubmitted(HttpMessage::Ptr msg);

	virtual void OnMessagingError( int error, const char* errorDescription );
	//virtual void	OnClose(); 

private:
	void initHint();
	HttpMessage::StatusCodeEx setResponseError(HttpMessage::Ptr& resp, HttpMessage::StatusCodeEx code, const char* warning =NULL);

protected:
	HttpHandler::Ptr			_handler;
	HttpServer&					_server;

	bool						_keepAlive;
	int					    	_idleTimeout;
};

#define CONNFMT(FUNC, FMT) CLOGFMT(HttpPassiveConn, #FUNC "() " FMT)
#define SEND_GUARD()  ZQ::common::MutexGuard sg(_lkSend)

HttpPassiveConn::HttpPassiveConn(HttpServer& server)
	: HttpConnection(server._logger, NULL, &server), _server(server),
	_handler(NULL), _idleTimeout(server.keepAliveTimeout()), _keepAlive(false)
{
}

HttpMessage::StatusCodeEx HttpPassiveConn::setResponseError(HttpMessage::Ptr& resp, HttpMessage::StatusCodeEx code, const char* warning)
{
	if (!resp)
	{
		_logger(ZQ::common::Log::L_WARNING, CONNFMT(HttpPassiveConn, "setResponseError(%d) NULL resp, WARN:%s"), code, warning);
		return HttpMessage::errBackendProcess;
	}

	resp->_statusCode = code;
	if (warning)
	{
		MAPSET(HttpMessage::Headers, resp->_headers, "Warning", warning);
		// _logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpPassiveConn, "setResponseError(%d): %s"), code, warning);
	}
	else resp->_headers.erase("Warning");

	return (HttpMessage::StatusCodeEx) resp->statusCode();
}

void HttpPassiveConn::OnMessagingError(int error, const char* errorDescription)
{
	_logger(ZQ::common::Log::L_ERROR, CONNFMT(OnReceiveError, "error(%d): %s"), error, errorDescription);

	if(_handler)
		_handler->OnMessagingError(error, errorDescription);

	disconnect(true);
}

void HttpPassiveConn::OnBodyPayloadSubmitted(size_t bytesPushed, uint64 offsetBodyPayload)
{
	HttpConnection::OnBodyPayloadSubmitted(bytesPushed, offsetBodyPayload);

	if(_handler)
		_handler->OnResponsePayloadSubmitted(bytesPushed, offsetBodyPayload);
}

void HttpPassiveConn::OnMessageSubmitted(HttpMessage::Ptr msg)
{
	HttpConnection::OnMessageSubmitted(msg);

	//if (_keepAlive && _idleTimeout >0 && (_stampLastRecv + _idleTimeout) > ZQ::common::now())
	//{
	//	_logger(ZQ::common::Log::L_DEBUG, CONNFMT(OnMessageSubmitted, "keep-alive timeout %dmsec"), _idleTimeout);
	//	return;
	//}

	//_logger(ZQ::common::Log::L_DEBUG, CONNFMT(OnMessageSubmitted, "disconnecting"));
	//disconnect(true);
}

void HttpPassiveConn::OnTimer()
{
	if (!_keepAlive)
	{
		if (!_msgOutgoing &&  _stampLastSent>0 && (ZQ::common::now() - _stampLastSent) > _server._config.watchDogInterval)
			disconnect(false); // for _keepAlive = false
	}
	else 
	{
		// keepAlive= true
		int idleElapsed = (int) (ZQ::common::now() - MAX(_stampLastSent,_stampLastRecv));
		if (_idleTimeout>0 && idleElapsed > _idleTimeout)
		{
			_logger(ZQ::common::Log::L_DEBUG, CONNFMT(OnTimer, "disconnecting, idle time %dmsec"), idleElapsed);
			disconnect(false);
		}
	}
}

HttpMessage::StatusCodeEx HttpPassiveConn::OnBodyPayloadReceived(const uint8* data, size_t size)
{
	if (_handler && _handler->_req) // && _handler->_req->chunked())
		return _handler->OnRequestPayload((const char*)data, size);

	return HttpConnection::OnBodyPayloadReceived(data, size);
}

HttpMessage::StatusCodeEx HttpPassiveConn::OnHeadersReceived(const HttpMessage::Ptr& msg)
{
	HttpServer* pSev =NULL;
	HttpHandler::Response::Ptr resp = new HttpHandler::Response(_server, msg);
	if (!resp)
	{
		//should make a 404 response
		_logger(ZQ::common::Log::L_ERROR, CONNFMT(HttpPassiveConn, "OnHeadersReceived failed to create respose"));
		return HttpMessage::scInternalServerError;
	}

	resp->_statusCode = HttpMessage::scInternalServerError;

	pSev = dynamic_cast<HttpServer*>(_tcpServer);
	if (pSev == NULL)
	{
		_logger(ZQ::common::Log::L_ERROR, CONNFMT(HttpPassiveConn, "OnHeadersReceived not found HttpServer"));
		return resp->post(HttpMessage::scInternalServerError);
	}
	resp->header("Server", pSev->_config.serverName);

	uint vMajor=0, vMinor=0;
	msg->getHTTPVersion(vMajor, vMinor);
	if (vMajor != 1 && (vMinor > 1) )
	{
		_logger(ZQ::common::Log::L_WARNING, CONNFMT(HttpPassiveConn,"OnHeadersReceived, unsupport http version[%u/%u], reject"), vMajor, vMinor);
		return resp->post(HttpMessage::scHTTPVersionNotSupported);
	}

	_handler = _server.createHandler(msg, *this);
	if( NULL == _handler)
	{
		//should make a 404 response
		_logger(ZQ::common::Log::L_WARNING, CONNFMT(HttpPassiveConn, "OnHeadersReceived failed to find a suitable handle to process uri[%s]"), msg->uri().c_str());
		return resp->post(HttpMessage::scNotFound, "no associated handler");
	}

	_keepAlive = msg->keepAlive();
	resp->_statusCode = _handler->OnRequestHeaders(resp);
	if (HttpMessage::errAsyncInProgress == resp->_statusCode)
		return HttpMessage::errAsyncInProgress;

	_logger(HTTP_SUCC(resp->_statusCode) ? ZQ::common::Log::L_INFO :ZQ::common::Log::L_ERROR, CONNFMT(HttpPassiveConn, "OnHeadersReceived, finished handling, posting response(%d) uri[%s]"), resp->_statusCode, msg->uri().c_str());
	_handler = NULL;

	return resp->post(resp->_statusCode);
}

void HttpPassiveConn::OnMessageReceived(const HttpMessage::Ptr& req)
{
	if(!_handler)
		return;

	HttpMessage::StatusCodeEx ret = _handler->OnRequestCompleted();
	if (HttpMessage::errAsyncInProgress == ret)
		return;
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
// class HttpHandler
// ---------------------------------------
HttpHandler::HttpHandler(const HttpMessage::Ptr& req, IBaseApplication& app, HttpServer& server, const HttpHandler::Properties& dirProps)
                 : _req(req), _app(app), _dirProps(dirProps)
{}

HttpHandler::~HttpHandler()
{}
// ---------------------------------------
// class HttpResponse
// ---------------------------------------
		//	: _server(server), HttpMessage(HttpMessage::MSG_RESPONSE, req->getConnId()), _req(req)
		//{
		//	_statusCode = scInternalServerError;
		//	_server.addAwait(this);
		//}
HttpHandler::Response::Response(HttpServer& server, const HttpMessage::Ptr& req)
: _server(server), HttpMessage(HttpMessage::MSG_RESPONSE, req->getConnId()),
  _req(req), _stampPosted(0), _bodyBytesPushed(0)
{
	if (_req)
	{
		_txnId = req->header(Header_RequestId);
		if (_txnId.empty())
		{
			char tmp[100];
			snprintf(tmp, sizeof(tmp)-2, "%s@%s", HttpMessage::method2str(_req->method()), _req->getConnId().c_str());
			_txnId = tmp;
			header(Header_RequestId, _txnId);
		}
	}
}

TCPConnection* HttpHandler::Response::getConn() 
{	
	return _server.findConn(getConnId()); 
}

HttpMessage::StatusCodeEx HttpHandler::Response::post(int statusCode, const std::string& body, const char* errMsg) 
{
	HttpMessage::StatusCodeEx ret = HttpMessage::scInternalServerError;
	{
		// to avoid double post response
		ZQ::common::MutexGuard g(_locker);
		if (_stampPosted)
			return ret;
		_stampPosted = ZQ::common::now();
	}

	HttpPassiveConn* conn = dynamic_cast<HttpPassiveConn*>(getConn());
	if (conn == NULL)
	{
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpResponse, "post() drop %s per conn[%s] already closed"), _txnId.c_str(), getConnId().c_str());
		return ret;
	}

	MAPSET(HttpMessage::Headers, _headers, "Date",   HttpMessage::httpdate());

	if (HTTP_STATUSCODE(statusCode))
		_statusCode = statusCode;
	else _statusCode = scInternalServerError;

	size_t len = body.length();
        if (HttpMessage::errSendConflict != ret && len >0)
       	{
        	bool end = false;
                if (!chunked() && (_bodyBytesPushed + len) >= contentLength())
                	end = true;

                conn->pushOutgoingPayload((const void*) body.c_str(), len, end);
                _bodyBytesPushed += len;
        }

	
	ret = conn->sendMessage(this);

	return ret;
}

int64 HttpHandler::Response::declareContentLength(int64 contentLen, const char* contentType, bool chunked)
{ 
	_declaredBodyLength = contentLen; 
	if (contentType)
		header("Content-Type", contentType); 

	if (chunked)
	{
		_declaredBodyLength = 0; 
		_flags |= F_CHUNKED;
		header("Transfer-Encoding", "chunked"); 
	}

	return _declaredBodyLength;
}

HttpMessage::StatusCodeEx HttpHandler::Response::pushBody(const uint8* data, size_t len, bool chunkedEnd)
{
	bool end = chunkedEnd;
	if (NULL == data || len <=0)
		return HttpMessage::scBadRequest;

	HttpPassiveConn* conn = dynamic_cast<HttpPassiveConn*>(getConn());
	if (conn == NULL)
	{
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpResponse, "pushBody() %s failed per conn[%s] not found"), _txnId.c_str(), getConnId().c_str());
		return HttpMessage::scInternalServerError;
	}

	if (!chunked() && (_bodyBytesPushed + len) >= contentLength())
		end = true;

	conn->pushOutgoingPayload(data, len, end);
	return end ? HttpMessage::scOK : HttpMessage::scPartialContent;
}

// ---------------------------------------
// class HttpServer
// ---------------------------------------
HttpServer::HttpServer( const TCPServer::ServerConfig& conf, ZQ::common::Log& logger)
		:TCPServer(conf,logger)
{
}

HttpServer::~HttpServer()
{
}

TCPConnection* HttpServer::createPassiveConn()
{
	return new HttpPassiveConn(*this);
}

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

bool HttpServer::unmount(const std::string& uriEx, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	// address the virtual site
	bool ret = false;
	VSites::iterator itSite = _vsites.find(vsite);
	if (_vsites.end() == itSite)
		return ret;

	for(MountDirs::iterator it = itSite->second.begin(); it < itSite->second.end();)
	{
		if (it->uriEx == uriEx)
		{
			it = itSite->second.erase(it);
			ret = true;
		}
		else  it++;
	}

	return ret;
}

HttpHandler::Ptr HttpServer::createHandler(const HttpMessage::Ptr& req, HttpConnection& conn, const std::string& virtualSite)
{
	HttpHandler::AppPtr app = NULL;

	// cut off the paramesters
	std::string uriWithnoParams = req->uri();
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
				handler = it->app->create(*this, req, it->props);
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
