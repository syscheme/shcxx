#include "ZQ_common_conf.h"
#include "HttpClient.h"
#include "urlstr.h"
#include "TimeUtil.h"

#define TIMER_INTERVAL  (100)  // 100msec

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class HTTPUserAgent
// ---------------------------------------
HTTPUserAgent::HTTPUserAgent(ZQ::common::Log& logger, ZQ::eloop::Loop& loop, const std::string& userAgent, const std::string& bindIP)
: _log(logger), _eloop(loop), _userAgent(userAgent), _bindIP(bindIP)
{
	Async::init(_eloop);
	Timer::init(_eloop);
	Timer::start(TIMER_INTERVAL, TIMER_INTERVAL);
}

HTTPUserAgent::~HTTPUserAgent()
{
	Timer::stop();
	Timer::close();
	Async::close();

	ZQ::common::MutexGuard gGuard(_locker);
	_outgoings.clear();
	_awaits.clear();
}

HttpRequest::Ptr HTTPUserAgent::createRequest(HttpMessage::HttpMethod method, const std::string& url, const std::string& reqbody, const HttpMessage::Properties& params, const HttpMessage::Headers& headers)
{
	return new HttpRequest(*this, method, url, reqbody, params, headers);
}

HttpStream::Ptr  HTTPUserAgent::createStream(const std::string& url, HttpStream::IDownloadSink* cbDownload, const HttpMessage::Properties& params/* = HttpMessage::Properties()*/, const HttpMessage::Headers& headers/* = HttpMessage::Headers()*/)
{
	return new HttpStream(*this, url, cbDownload, params, headers);
}

void HTTPUserAgent::enqueue(HttpRequest::Ptr req)
{
	if (!req)
		return;

    ZQ::common::MutexGuard gGuard(_locker);
	_outgoings.push_back(req);
	Async::send();
}

void HTTPUserAgent::poll()
{
	std::vector<HttpRequest::Ptr> list2expire;
	std::string txnExpired, txnStarted;
	size_t cStarted =0, cAwaits=0;
	// step 1. about the out-going requests
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
			txnStarted += req->_txnId +",";
			cStarted++;
		}

		// step 2. about the await responses
		cAwaits = _awaits.size();
		for (RequestMap::iterator it = _awaits.begin(); it != _awaits.end();)
		{
			if (it->second)
			{
				if (it->second->getTimeLeft() >0)
					continue;

				list2expire.push_back(it->second);
			}

			_awaits.erase(it++);
		}

	}

	for (size_t i =0; i < list2expire.size(); i++)
	{
		try {
			list2expire[i]->dispatchResult();
			txnExpired += list2expire[i]->_txnId +",";
		}
		catch(...) {}
	}

	if ((cStarted + list2expire.size())>0)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(HTTPUserAgent, "%s[%s] started %d reqs[%s] and expired %d[%s], await-size %d"), _userAgent.c_str(), _bindIP.c_str(), cStarted, txnStarted.c_str(), list2expire.size(), txnExpired.c_str(), cAwaits);
}

// ---------------------------------------
// class HttpRequest
// ---------------------------------------
#define REQFMT(FUNC, FMT) CLOGFMT(HttpRequest, #FUNC "txn[%s] " FMT), _txnId.c_str()

HttpRequest::HttpRequest(HTTPUserAgent& ua, HttpMethod _method, const std::string& url, const std::string& reqbody, const Properties& params, const Properties& headers)
: _ua(ua), HttpMessage(MSG_REQUEST), HttpConnection(ua._log), _port(80), _cb(NULL), _stampRequested(0), _timeout(TIMEOUT_INF), _localPort(0), _localIP("0.0.0.0")
{
	// bound with a dummy response initially
	_respMsg = new HttpMessage(MSG_RESPONSE);

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

	_logger(ZQ::common::Log::L_DEBUG, REQFMT(HttpRequest, "created, bodylen %d"), reqbody.length());
}

HttpRequest::~HttpRequest()
{
    ZQ::common::MutexGuard gGuard(_ua._locker);
	_ua._awaits.erase(connId());
}

int HttpRequest::getTimeLeft() const
{
	return (_timeout>0 && TIMEOUT_INF != _timeout) ? (int)(_stampRequested + _timeout - ZQ::common::now()) : 60000; 
}

void HttpRequest::takeClientIP(const char* localIP, int port)
{
	if (NULL != localIP)
		_localIP = localIP;
	_localPort = (port>0) ?port: 0;
}

ZQ::eloop::HttpMessage::StatusCodeEx HttpRequest::setResult(ZQ::eloop::HttpMessage::StatusCodeEx error, const char* errMsg)
{
	if (!_respMsg)
		return error;

	_respMsg->_statusCode = (StatusCodeEx) error;

	if (errMsg)
	{	MAPSET(HttpMessage::Headers, _respMsg->_headers, "Warning", errMsg); }
	else _respMsg->_headers.erase("Warning");

	return error;
}

ZQ::eloop::HttpMessage::StatusCodeEx HttpRequest::waitResult(int32 timeout) // , ICallBack* cbAsync)
{
	_pEvent  = new ZQ::common::Event();

	if (timeout >0)
		timeout += TIMER_INTERVAL *2; // add additional 200msec

	subscribeResult(NULL, timeout);
	// _logger.hexDump(ZQ::common::Log::L_DEBUG, _reqBody.c_str(), (int)_reqBody.length(), _txnId.c_str(), true);

	if (!_pEvent->wait(timeout))
		return setResult(scRequestTimeout);

	return (ZQ::eloop::HttpMessage::StatusCodeEx) _respMsg->statusCode();
}

void HttpRequest::subscribeResult(IResponseSink* cb, int32 timeout) // , ICallBack* cbAsync)
{
	_cb = cb;
	_stampRequested = ZQ::common::now();
	_timeout = (timeout <=0) ? TIMEOUT_INF : timeout;

	setResult(scRequestTimeout);
	_ua.enqueue(this);
	// _logger.hexDump(ZQ::common::Log::L_DEBUG, _reqBody.c_str(), (int)_reqBody.length(), _txnId.c_str(), true);
}

HttpMessage::StatusCodeEx HttpRequest::OnBodyPayloadReceived(const uint8* data, size_t size)
{ 
	if (NULL != data && size>0) 
		_respBody.append((const char*)data, size);

	return errAsyncInProgress;
}

bool HttpRequest::startRequest()
{
	_logger(ZQ::common::Log::L_DEBUG, REQFMT(startRequest, "bind[%s] connecting to [%s:%d]"), _ua._bindIP.c_str(), _host.c_str(), _port);

	setResult(errBindFail);
	std::string clientIP = _localIP.empty() ? _ua._bindIP : _localIP;
	if (_localPort<=0)
		_localPort =0;
	if (!clientIP.empty() && clientIP !="0.0.0.0" && HttpConnection::bind4(clientIP.c_str(), _localPort) <0)
	{
		_logger(ZQ::common::Log::L_ERROR, REQFMT(startRequest, "failed to bind localIP[%s]"), clientIP.c_str());
		return false;
	}
	
	setResult(errConnectFail);
	if (HttpConnection::connect4(_host.c_str(), _port) <0)
	{
		_logger(ZQ::common::Log::L_ERROR, REQFMT(startRequest, "failed to connect to [%s:%d]"), _host.c_str(), _port);
		return false;
	}

	setResult(errConnectTimeout);
	return true;
}

void HttpRequest::dispatchResult()
{
    HttpConnection::disconnect();
	if (_pEvent)
		_pEvent->signal();

	if (_cb)
		_cb->OnHttpResult(this);

	ZQ::common::MutexGuard gGuard(_ua._locker);
	_ua._awaits.erase(connId());
}

void HttpRequest::OnMessagingError(int error, const char* errorDescription)
{
	_logger(ZQ::common::Log::L_ERROR, REQFMT(OnMessagingError, "error(%d) %s"), error, errorDescription);
	setResult((StatusCodeEx) error, errorDescription);
	dispatchResult();
}

void HttpRequest::OnMessageReceived(const ZQ::eloop::HttpMessage::Ptr msg)
{
	_respMsg = msg;
	for (HttpConnection::PayloadChunk::Ptr chunk = popReceivedPayload(); chunk; chunk = popReceivedPayload())
		_respBody += std::string((const char*)chunk->data(), chunk->len());

	if (_respMsg)
		setResult((StatusCodeEx)_respMsg->statusCode());

	_logger(ZQ::common::Log::L_INFO, REQFMT(OnMessageReceived, "respond %d, contentlen[%d]"), msg->statusCode(), _respBody.length());
	dispatchResult();
}

// ---------------------------------------
// class HttpStream
// ---------------------------------------
#undef REQFMT
#define REQFMT(FUNC, FMT) CLOGFMT(HttpStream, #FUNC "txn[%s] " FMT), _txnId.c_str()

HttpStream::HttpStream(HTTPUserAgent& ua, const std::string& url, IDownloadSink* cbDownload, const Properties& params, const Properties& headers)
: HttpRequest(ua, GET, url, "", params, headers), _cbDownload(cbDownload), _stampLastDownload(0)
{
}

HttpStream::~HttpStream()
{
}

int HttpStream::getTimeLeft() const
{
	if (_stampLastDownload > _stampRequested && _timeout >0 && TIMEOUT_INF != _timeout)
		return (int)(_stampLastDownload + _timeout - ZQ::common::now());
	
	return HttpRequest::getTimeLeft(); 
}

HttpMessage::StatusCodeEx HttpStream::OnBodyPayloadReceived(const uint8* data, size_t size)
{
	_stampLastDownload =ZQ::common::now();
	if (_cbDownload)
		_cbDownload->OnDownloadData(data, size);
	
	_logger(ZQ::common::Log::L_DEBUG, REQFMT(OnBodyPayloadReceived, "received %dB"), size);
	return errAsyncInProgress;
}

HttpMessage::StatusCodeEx HttpStream::OnHeadersReceived(const HttpMessage::Ptr resp)
{
	_respMsg = resp;

	// forward the received chunks to _cbDownload if there are any
	for (HttpConnection::PayloadChunk::Ptr chunk = popReceivedPayload(); chunk; chunk = popReceivedPayload())
	{
		if (!chunk || !_cbDownload)
			continue;

		_cbDownload->OnDownloadData(chunk->data(), chunk->len());
	}

	_logger(ZQ::common::Log::L_DEBUG, REQFMT(OnHeadersReceived, "respond %d"), resp->statusCode());
	dispatchResult();

	return HttpMessage::errAsyncInProgress;
}

} }//namespace ZQ::eloop
