#ifndef __HTTP_CLIENT_h__
#define __HTTP_CLIENT_h__

#include "HttpMessage.h"
#include "HttpConnection.h"

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpClient
// ---------------------------------------
class HttpClient : public HttpConnection {
public:
	HttpClient(ZQ::common::Log& logger);
	virtual void OnConnected(ElpeError status);
	bool beginRequest( HttpMessage::Ptr msg, const std::string& url);

private:
	//disallow copy construction
	HttpClient(const HttpClient&);
	HttpClient& operator=(const HttpClient&);

public:
	virtual ~HttpClient(void);


protected:
	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataSent() { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataReceived( size_t size ) { }

	virtual bool	onHeadersEnd( const HttpMessage::Ptr msg) { 
		abort();
		return false; 
	}

	virtual bool	onBodyData( const char* data, size_t size) { return false; }

	virtual void	onMessageCompleted() { }

	virtual void	onParseError( int error,const char* errorDescription ) { }

private:
		HttpMessage::Ptr		_SendMsg;

};
} }//namespace ZQ::eloop
#endif