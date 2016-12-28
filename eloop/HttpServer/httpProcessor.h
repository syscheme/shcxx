#ifndef __HTTP_PROCESSOR_h__
#define __HTTP_PROCESSOR_h__

#include "httpParser.h"
#include "eloop_net.h"
#include <set>
#include <assert.h>
using namespace ZQ::eloop;


// ---------------------------------------
// class HttpProcessor
// ---------------------------------------
class HttpProcessor:public TCP,public ParserCallback
{
public:
	virtual ~HttpProcessor();

	int send(const char* buf,size_t len);
	void setkeepAlive(bool alive){mbOutgoingKeepAlive = alive;}
	bool getkeepAlive(){ return mbOutgoingKeepAlive;}

protected:
	HttpProcessor( bool clientSide );
	void			reset( ParserCallback* callback = NULL );
	int getSendCount(){return mSendCount;}
	
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);

private:
	HttpProcessor( const HttpProcessor&);
	HttpProcessor& operator=( const HttpProcessor&);

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