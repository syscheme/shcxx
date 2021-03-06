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
// HttpRequest represent a simple HTTP request to the server-side
class HttpRequest : public HttpConnection, public HttpMessage
{
public:
	friend class HTTPUserAgent;
	typedef ZQ::common::Pointer<HttpRequest> Ptr;

	class IResponseSink {
	public:
		virtual void OnHttpResult(const Ptr& req) =0;
	};

public:
	virtual ~HttpRequest();

	// trigger the request and wait for the result, the response can be accessed via response()
	virtual ZQ::eloop::HttpMessage::StatusCodeEx waitResult(int32 timeout=TIMEOUT_INF);

	// trigger the request async
	virtual void subscribeResult(IResponseSink* cb, int32 timeout =TIMEOUT_INF);

	virtual int getTimeLeft() const;
	void takeClientIP(const char* localIP, int port =0);

	virtual ZQ::eloop::HttpMessage::Ptr response(std::string& body) const { body =_respBody; return _respMsg; }

protected:
	HttpRequest(HTTPUserAgent& ua, HttpMethod mtd, const std::string& url, const std::string& reqbody="", const Properties& params = Properties(), const Properties& headers = Properties());

private: //disallow construction and compier
	HttpRequest(const HttpRequest&);
	HttpRequest& operator=(const HttpRequest&);

protected: // override of HttpConnection
	// about message receiving triggered by HTTP parser
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnHeadersReceived(const HttpMessage::Ptr resp) { _respMsg = resp; return HttpMessage::errAsyncInProgress; }
	virtual void OnMessageReceived(const HttpMessage::Ptr resp);
	virtual void OnMessagingError(int error, const char* errorDescription);

	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnBodyPayloadReceived(const uint8* data, size_t size);

protected:
	StatusCodeEx setResult(StatusCodeEx error, const char* errMsg =NULL); // all the status will be apply to _respMsg
	virtual void dispatchResult();

	// the kickoff point
	virtual bool startRequest();

	HTTPUserAgent& _ua;
	ZQ::common::Event::Ptr _pEvent;

	std::string  _txnId;
	std::string  _host, _localIP;
	int _port, _localPort;

	ZQ::eloop::HttpMessage::Ptr _respMsg;
    std::string  _respBody;
	IResponseSink* _cb;
	int64          _stampRequested;
	int32          _timeout;
};

// ---------------------------------------
// class HttpStream
// ---------------------------------------
// HttpStream extends HttpRequest to transfer big size data stream, currently cover download stream
class HttpStream : public HttpRequest
{
	friend class HTTPUserAgent;
public:
	typedef ZQ::common::Pointer<HttpStream> Ptr;

public:
	virtual ~HttpStream();

	class IDownloadSink
	{
		friend class HttpStream;
	public:
		virtual void OnDownloadError(const int err, const std::string& msg) = 0;
		virtual void OnDownloadData(const uint8* data, const size_t size) = 0;
		virtual void OnEndOfDownloadStream(bool bBroken =false) = 0;
	};

	int getTimeLeft() const;

private: //disallow construction and compier
	HttpStream(HTTPUserAgent& ua, const std::string& url, IDownloadSink* cbDownload, const Properties& params = Properties(), const Properties& headers = Properties());
	HttpStream(const HttpRequest&);
	HttpStream& operator=(const HttpRequest&);

private: // final overrides of HttpRequest
	StatusCodeEx OnHeadersReceived(const HttpMessage::Ptr resp);
	StatusCodeEx OnBodyPayloadReceived(const uint8* data, size_t size);
	void OnMessagingError(int error, const char* errorDescription) { if (_cbDownload) _cbDownload->OnDownloadError(error, errorDescription); }
	void OnMessageReceived(const HttpMessage::Ptr resp) { if (_cbDownload) _cbDownload->OnEndOfDownloadStream(); }

	IDownloadSink* _cbDownload;
	int64          _stampLastDownload;
};

// ---------------------------------------
// class HTTPUserAgent
// ---------------------------------------
// HTTPUserAgent inherit ZQ::eloop::Async in order to call send/receive within the eloop thread via Async::OnAsync()
class HTTPUserAgent : public InterruptibleLoop
{
friend class HttpRequest;

public:
	HTTPUserAgent(ZQ::common::Log& logger, const std::string& userAgent="TianShan", const std::string& bindIP="", int msHeatbeat=-1, int cpuId=-1);
    virtual ~HTTPUserAgent();

	HttpRequest::Ptr createRequest(HttpMessage::HttpMethod method, const std::string& url, const std::string& reqbody="", const HttpMessage::Properties& params = HttpMessage::Properties(), const HttpMessage::Headers& headers = HttpMessage::Headers());
	HttpStream::Ptr  createStream(const std::string& url, HttpStream::IDownloadSink* cbDownload, const HttpMessage::Properties& params = HttpMessage::Properties(), const HttpMessage::Headers& headers = HttpMessage::Headers());

	ZQ::common::Log& getLogger(){return _log;}

protected:
	// impl of InterruptibleLoop
	virtual void poll(bool isHeartbeat=false);

protected:
	void enqueue(HttpRequest::Ptr req);

	std::string          _userAgent;
	ZQ::common::Log&     _log;
	std::string          _bindIP;

private:
	typedef std::map<std::string, HttpRequest::Ptr>  RequestMap;
	typedef std::list<HttpRequest::Ptr>  RequestQueue;
    RequestMap            _awaits; // map of connId to HttpRequest::Ptr
	RequestQueue          _outgoings;
	ZQ::common::Mutex     _locker;

	int64                 _lastPoll;
};

} }//namespace ZQ::eloop

#endif
