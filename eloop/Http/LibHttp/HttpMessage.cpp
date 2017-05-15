#include <stdio.h>
#include <time.h>
#include "http_parser.h"
#include "HttpMessage.h"



namespace ZQ {
namespace eloop {
// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
Code2StatusMapInitializer c2smapinitializer;
HttpMessage::HttpMessage(MessgeType type)
		:_Type((http_parser_type)type),
		_Method(GET),
		_Code(0),
		_bChunked(false),
		_bKeepAlive(false),
		_VerMajor(0),
		_VerMinor(0),
		_BodyLength(0)
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

std::string HttpMessage::uint2hex(unsigned long u, size_t alignLen, char paddingChar)
{
	static char hexCharTbl[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};

	std::string ret;
	ret.reserve(8);

	while(u)
	{
		ret.insert(ret.begin(), hexCharTbl[u & 0xF]);
		u >>= 4;
	}

	if(ret.empty())
		return "0";

	while(ret.size() < alignLen)
		ret.insert(ret.begin(), paddingChar);

	return ret;
}

const std::string& HttpMessage::code2status( int code ) {
	std::map<int,std::string>::const_iterator it = code2statusmap.find(code);
	if( it != code2statusmap.end())
		return it->second;
	return unknownstatus;
}

HttpMessage::HttpMethod HttpMessage::method() const{
	return _Method;
}

void HttpMessage::method(HttpMethod mtd){
	_Method = mtd;
}

void HttpMessage::code( int c) {
	_Code = c;
}	

int	HttpMessage::code() const {
	return _Code;
}	

void HttpMessage::status( const std::string& s) {
	_Status = s;
}

const std::string& HttpMessage::status() const {
	return _Status;
}		

const std::string& HttpMessage::url() const {
	return _Uri;
}

void HttpMessage::url(const std::string& url) {
	_Uri = url;
}

const std::string& HttpMessage::queryArgument(const std::string& k)
{
	std::map<std::string, std::string>::const_iterator it = _argument.find(k);
	if (it == _argument.end())
		return _DummyHeaderValue;
	return it->second;
}

void HttpMessage::queryArgument(const std::string& k, const std::string& v)
{
	 _argument[k] = v;
}

std::map<std::string, std::string> HttpMessage::queryArguments()
{
	return _argument;
}



const std::string& HttpMessage::header( const std::string& key) const {
	ZQ::common::MutexGuard gd(_Locker);
	HEADERS::const_iterator it = _Headers.find(key);
	if( it == _Headers.end())
		return _DummyHeaderValue;
	return it->second;
}

void HttpMessage::eraseHeader( const std::string& key ) {
	ZQ::common::MutexGuard gd(_Locker);
	_Headers.erase( key );
}

bool HttpMessage::keepAlive() const {
	return _bKeepAlive;
}	
void HttpMessage::keepAlive( bool b) {
	_bKeepAlive = b;		
}	

bool HttpMessage::chunked() const {
	return _bChunked;
}

void HttpMessage::chunked(bool b) {
	_bChunked = b;
	if(b){
		_BodyLength = -1;
	}
}	

int64 HttpMessage::contentLength() const {
	if(_bChunked)
		return 0;
	return _BodyLength;
}	
void HttpMessage::contentLength(int64 length) {
	if(length < 0)
		return;
	_bChunked = false;
	_BodyLength = length;
}

std::string HttpMessage::toRaw() 
{
	std::ostringstream oss;
	static const std::string line_term = "\r\n";
	ZQ::common::MutexGuard gd(_Locker);
	if( !_RawMessage.empty())
		return _RawMessage;
	if(_Type != HTTP_RESPONSE ) {
		oss<< http_method_str((http_method)_Method) << " " << _Uri << " " << "HTTP/1.1"<< line_term;
	} else {
		oss<<"HTTP/1.1" <<" " <<_Code<<" "<<_Status<<line_term;
	}
	if(_bChunked) {
		_Headers["Transfer-Encoding"] = "chunked";
		_Headers.erase("Content-Length");
	} else {
		_Headers.erase("Transfer-Encoding");
//		if(_BodyLength > 0 ) {
			std::ostringstream ossBL;ossBL<<_BodyLength;
			_Headers["Content-Length"] = ossBL.str();
// 		} else {
// 			_Headers.erase("Content-Length");
// 		}
	}

	if(_bKeepAlive) {
		_Headers["Connection"] = "keep-alive";
	} else {
		_Headers["Connection"] = "close";
	}
	HEADERS::const_iterator it = _Headers.begin();
	for( ; it != _Headers.end() ; it ++ ) {
		oss<<it->first<<": "<<it->second<<line_term;
	}
	oss<<line_term;
	_RawMessage = oss.str();
	return _RawMessage;	
}
} }//namespace ZQ::eloop
