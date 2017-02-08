#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "http_parser.h"
#include "httpMessage.h"


// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
HttpMessage::HttpMessage(MessgeType type)
		:_Type((http_parser_type)type),
		_Method(HTTP_GET),
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

const std::string& HttpMessage::code2status( int code ) {
	std::map<int,std::string>::const_iterator it = code2statusmap.find(code);
	if( it != code2statusmap.end())
		return it->second;
	return unknownstatus;
}

http_method HttpMessage::method() const{
	return _Method;
}

void HttpMessage::method(http_method mtd){
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
		_BodyLength = 0;
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
		oss<< http_method_str(_Method) << " " << _Uri << " " << "HTTP/1.1"<< line_term;
	} else {
		oss<<"HTTP/1.1" <<" " <<_Code<<" "<<_Status<<line_term;
	}
	if(_bChunked) {
		_Headers["Transfer-Encoding"] = "chunked";
		_Headers.erase("Content-Length");
	} else {
		_Headers.erase("Transfer-Encoding");
		if(_BodyLength > 0 ) {
			std::ostringstream ossBL;ossBL<<_BodyLength;
			_Headers["Content-Length"] = ossBL.str();
		} else {
			_Headers.erase("Content-Length");
		}
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


/*
// --------------------------------------------------
// class HttpParser
// --------------------------------------------------
HttpParser::HttpParser(http_parser_type type)
:_Type(type),
_Stopped(false),
_HeaderValue(NULL)
{
	_ParserSettings.on_body				= HttpParser::on_body;
	_ParserSettings.on_header_field		= HttpParser::on_header_field;
	_ParserSettings.on_header_value		= HttpParser::on_header_value;
	_ParserSettings.on_headers_complete	= HttpParser::on_headers_complete;
	_ParserSettings.on_message_begin	= HttpParser::on_message_begin;
	_ParserSettings.on_message_complete	= HttpParser::on_message_complete;
	_ParserSettings.on_status			= HttpParser::on_status;
	_ParserSettings.on_url				= HttpParser::on_uri;

	reset(NULL);
}

HttpParser::~HttpParser()
{

}

int HttpParser::lastError() const{
	return (int)_Parser.http_errno;
}

void HttpParser::reset(IHttpParseSink* cb){
	_Callback	= cb;
	http_parser_init(&_Parser,_Type);
	_Parser.data = reinterpret_cast<void*>(this);
	_Stopped = false;
	_HeaderField.clear();
	_HeaderValue = NULL;
	_ParserState = STATE_INIT;
	_Message = new HttpMessage(_Type);
}

size_t HttpParser::parse( const char* data, size_t size) {
	return http_parser_execute(&_Parser, &_ParserSettings, data, size);
}

int	HttpParser::onMessageBegin( ){
	_ParserState = STATE_HEADERS;
	return 0;
}

int	HttpParser::onHeadersComplete(){
	assert(_Callback != NULL);
	_Message->contentLength((int64)_Parser.content_length);
	_Message->keepAlive((_Parser.flags & F_CONNECTION_KEEP_ALIVE) != 0);
	_Message->chunked((_Parser.flags & F_CHUNKED) != 0 );
	_Message->code((int)_Parser.status_code);
	_Message->method((http_method)_Parser.method);
	_Message->setVersion(_Parser.http_major, _Parser.http_minor);
	_ParserState = STATE_BODY;
	if(_Parser.http_errno == 0) {
		if(!_Callback->onHeadersEnd(_Message))
			return -1;//user cancel parsing procedure
		return 0;
	}
	return -2;//failed to parse http raw message
}

int	HttpParser::onMessageComplete(){
	assert(_Callback != NULL);
	_Callback->onMessageCompleted();
	_ParserState = STATE_COMPLETE;
	_Message = NULL;
	return 0;
}

int	HttpParser::onUri(const char* at, size_t size){
	_Message->_Uri.append(at, size);
	return 0;
}

int	HttpParser::onStatus(const char* at, size_t size){
	_Message->_Status.append(at, size);
	return 0;
}

int	HttpParser::onHeaderField(const char* at, size_t size){
	_HeaderValue = NULL;
	_HeaderField.append(at,size);
	return 0;
}

int	HttpParser::onHeaderValue(const char* at, size_t size){
	if(_HeaderValue == NULL ) {
		std::pair<HttpMessage::HEADERS::iterator,bool> ret = _Message->_Headers.insert(
			HttpMessage::HEADERS::value_type(_HeaderField, std::string(at,size)));
		if(!ret.second){
			_Message->_Headers.erase(_HeaderField);
			ret = _Message->_Headers.insert(
				HttpMessage::HEADERS::value_type(_HeaderField, std::string(at,size)));
			assert(ret.second);
		}
		_HeaderValue = &ret.first->second;
		_HeaderField.clear();
	} else {
		_HeaderValue->append(at, size);
	}
	return 0;
}

int	HttpParser::onBody(const char* at, size_t size){
	assert(_Callback!=NULL);
	if(!_Callback->onBodyData(at, size))
		return -1;//user cancel parsing procedure
	return 0;
}

int HttpParser::on_message_begin(http_parser* parser){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onMessageBegin();
}

int HttpParser::on_headers_complete(http_parser* parser){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onHeadersComplete();
}

int HttpParser::on_message_complete(http_parser* parser){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onMessageComplete();
}

int HttpParser::on_uri(http_parser* parser,const char* at,size_t size){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onUri(at, size);
}

int HttpParser::on_status(http_parser* parser, const char* at, size_t size){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onStatus(at, size);
}

int HttpParser::on_header_field(http_parser* parser, const char* at, size_t size){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onHeaderField(at, size);
}

int HttpParser::on_header_value(http_parser* parser, const char* at, size_t size){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onHeaderValue(at, size);
}

int HttpParser::on_body(http_parser* parser, const char* at, size_t size){
	HttpParser* pThis = reinterpret_cast<HttpParser*>(parser->data);
	return pThis->onBody(at, size);
}*/