#ifndef __HTTP_CLIENT_h__
#define __HTTP_CLIENT_h__

#include "HttpConnection.h"

#include <list>

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API HTTPUserAgent;
class ZQ_ELOOP_HTTP_API HttpRequest;

// ---------------------------------------
// class HttpRequest
// ---------------------------------------
class HttpRequest : public HttpConnection, public HttpMessage
{
	friend class HTTPUserAgent;
public:
	typedef ZQ::common::Pointer<HttpRequest> Ptr;

	class ICallBack {
	public: 
		virtual void OnResult(const ZQ::eloop::HttpMessage::Ptr& resp) =0;
	};

public:
	virtual ~HttpRequest();

	virtual StatusCodeEx getResponse(ZQ::eloop::HttpMessage::Ptr& resp, int32 timeout=-1, ICallBack* cbAsync =NULL);

//	virtual StatusCodeEx POST(int32 timeout=-1);
//    virtual StatusCodeEx GET(int32 timeout=-1);

private: //disallow construction and compier
	HttpRequest(HTTPUserAgent& ua, HttpMethod mtd, const std::string& url, const std::string& reqbody="", const Properties& params = Properties(), const Properties& headers = Properties());
	HttpRequest(const HttpRequest&);
	HttpRequest& operator=(const HttpRequest&);

	bool wait(int32 timeout=-1);

protected: // override of HttpConnection
	// about message receiving triggered by HTTP parser
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnHeadersReceived(const HttpMessage::Ptr resp) { _respMsg = resp; return HttpMessage::errAsyncInProgress; }

	virtual void OnMessageReceived(const HttpMessage::Ptr resp);

	virtual void OnMessagingError(int error, const char* errorDescription);

protected: // override of HttpMessage
	virtual std::string uri();
	void	setError(int error, const char* errMsg=NULL);

protected:
	// OnBodyPayloadReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void    OnExecuted();

	// the kickoff point
	virtual bool startRequest();
	//bool beginRequest( HttpMessage::Ptr msg, const std::string& url);
	//bool beginRequest( HttpMessage::Ptr msg, const std::string& ip, const unsigned int& port);

	HTTPUserAgent& _ua;
	ZQ::common::Event::Ptr    _pEvent;
	ICallBack* _cb;

	std::string  _txnId;
	std::string _host;
	int _port;

	StatusCodeEx   _ret;
	std::string  _retStr;

	ZQ::eloop::HttpMessage::Ptr _respMsg;
    std::string  _respBody;
};

// ---------------------------------------
// class HTTPUserAgent
// ---------------------------------------
// HTTPUserAgent inherit ZQ::eloop::Async in order to call send/receive within the eloop thread via Async::OnAsync()
class HTTPUserAgent : public ZQ::eloop::Async
{
friend class HttpRequest;

public:
	HTTPUserAgent(ZQ::common::Log& logger, ZQ::eloop::Loop& loop, const std::string& userAgent="TianShan", const std::string& bindIP="");
    virtual ~HTTPUserAgent();

	HttpRequest::Ptr createRequest(HttpMessage::HttpMethod method, const std::string& url, const std::string& reqbody="", const HttpMessage::Properties& params = HttpMessage::Properties(), const HttpMessage::Headers& headers = HttpMessage::Headers());
	// HttpStream::Ptr createDownStream(const std::string& url, const Properties& params = Properties(), const Properties& headers = Properties());

protected: // impl of ZQ::eloop::Async
	virtual void OnAsync();

protected:
	void enqueue(HttpRequest::Ptr req);

	std::string          _userAgent;
	ZQ::common::Log&     _log;
	std::string          _bindIP;
	ZQ::eloop::Loop&     _eloop;

private:
	typedef std::map<std::string, HttpRequest::Ptr>  RequestMap;
	typedef std::list<HttpRequest::Ptr>  RequestQueue;
    RequestMap            _awaits; // map of connId to HttpRequest::Ptr
	RequestQueue          _outgoings;
	ZQ::common::Mutex     _locker;
};

} }//namespace ZQ::eloop

#endif