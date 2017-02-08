#ifndef __HTTP_Connection_H__
#define __HTTP_Connection_H__

#include "httpMessage.h"
#include "eloop_net.h"
#include <set>
#include <assert.h>
// #include "http_parser.h"

struct http_parser;
struct http_parser_settings;

namespace ZQ {
namespace eloop {

// ---------------------------------------
// interface IHttpParseSink
// ---------------------------------------
// Inherit IHttpParseSink so that you involves the stages of http message parsing
class IHttpParseSink
{
public:
	/// this method is invoked when http header is completely parsed
	/// You can get uri, method or code, header field and header value in HttpMessage
	/// return true if you don't want to interrupt the parsing procedure
	virtual bool onHeadersEnd( const HttpMessage::Ptr msg) = 0;

	/// this method is invoked when http body data is received and decoded
	/// NOTE: this method may be called many times for one data buffer,
	///		  So don' take this as the finish of the usage of you buffer passed into HttpClient
	/// return true if you don't want to interrupt the parsing procedure
	virtual bool onBodyData( const char* data, size_t size) = 0;

	/// the whole http message is decoded, that is no more data for current http message
	virtual void onMessageCompleted() = 0;

	/// error occured during data receiving or parsing stage
	virtual void onParseError( int error ) = 0;
};

// ---------------------------------------
// class HttpConnection
// ---------------------------------------
class HttpConnection: public TCP, public IHttpParseSink
{
private:
	HttpConnection(const HttpConnection&);
	HttpConnection& operator=( const HttpConnection&);

public:
	virtual ~HttpConnection();

	int send(const char* buf,size_t len);
	void setkeepAlive(bool alive){_bOutgoingKeepAlive = alive;}
	bool getkeepAlive(){ return _bOutgoingKeepAlive;}

protected:
	HttpConnection(bool clientSide);
	void			reset( IHttpParseSink* callback = NULL);
	int getSendCount(){return _SendCount;}
	
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);


protected:
	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void onHttpDataSent(bool keepAlive) { }
	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void onHttpDataReceived(size_t size) { }

protected: // implementation of IHttpParseSink that also present the message receiving thru the connection

	/// this method is invoked when all http headers are completely parsed
	/// You can get uri, method or code, header field and header value in HttpMessage then
	///@return true if you don't want to interrupt the parsing procedure
	virtual bool onHeadersEnd( const HttpMessage::Ptr msg) { return true; }

	/// this method is invoked when a piecee of http body data is received and decoded
	/// NOTE: this method may be called several times for one data buffer,
	///		  So don' take this as the finish of the usage of you buffer passed into HttpClient
	///@return true if you don't want to interrupt the parsing procedure
	virtual bool onBodyData( const char* data, size_t size) { return true; }

	/// the whole http message is decoded, that is no more data for current http message
	virtual void onMessageCompleted() {}

	/// error occured during data receiving or parsing stage
	virtual void onParseError( int error ) {}

private:

	http_parser*             _Parser; // its type can be determined by clientSide
	http_parser_settings*    _ParserSettings;
	HttpMessage::Ptr         _CurrentParseMsg;
	HttpMessage::MessgeType  _Type;
	IHttpParseSink*			 _Callback;

	bool				_Stopped;

	std::string			_HeaderField;
	std::string*		_HeaderValue;
//	http_parser_type		_ParseType;
//	HttpParser				_HttpParser;

	HttpMessage::Ptr		_IncomingMsg;

	int						_SendCount;
	bool					_bOutgoingKeepAlive;

// from previouw HttpParser
private:
	enum ParseState {
		STATE_INIT,
		STATE_HEADERS,
		STATE_BODY,
		STATE_COMPLETE
	};
	ParseState			_ParserState;
	size_t parse( const char* data, size_t size);
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

};
} }//namespace ZQ::eloop

#endif