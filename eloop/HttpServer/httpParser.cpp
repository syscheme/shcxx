#include "httpParser.h"
#include <assert.h>

const char* ErrorCodeToStr( ErrorCode c ) {
	switch( c ) {
		case ERR_ERROR:		return "Generic Error";
		case ERR_EOF:		return "EOF";
		case ERR_INVAID:	return "Invalid Paramater";
		case ERR_ADDRINUSE:	return "Address In Use";
		case ERR_TIMEOUT:	return "Timed Out";
		case ERR_CONNREFUSED:	return "Connection Refused";
		case ERR_RECVFAIL:	return "Recv Failed";
		case ERR_SENDFAIL:	return "Send Fail";
		case ERR_EAGAIN:	return "Work In Progress";
		case ERR_SOCKETVAIN:	return "Socket Closed/Operation In Progress";
		case ERR_MEMVAIN:	return "Not Enough Memory";
		case ERR_EPOLLREGISTERFAIL:	return "EPOLL Error";
		case ERR_EPOLLEXCEPTION:		return "EPOLL Exception";
		case ERR_EOF2:		return "EOF2";
		case ERR_EOF3:		return "EOF3";
		default:			return "Unknown Error";
	}
}

HttpParser::HttpParser(http_parser_type type)
	:mType(type),
	mStopped(false),
	mHeaderValue(NULL)
{
	mParserSettings.on_body				= HttpParser::on_body;
	mParserSettings.on_header_field		= HttpParser::on_header_field;
	mParserSettings.on_header_value		= HttpParser::on_header_value;
	mParserSettings.on_headers_complete	= HttpParser::on_headers_complete;
	mParserSettings.on_message_begin	= HttpParser::on_message_begin;
	mParserSettings.on_message_complete	= HttpParser::on_message_complete;
	mParserSettings.on_status			= HttpParser::on_status;
	mParserSettings.on_url				= HttpParser::on_uri;

	reset(NULL);
}

HttpParser::~HttpParser()
{

}

int HttpParser::lastError() const{
	return (int)mParser.http_errno;
}

void HttpParser::reset(ParserCallback* cb){
	mCallback	= cb;
	http_parser_init(&mParser,mType);
	mParser.data = reinterpret_cast<void*>(this);
	mStopped = false;
	mHeaderField.clear();
	mHeaderValue = NULL;
	mParserState = STATE_INIT;
	mMessage = new HttpMessage(mType);
}

size_t HttpParser::parse( const char* data, size_t size) {
	return http_parser_execute(&mParser, &mParserSettings, data, size);
}

int	HttpParser::onMessageBegin( ){
	mParserState = STATE_HEADERS;
	return 0;
}

int	HttpParser::onHeadersComplete(){
	assert(mCallback != NULL);
	mMessage->contentLength((int64)mParser.content_length);
	mMessage->keepAlive((mParser.flags & F_CONNECTION_KEEP_ALIVE) != 0);
	mMessage->chunked((mParser.flags & F_CHUNKED) != 0 );
	mMessage->code((int)mParser.status_code);
	mMessage->method((http_method)mParser.method);
	mMessage->setVersion(mParser.http_major, mParser.http_minor);
	mParserState = STATE_BODY;
	if(mParser.http_errno == 0) {
		if(!mCallback->onHttpMessage(mMessage))
			return -1;//user cancel parsing procedure
		return 0;
	}
	return -2;//failed to parse http raw message
}

int	HttpParser::onMessageComplete(){
	assert(mCallback != NULL);
	mCallback->onHttpComplete();
	mParserState = STATE_COMPLETE;
	mMessage = NULL;
	return 0;
}

int	HttpParser::onUri(const char* at, size_t size){
	mMessage->mUri.append(at, size);
	return 0;
}

int	HttpParser::onStatus(const char* at, size_t size){
	mMessage->mStatus.append(at, size);
	return 0;
}

int	HttpParser::onHeaderField(const char* at, size_t size){
	mHeaderValue = NULL;
	mHeaderField.append(at,size);
	return 0;
}

int	HttpParser::onHeaderValue(const char* at, size_t size){
	if(mHeaderValue == NULL ) {
		std::pair<HttpMessage::HEADERS::iterator,bool> ret = mMessage->mHeaders.insert(
			HttpMessage::HEADERS::value_type(mHeaderField, std::string(at,size)));
		if(!ret.second){
			mMessage->mHeaders.erase(mHeaderField);
			ret = mMessage->mHeaders.insert(
				HttpMessage::HEADERS::value_type(mHeaderField, std::string(at,size)));
			assert(ret.second);
		}
		mHeaderValue = &ret.first->second;
		mHeaderField.clear();
	} else {
		mHeaderValue->append(at, size);
	}
	return 0;
}

int	HttpParser::onBody(const char* at, size_t size){
	assert(mCallback!=NULL);
	if(!mCallback->onHttpBody(at, size))
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
}
