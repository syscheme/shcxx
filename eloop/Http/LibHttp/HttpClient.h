#ifndef __HTTP_CLIENT_h__
#define __HTTP_CLIENT_h__

#include "HttpMessage.h"
#include "HttpConnection.h"

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API HttpClient;
// ---------------------------------------
// class HttpClient
// ---------------------------------------
class HttpClient : public HttpConnection {
public:
	HttpClient(ZQ::common::Log& logger);
	
	virtual void OnConnected(ElpeError status);
	bool beginRequest( HttpMessage::Ptr msg, const std::string& url);
	bool beginRequest( HttpMessage::Ptr msg, const std::string& ip, const unsigned int& port);

private:
	//disallow copy construction
	HttpClient(const HttpClient&);
	HttpClient& operator=(const HttpClient&);

public:
	virtual ~HttpClient(void);


protected:
	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataSent(size_t size) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataReceived( size_t size ) { }

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg) { 
		return false; 
	}

	virtual bool	onBodyData( const char* data, size_t size) { return false; }

	virtual void	onMessageCompleted() { }

	virtual void	OnConnectionError( int error,const char* errorDescription );

	HttpMessage::Ptr		_req;
private:
	ZQ::common::Log&		_logger;
};

} }//namespace ZQ::eloop

#endif