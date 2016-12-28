#ifndef __HTTP_CLIENT_h__
#define __HTTP_CLIENT_h__

#include "httpMessage.h"
#include "httpProcessor.h"

class HttpClient : public HttpProcessor {
public:
	HttpClient();
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
	virtual void	onHttpDataSent(bool keepAlive) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void	onHttpDataReceived( size_t size ) { }

	virtual bool	onHttpMessage( const HttpMessage::Ptr msg) { 
		abort();
		return false; 
	}

	virtual bool	onHttpBody( const char* data, size_t size) { return false; }

	virtual void	onHttpComplete() { }

	virtual void	onHttpError( int error ) { }

private:
		HttpMessage::Ptr		mSendMsg;

};

#endif