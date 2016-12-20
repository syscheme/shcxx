#ifndef __HTTP_PARSER_h__
#define __HTTP_PARSER_h__

#include "httpMessage.h"
#include <string>
#include "http_parser.h"



typedef enum _ErrorCode
{
	ERR_ERROR			= -1,	//generic error
	ERR_EOF				= -2,	//end of stream, we also take CONNECTION_RESET as EOF due to lot of tcp application will do close a connection brutally
	ERR_INVAID			= -3,	//invalid parameter
	ERR_ADDRINUSE		= -4,	//address can not be bound
	ERR_ADDRNOTAVAIL	= -5,	//interface is not found
	ERR_TIMEOUT			= -6,	//timeout in operation
	ERR_CONNREFUSED		= -7,	//connection refused
	ERR_RECVFAIL    = -8, //revc return false
	ERR_SENDFAIL    = -9, //send return false
	ERR_EAGAIN      = -10,
	ERR_SOCKETVAIN = -11, // the socket can not been send/recv
	ERR_MEMVAIN = -12,   // the mem malloc err 
	ERR_BUFFERTOOBIG = -13,
	ERR_UDPSOCKETNOTALIVE = -14,
	//linux
	ERR_EPOLLREGISTERFAIL  = -21,
	ERR_EPOLLEXCEPTION = - 22,
	ERR_EOF2           = -23,
	ERR_EOF3           = -24
}ErrorCode;

const char* ErrorCodeToStr( ErrorCode c );

// ---------------------------------------
// interface ParserCallback
// ---------------------------------------
/*
* Inhereit ParserCallback so that you can get the result of http message parsing
*/
class ParserCallback{
public:
	virtual ~ParserCallback(){}

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
		mStopped = true;
		return true;
	}

	/// check if headers are all parsed
	bool	headerComplete() const { 
		return mParserState > STATE_HEADERS;
	}
	bool	httpComplete() const {
		return mParserState >= STATE_COMPLETE;
	}
	ParserState		state() const {
		return mParserState;
	}

	int		lastError() const;

	HttpMessage::Ptr	currentHttpMessage() {
		return mMessage;
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
	http_parser			mParser;
	http_parser_type	mType;
	HttpMessage::Ptr	mMessage;
	bool				mStopped;
	ParserCallback*		mCallback;

	std::string			mHeaderField;
	std::string*		mHeaderValue;
	http_parser_settings	mParserSettings;
	ParserState			mParserState;

};


#endif