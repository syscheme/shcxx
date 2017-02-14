#ifndef __HTTP_CLIENT_h__
#define __HTTP_CLIENT_h__

#include "httpMessage.h"
#include "HttpConnection.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class HttpClient
// ---------------------------------------
class HttpClient : public HttpConnection
{
public:
	HttpClient();
	virtual ~HttpClient(void);

	bool beginRequest( HttpMessage::Ptr msg, const std::string& url);

private:
	//disallow copier constructions
	HttpClient(const HttpClient&);
	HttpClient& operator=(const HttpClient&);

protected: // overrides
	virtual void OnConnected(ElpeError status);

	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataSent(bool keepAlive) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataReceived( size_t size ) { }

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg)
	{ 
		abort();
		return false; 
	}

	virtual bool	onHttpBody( const char* data, size_t size) { return false; }
	virtual void	onHttpComplete() { }
	virtual void	onHttpError( int error ) { }

private:
	HttpMessage::Ptr		_SendMsg;
};

} }//namespace ZQ::eloop
#endif