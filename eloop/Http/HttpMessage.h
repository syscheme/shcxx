#ifndef __HTTP_MESSAGE_h__
#define __HTTP_MESSAGE_h__

#include <Locks.h>
#include <sstream>
#include <string>
#include <Pointer.h>
#include <map>
#include "eloop_lock.h"
#include "http_parser.h"

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

static const char* httpDateStrWeekDay[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* httpDateStrMonth[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
class HttpMessage : public ZQ::common::SharedObject{
public:
	typedef ZQ::common::Pointer<HttpMessage> Ptr;
	typedef enum _MessgeType { HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH } MessgeType;

public:
	HttpMessage(MessgeType type);
	virtual ~HttpMessage();


	static const std::string& code2status(int code);
	static std::string httpdate( int deltaInSecond = 0 );

	http_method method() const;
	void		method(http_method mtd);

	const std::string& url() const;
	void		url(const std::string& url);

	void	code( int c);	//set status code
	int		code() const;	//get status cde

	void	status( const std::string& s);	// set status description
	const std::string& status() const;		//get status description

	const std::string& header( const std::string& key) const;

	template<typename T>
	void	header( const std::string& key, const T& value){
		std::ostringstream oss;
		oss<<value;
		ZQ::common::MutexGuard gd(_Locker);

		_Headers[key] = oss.str();
	}
	void	eraseHeader( const std::string& key );

	bool	keepAlive() const;	//check if keepAlive is set
	void	keepAlive( bool b);	//enable/disable keepAlive

	bool	chunked() const;	//check if chunked is set
	void	chunked(bool b);	//enable/disable chunked transfer-encoding,
	//enable chunked also disable Content-Length

	int64	contentLength() const;	//get Content-Length, 0 if chunked is enabled
	void	contentLength(int64 length); //set content-length, this behaviour supress chunked

	bool	hasContentBody( ) const {
		return chunked() || contentLength() > 0;
	}

	std::string toRaw(); // generate raw http message, only start line and headers are generated

	struct caseInsensativeCmp{
		bool operator()( const std::string& lhr, const std::string& rhs) const {
#ifdef ZQ_OS_LINUX
#	define stricmp strcasecmp
#endif
			return stricmp(lhr.c_str(),rhs.c_str()) < 0;
		}
	};
	typedef std::map<std::string,std::string,caseInsensativeCmp> HEADERS;

	inline unsigned int versionMajor() const { return _VerMajor; }
	inline unsigned int versionMinor() const { return _VerMinor; }

	void setVersion( unsigned int major, unsigned int minor) {
		_VerMajor = major;
		_VerMinor = minor;
	}

private:
	friend class HttpConnection;

	ZQ::common::Mutex	_Locker;
	http_parser_type	_Type;//request or response
	http_method			_Method;
	std::string			_Uri;
	std::string			_Status;
	int					_Code;//status code
	bool				_bChunked;
	bool				_bKeepAlive;

	unsigned int		_VerMajor;
	unsigned int		_VerMinor;
	HEADERS				_Headers;
	std::string			_DummyHeaderValue;
	int64				_BodyLength;//only valid if _bChunked == false
	std::string 		_RawMessage;

};


// ---------------------------------------
// interface ParserCallback
// ---------------------------------------
/*
* Inherit ParserCallback so that you can get the result of http message parsing
*/
class ParserCallback
{
public:
	virtual ~ParserCallback() {}

	/// this method is invoked when http header is completely parsed
	/// You can get uri, method or code, header field and header value in HttpMessage
	/// return true if you don't want to interrupt the parsing procedure
	virtual bool onHttpMessage( const HttpMessage::Ptr msg) = 0;

	/// this method is invoked when http body data is received and decoded
	/// NOTE: this method may be called many times for one data buffer,
	///		  So don' take this as the finish of the usage of you buffer passed into HttpClient
	/// return true if you don't want to interrupt the parsing procedure
	virtual bool onHttpBody( const char* data, size_t size) = 0;

	/// the whole http message is decoded, that is no more data for current http message
	virtual void onHttpComplete() = 0;

	/// error occured during data receiving or parsing stage
	virtual void onHttpError( int error ) = 0;
};

/*
// ---------------------------------------
// class HttpParser
// ---------------------------------------
class HttpParser
{
public:
	HttpParser(http_parser_type type);
	~HttpParser();

	enum ParserState {
		STATE_INIT,
		STATE_HEADERS,
		STATE_BODY,
		STATE_COMPLETE
	};


	/// reset parser to initialized stage
	void	reset( ParserCallback* cb );

	///return value == size means successfully parsed,
	/// or else, error occurred
	size_t	parse( const char* data, size_t size);

	/// stop current parsing procedure
	bool	stopParsing() {
		_Stopped = true;
		return true;
	}

	/// check if headers are all parsed
	bool	headerComplete() const { 
		return _ParserState > STATE_HEADERS;
	}
	bool	httpComplete() const {
		return _ParserState >= STATE_COMPLETE;
	}
	ParserState		state() const {
		return _ParserState;
	}

	int		lastError() const;

	HttpMessage::Ptr	currentHttpMessage() {
		return _Message;
	}


private:
	static int on_message_begin(http_parser* parser);
	static int on_headers_complete(http_parser* parser);
	static int on_message_complete(http_parser* parser);
	static int on_uri(http_parser* parser,const char* at,size_t size);
	static int on_status(http_parser* parser, const char* at, size_t size);
	static int on_header_field(http_parser* parser, const char* at, size_t size);
	static int on_header_value(http_parser* parser, const char* at, size_t size);
	static int on_body(http_parser* parser, const char* at, size_t size);

	int		onMessageBegin( );
	int		onHeadersComplete();
	int		onMessageComplete();
	int		onUri(const char* at, size_t size);
	int		onStatus(const char* at, size_t size);
	int		onHeaderField(const char* at, size_t size);
	int		onHeaderValue(const char* at, size_t size);
	int		onBody(const char* at, size_t size);

private:
	http_parser			_Parser;
	http_parser_type	_Type;
	HttpMessage::Ptr	_Message;
	bool				_Stopped;
	ParserCallback*		_Callback;

	std::string			_HeaderField;
	std::string*		_HeaderValue;
	http_parser_settings	_ParserSettings;
	ParserState			_ParserState;
};*/








#endif