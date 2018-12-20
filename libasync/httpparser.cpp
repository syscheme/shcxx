#include <ZQ_common_conf.h>
#include <time.h>
#include <assert.h>
#include "httpparser.h"

namespace LibAsync{

	HttpParser::HttpParser(http_parser_type type)
	:mType(type),
	mStopped(false),
	mHeaderValue(NULL){
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

	HttpParser::~HttpParser(){

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

	int HttpParser::on_uri(http_parser* parser, const char* at, size_t size){
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


	struct HttpCode2Str {
		int 			code;
		const char*		status;
	};
	static struct HttpCode2Str httpc2s[] = {
		{100,"Continue"},
		{101,"Switching Protocols"},
		{200,"OK"},
		{201,"Created"},
		{202,"Accepted"},
		{203,"Non-Authoritative Information"},
		{204,"No Content"},
		{205,"Reset Content"},
		{206,"Partial Content"},
		{300,"Multiple Choices"},
		{301,"Moved Permanently"},
		{302,"Found"},
		{303,"See Other"},
		{304,"Not Modified"},
		{305,"Use Proxy"},
		{307,"Temporary Redirect"},
		{400,"Bad Request"},
		{401,"Unauthorized"},
		{402,"Payment Required"},
		{403,"Forbidden"},
		{404,"Not Found"},
		{405,"Method Not Allowed"},
		{406,"Not Acceptable"},
		{407,"Proxy Authentication Required"},
		{408,"Request Timeout"},
		{409,"Conflict"},
		{410,"Gone"},
		{411,"Length Required"},
		{412,"Precondition Failed"},
		{413,"Request Entity Too Large"},
		{414,"Request-URI Too Long"},
		{415,"Unsupported Media Type"},
		{416,"Requested Range Not Satisfiable"},
		{417,"Expectation Failed"},
		{500,"Internal Server Error"},
		{501,"Not Implemented"},
		{502,"Bad Gateway"},
		{503,"Service Unavailable"},
		{504,"Gateway Timeout"},
		{505,"HTTP Version Not Supported"},
		};

	static std::map<int, std::string> code2statusmap;
	static std::string 				unknownstatus = "unkown";

	class Code2StatusMapInitializer{
	public:
		Code2StatusMapInitializer() {
			size_t count = sizeof( httpc2s ) / sizeof(httpc2s[0]);
			for( size_t i = 0 ; i < count; i ++ ) {
				code2statusmap[ httpc2s[i].code ]  = httpc2s[i].status;
			}
		}
	};
	
	Code2StatusMapInitializer c2smapinitializer;

	static const char* httpDateStrWeekDay[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	static const char* httpDateStrMonth[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


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
	//////////////HttpMessage
	HttpMessage::HttpMessage(http_parser_type type)
		:mType(type),
		mMethod(HTTP_GET),
		mCode(0),
		mbChunked(false),
		mbKeepAlive(false),
		mVerMajor(0),
		mVerMinor(0),
		mBodyLength(0){

	}

	HttpMessage::~HttpMessage(){

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
 
	std::string HttpMessage::toRaw() {
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

}//namespae LibAsync
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312
