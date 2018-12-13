#include "ZQ_common_conf.h"
#include "HttpClient.h"
#include "urlstr.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class HTTPUserAgent
// ---------------------------------------
HTTPUserAgent::HTTPUserAgent(ZQ::common::Log& logger, ZQ::eloop::Loop& loop, const std::string& userAgent, const std::string& bindIP)
: _log(logger), _eloop(loop), _userAgent(userAgent), _bindIP(bindIP)
{
}

HTTPUserAgent::~HTTPUserAgent()
{
    ZQ::common::MutexGuard gGuard(_locker);
	_outgoings.clear();
	_awaits.clear();
}

HttpRequest::Ptr HTTPUserAgent::createRequest(HttpMessage::HttpMethod method, const std::string& url, const std::string& reqbody, const HttpMessage::Properties& params, const HttpMessage::Headers& headers)
{
	return new HttpRequest(*this, method, url, reqbody, params, headers);
}

void HTTPUserAgent::enqueue(HttpRequest::Ptr req)
{
	if (!req)
		return;

    ZQ::common::MutexGuard gGuard(_locker);
	_outgoings.push_back(req);
	Async::send();
}

void HTTPUserAgent::OnAsync()
{
    ZQ::common::MutexGuard gGuard(_locker);
    while (!_outgoings.empty())
    {
        HttpRequest::Ptr req = _outgoings.front();
		_outgoings.pop_front();

        if (!req)
			continue;

		MAPSET(RequestMap, _awaits, req->connId(), req);

		// kick off the request by starting connection
		req->startRequest();
    }
}

/*

HttpClient::~HttpClient()
{
}

void HttpClient::OnConnected(ElpeError status)
{
	HttpConnection::OnConnected(status);

	if (status != elpeSuccess)
		return;

	read_start();
	beginSend(_req);
	endSend();
}

bool HttpClient::beginRequest( HttpMessage::Ptr msg, const std::string& url)
{
	ZQ::common::URLStr urlstr(url.c_str());
	const char* host = urlstr.getHost();

	//change uri, host in msg
	msg->url( urlstr.getPathAndParam() );
	if(msg->url().empty() ) 
		msg->url("/");

//	printf("ip = %s,port = %d,url = %s \n",host,urlstr.getPort(),msg->url().c_str());

	msg->header("Host",host);

	_req = msg;
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "beginRequest() connect to[%s:%d]"),host,urlstr.getPort());
	connect4(host,urlstr.getPort());
	return true;
}

bool HttpClient::beginRequest( HttpMessage::Ptr msg, const std::string& ip, const unsigned int& port )
{
	_req = msg;
	connect4(ip.c_str(),port);
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "beginRequest() connect to[%s:%u]"), ip.c_str(), port);
	return true;
}

void HttpClient::OnConnectionError( int error, const char* errorDescription )
{
	_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "OnConnectionError [%p] %s error(%d) %s"), 
		this, linkstr(), error,errorDescription);

	shutdown();
}
*/

// ---------------------------------------
// class HttpRequest
// ---------------------------------------
HttpRequest::HttpRequest(HTTPUserAgent& ua, HttpMethod _method, const std::string& url, const std::string& reqbody, const Properties& params, const Properties& headers)
: _ua(ua), HttpMessage(MSG_REQUEST), HttpConnection(ua._log), _port(80), _cb(NULL)
{
	header("User-Agent", _ua._userAgent);
	HttpMessage::_method = _method;
	HttpMessage::_uri = url;

	HttpMessage::chopURI(url, _host, _port, HttpMessage::_uri, HttpMessage::_qstr);
	if (std::string::npos != _host.find_first_not_of("0123456789.")) // sounds like a domain name
		header("Host", _host);
	if (_port <=0)
		_port = 80;

	for (Properties::const_iterator it= headers.begin(); it!=headers.end(); it++)
	{
		if (it->first.empty())
			continue;
		header(it->first, it->second);
	}

	// bound with a dummy response initially
	_respMsg = new HttpMessage(MSG_RESPONSE);

	bool bParamOverwrite = false;
	std::string paramstr;
	char buf[512];
	for (Properties::const_iterator it= params.begin(); it!=params.end(); it++)
	{
		if (it->first.empty())
			continue;

		std::string varstr ="&";
		if (!ZQ::common::URLStr::encode(it->first.c_str(), buf, sizeof(buf)-2))
			continue;
		varstr += buf;

		if (!ZQ::common::URLStr::encode(it->second.c_str(), buf, sizeof(buf)-2))
			continue;
		varstr += std::string("=") + buf;


		//if (strlen(parser.getVar(it->first.c_str())) >0)
		//	bParamOverwrite = true;

		//parser.setVar(it->first.c_str(), it->second.c_str());
		paramstr += varstr;
	}

	//if (bParamOverwrite)
	//{
	//	const char* tmp = parser.getPathAndParam();
	//	if (tmp)
	//	{
	//		_uri = tmp;
	//		if ((pos = _uri.find_first_of("?&")) >1)
	//		{
	//			_qstr = _uri.substr(pos+1);
	//			_uri = _uri.substr(0, pos);
	//		}
	//	}
	//}
	//else if (paramstr.length()>1)
	//{
		if (_qstr.length() >1) _qstr += paramstr;
		else _qstr = paramstr.substr(1);
	//}

	_txnId = connId() + " [" + url+"]";

	if (_qstr.length() >1 && HTTP_POST == _method)
	{
		PayloadChunk::Ptr qchunk = new PayloadChunk((const uint8*)(_qstr+"\r\n").c_str(), _qstr.length()+2);
		// if (_payloadOutgoing.empty())
		_payloadOutgoing.push(qchunk);
		//else
		//{
		//	// insert the qstr as the first outgoing payload chunk
		//	Payload	tmp;
		//	tmp.push(qchunk);
		//	_payloadOutgoing.push(qchunk);
		//	for(; !_payloadOutgoing.empty(); _payloadOutgoing.pop())
		//		tmp.push(tmp.front());
		//	_payloadOutgoing=tmp;
		//}
	}

	if (!reqbody.empty())
		_payloadOutgoing.push(new PayloadChunk((const uint8*) reqbody.c_str(), reqbody.length()));

    char tmp[80] = {0};
	snprintf(tmp, sizeof(tmp)-2, "%s:%s[", connId().c_str(), method2str(HttpMessage::method()));
	_txnId = tmp; _txnId += url +"]";
}

HttpRequest::~HttpRequest()
{
    ZQ::common::MutexGuard gGuard(_ua._locker);
	_ua._awaits.erase(connId());
}

ZQ::eloop::HttpMessage::Ptr HttpRequest::getResponse(int32 timeout, ICallBack* cbAsync)
{
	char tmp[60];
	snprintf(tmp, sizeof(tmp)-2, "%s %s [%s:%d", connId().c_str(), method2str(_method), _host.c_str(), _port);
	_txnId = tmp; _txnId += _uri +"]"; // +"?" +_qstr +"]";

	_cb = cbAsync;
	if (_respMsg)
		_respMsg->_statusCode = errAsyncInProgress;

	if (NULL ==_cb)
		_pEvent = new ZQ::common::Event();

	if (!_pEvent)
		return errAsyncInProgress;

	if (_respMsg)
		_respMsg->_statusCode = scRequestTimeout;

	_ua.enqueue(this);
	// _logger.hexDump(ZQ::common::Log::L_DEBUG, _reqBody.c_str(), (int)_reqBody.length(), _txnId.c_str(), true);

	if (!_pEvent)
		return errAsyncInProgress;

	if (timeout <=0)
		timeout = -1;

	if (!_pEvent->wait(timeout))
		return scRequestTimeout;

	return (ZQ::eloop::HttpMessage::StatusCodeEx)resp->statusCode();
}

HttpMessage::StatusCodeEx HttpRequest::OnBodyPayloadReceived(const char* data, size_t size)
{ 
	if (NULL != data && size>0) _respBody.append(data, size();
	return HttpMessage::errAsyncInProgress;
}


bool HttpRequest::startRequest()
{
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "startRequest() bind[%s] connecting to [%s:%d]"), _ua._bindIP.c_str(), _host.c_str(), _port);

	setError(errBindFail);
	if (!_ua._bindIP.empty() && _ua._bindIP !="0.0.0.0" && HttpConnection::bind4(_ua._bindIP.c_str(), 0) <0)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpRequest, "startRequest() failed to bind[%s]"), _ua._bindIP.c_str());
		return false;
	}
	
	setError(errConnectFail);
	if (HttpConnection::connect4(_host.c_str(), _port) <0)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpRequest, "startRequest() failed to connect to [%s:%d]"), _host.c_str(), _port);
		return false;
	}

	setError(errConnectTimeout);
	return true;
}

void HttpRequest::OnExecuted()
{
    HttpConnection::disconnect();
	if (_pEvent)
		_pEvent->signal();

	if (_cb)
		_cb->OnResult(_respMsg);
}

void HttpRequest::setError(int error, const char* errMsg)
{
	_ret = (StatusCodeEx) error;
	_retStr = errMsg ? errMsg : code2status(error);
}

void HttpRequest::OnMessagingError(int error, const char* errorDescription)
{
	_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpRequest, "onError error(%d) %s"), error, errorDescription);
	setError(error, errorDescription);
	OnExecuted();
}

void HttpRequest::OnMessageReceived(const ZQ::eloop::HttpMessage::Ptr msg)
{
	_respMsg = msg;
	for (HttpConnection::PayloadChunk::Ptr chunk = popReceivedPayload(); chunk; chunk = popReceivedPayload())
		_respBody += std::string((const char*)chunk->data(), chunk->len());

	if (_respMsg)
		setError(_respMsg->statusCode());

	_logger(ZQ::common::Log::L_INFO, CLOGFMT(HttpRequest, "req[%s] respond %d %s, bodylen[%d]"), _txnId.c_str(), _ret, _retStr.c_str(), _respBody.length());
	OnExecuted();
}

} }//namespace ZQ::eloop
