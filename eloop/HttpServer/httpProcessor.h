#ifndef __HTTP_PROCESSOR_h__
#define __HTTP_PROCESSOR_h__

#include "httpParser.h"
#include "eloop_net.h"
#include <set>
#include <assert.h>
using namespace ZQ::eloop;


// ---------------------------------------
// class HttpConnection
// ---------------------------------------
class HttpConnection: public TCP, public ParserCallback
{
public:
	virtual ~HttpConnection();

	int send(const char* buf,size_t len);
	void setkeepAlive(bool alive){mbOutgoingKeepAlive = alive;}
	bool getkeepAlive(){ return mbOutgoingKeepAlive;}

protected:
	HttpConnection( bool clientSide );
	void			reset( ParserCallback* callback = NULL );
	int getSendCount(){return mSendCount;}
	
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);

private:
	HttpConnection( const HttpConnection&);
	HttpConnection& operator=( const HttpConnection&);

protected:

	virtual void onHttpError(int err) {}

	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void onHttpDataSent(bool keepAlive) { }

	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void onHttpDataReceived(size_t size) { }

private:
	http_parser_type		mParseType;
	HttpParser				mHttpParser;

	HttpMessage::Ptr		mIncomingMsg;

	int						mSendCount;
	bool					mbOutgoingKeepAlive;
};

#endif