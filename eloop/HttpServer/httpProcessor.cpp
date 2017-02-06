#include "httpProcessor.h"


// ---------------------------------------
// class HttpProcessor
// ---------------------------------------
HttpProcessor::HttpProcessor(bool clientSide)
		:mParseType(clientSide?HTTP_RESPONSE:HTTP_REQUEST),
		mHttpParser(mParseType),
		mbOutgoingKeepAlive(true),
//		mbSending(false),
		mSendCount(0),
		mIncomingMsg(NULL)

{
	reset();
}


HttpProcessor::~HttpProcessor() {
}

void HttpProcessor::reset(ParserCallback* p ) {
	if(!p)
		p = dynamic_cast<ParserCallback*>(this);
	mHttpParser.reset(p);
}

int HttpProcessor::send(const char* buf,size_t len)
{
	mSendCount++;
	return write(buf,len);
}


void HttpProcessor::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		fprintf(stderr, "Read error %s\n",  errName((ZQ::eloop::Handle::ElpeError)nread));
		onHttpError(ERR_RECVFAIL);
		return;
	}
	std::string str = buf;
//	printf("recv data:%s,len = %d,size = %d\n", buf,nread,str.size());

	size_t nparsed = mHttpParser.parse(buf, nread);
	if(nparsed != nread){
		onHttpError(mHttpParser.lastError());
		return;
	}
	if(!mHttpParser.headerComplete()){
		if(mIncomingMsg != NULL)
			assert(false);
		return;
	} 
	mIncomingMsg = mHttpParser.currentHttpMessage();		

	if( mHttpParser.httpComplete()) {
		reset();
	}
	onHttpDataReceived(nread);	
}

void HttpProcessor::OnWrote(ElpeError status)
{
//	mbSending = false;
	mSendCount--;
	if (status != elpeSuccess)
	{
		fprintf(stderr, "send error %s\n",  errDesc(status)); 
		onHttpError(ERR_SENDFAIL);
		return;
	}
	
	// if whole http raw message has been sent
	// mOutgoingMsg should be NULL
	onHttpDataSent(mbOutgoingKeepAlive);
/*
	if( mOutgoingMsg == NULL ) {
		onHttpEndSent(mbOutgoingKeepAlive);
	}*/
}