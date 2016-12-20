#include <stdio.h>
#include <time.h>
#include "httpMessage.h"

HttpMessage::HttpMessage(http_parser_type type)
		:mType(type),
		mMethod(HTTP_GET),
		mCode(0),
		mbChunked(false),
		mbKeepAlive(false),
		mVerMajor(0),
		mVerMinor(0),
		mBodyLength(0)
{

}

HttpMessage::~HttpMessage(){

}


std::string HttpMessage::httpdate( int delta ) {
	char buffer[64] = {0};

	struct tm tnow;
	time_t time_now;
	time(&time_now);
	time_now += (time_t)delta;
#ifdef ZQ_OS_MSWIN   
	gmtime_s(&tnow, &time_now);
	struct tm* t = &tnow;
#elif defined ZQ_OS_LINUX
	struct tm* t = gmtime_r( &time_now, &tnow);		
#endif//ZQ_OS
	sprintf(buffer,"%3s, %02d %3s %04d %02d:%02d:%02d GMT", 
		httpDateStrWeekDay[t->tm_wday],	
		t->tm_mday,
		httpDateStrMonth[t->tm_mon],
		t->tm_year+1900,
		t->tm_hour,
		t->tm_min,
		t->tm_sec);

	return buffer;
}

const std::string& HttpMessage::code2status( int code ) {
	std::map<int,std::string>::const_iterator it = code2statusmap.find(code);
	if( it != code2statusmap.end())
		return it->second;
	return unknownstatus;
}

http_method HttpMessage::method() const{
	return mMethod;
}

void HttpMessage::method(http_method mtd){
	mMethod = mtd;
}

void HttpMessage::code( int c) {
	mCode = c;
}	

int	HttpMessage::code() const {
	return mCode;
}	

void HttpMessage::status( const std::string& s) {
	mStatus = s;
}

const std::string& HttpMessage::status() const {
	return mStatus;
}		

const std::string& HttpMessage::url() const {
	return mUri;
}

void HttpMessage::url(const std::string& url) {
	mUri = url;
}

const std::string& HttpMessage::header( const std::string& key) const {
	ZQ::common::MutexGuard gd(mLocker);
	HEADERS::const_iterator it = mHeaders.find(key);
	if( it == mHeaders.end())
		return mDummyHeaderValue;
	return it->second;
}

void HttpMessage::eraseHeader( const std::string& key ) {
	ZQ::common::MutexGuard gd(mLocker);
	mHeaders.erase( key );
}

bool HttpMessage::keepAlive() const {
	return mbKeepAlive;
}	
void HttpMessage::keepAlive( bool b) {
	mbKeepAlive = b;		
}	

bool HttpMessage::chunked() const {
	return mbChunked;
}

void HttpMessage::chunked(bool b) {
	mbChunked = b;
	if(b){
		mBodyLength = 0;
	}
}	

int64 HttpMessage::contentLength() const {
	if(mbChunked)
		return 0;
	return mBodyLength;
}	
void HttpMessage::contentLength(int64 length) {
	if(length < 0)
		return;
	mbChunked = false;
	mBodyLength = length;
}

std::string HttpMessage::toRaw() 
{
	std::ostringstream oss;
	static const std::string line_term = "\r\n";
	ZQ::common::MutexGuard gd(mLocker);
	if( !mRawMessage.empty())
		return mRawMessage;
	if(mType != HTTP_RESPONSE ) {
		oss<< http_method_str(mMethod) << " " << mUri << " " << "HTTP/1.1"<< line_term;
	} else {
		oss<<"HTTP/1.1" <<" " <<mCode<<" "<<mStatus<<line_term;
	}
	if(mbChunked) {
		mHeaders["Transfer-Encoding"] = "chunked";
		mHeaders.erase("Content-Length");
	} else {
		mHeaders.erase("Transfer-Encoding");
		if(mBodyLength > 0 ) {
			std::ostringstream ossBL;ossBL<<mBodyLength;
			mHeaders["Content-Length"] = ossBL.str();
		} else {
			mHeaders.erase("Content-Length");
		}
	}

	if(mbKeepAlive) {
		mHeaders["Connection"] = "keep-alive";
	} else {
		mHeaders["Connection"] = "close";
	}
	HEADERS::const_iterator it = mHeaders.begin();
	for( ; it != mHeaders.end() ; it ++ ) {
		oss<<it->first<<": "<<it->second<<line_term;
	}
	oss<<line_term;
	mRawMessage = oss.str();
	return mRawMessage;	
} 