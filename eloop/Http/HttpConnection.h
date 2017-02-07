#ifndef __HTTP_PROCESSOR_h__
#define __HTTP_PROCESSOR_h__

#include "httpMessage.h"
#include "eloop_net.h"
#include <set>
#include <assert.h>
// #include "http_parser.h"

struct http_parser;
struct http_parser_settings;

using namespace ZQ::eloop;



// ---------------------------------------
// class HttpConnection
// ---------------------------------------
class HttpConnection: public TCP, public ParserCallback
{
public:
	enum ParseState {
		STATE_INIT,
		STATE_HEADERS,
		STATE_BODY,
		STATE_COMPLETE
	};


public:
	virtual ~HttpConnection();

	int send(const char* buf,size_t len);
	void setkeepAlive(bool alive){_bOutgoingKeepAlive = alive;}
	bool getkeepAlive(){ return _bOutgoingKeepAlive;}

protected:
	HttpConnection(bool clientSide);
	void			reset( ParserCallback* callback = NULL);
	int getSendCount(){return _SendCount;}
	
//	int 			sendChunk( BufferHelper& bh);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ElpeError status);

private:
	HttpConnection( const HttpConnection&);
	HttpConnection& operator=( const HttpConnection&);

protected:
	virtual void onHttpError(int err) {}
	// onReqMsgSent is only used to notify that the sending buffer is free and not held by HttpClient any mre
	virtual void onHttpDataSent(bool keepAlive) { }
	// onHttpDataReceived is only used to notify that the receiving buffer is free and not held by HttpClient any mre
	virtual void onHttpDataReceived(size_t size) { }

private:

	http_parser*             _Parser; // its type can be determined by clientSide
	http_parser_settings*    _ParserSettings;
	HttpMessage::Ptr         _CurrentParseMsg;
	HttpMessage::MessgeType  _Type;
	ParserCallback*			 _Callback;
	ParseState			_ParserState;

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
	size_t HttpConnection::parse( const char* data, size_t size);
	static int on_message_begin(http_parser* parser);
	static int on_headers_complete(http_parser* parser);
	static int on_message_complete(http_parser* parser);
	static int on_uri(http_parser* parser,const char* at,size_t size);
	static int on_status(http_parser* parser, const char* at, size_t size);
	static int on_header_field(http_parser* parser, const char* at, size_t size);
	static int on_header_value(http_parser* parser, const char* at, size_t size);
	static int on_body(http_parser* parser, const char* at, size_t size);

	virtual int		onMessageBegin( );
	virtual int		onHeadersComplete();
	virtual int		onMessageComplete();
	virtual int		onUri(const char* at, size_t size);
	virtual int		onStatus(const char* at, size_t size);
	virtual int		onHeaderField(const char* at, size_t size);
	virtual int		onHeaderValue(const char* at, size_t size);
	virtual int		onBody(const char* at, size_t size);

};

#endif