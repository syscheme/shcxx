#ifndef __HTTP_Connection_H__
#define __HTTP_Connection_H__

#include "Pointer.h"
#include "Locks.h"

#include "TCPServer.h"

#include <queue>
#include <sstream>
#include <string>

#include "http_parser.h" // TO remove

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

struct http_parser;
struct http_parser_settings;

namespace ZQ {
namespace eloop {

#define CONTENT_LEN_CHUNKED   (-100)

// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
class HttpMessage : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<HttpMessage> Ptr;
	typedef std::map<std::string, std::string> Properties;
	typedef Properties Headers;

	typedef enum _MessgeType { 
		MSG_REQUEST  = HTTP_REQUEST, 
		MSG_RESPONSE = HTTP_RESPONSE, 
		MSG_BOTH     = HTTP_BOTH 
	} MessgeType;

	typedef enum _HttpMethod
	{
		HTTPDELETE	= HTTP_DELETE,
		GET			= HTTP_GET,
		POST		= HTTP_POST,
		PUT			= HTTP_PUT,
		OPTIONS		= HTTP_OPTIONS
	} HttpMethod;

	typedef enum _MessagingPhase {
		hmpNil, // not yet messasging
		hmpStarted, // messaging started
		hmpHeaders, // sending/receiving headers
		hmpBody, // sending/receiving body
		hmpCompleted // messaging completed
	} MessagingPhase;

	enum Encoding
	{
		heNone, // raw data
		heFormUrlEncoded,
		heFormMultiparts
	};

	typedef enum _StatusCodeEx
	{
		scContinue                       =100,
		scSwitchingProtocols             =101,
		scOK                             =200,
		scCreated                        =201,
		scAccepted                       =202,
		scNonAuthoritativeInformation    =203,
		scNoContent                      =204,
		scResetContent                   =205,
		scPartialContent                 =206,
		scMultipleChoices                =300,
		scMovedPermanently               =301,
		scFound                          =302,
		scSeeOther                       =303,
		scNotModified                    =304,
		scUseProxy                       =305,
		scTemporaryRedirect              =307,
		scBadRequest                     =400,
		scUnauthorized                   =401,
		scPaymentRequired                =402,
		scForbidden                      =403,
		scNotFound                       =404,
		scMethodNotAllowed               =405,
		scNotAcceptable                  =406,
		scProxyAuthenticationRequired    =407,
		scRequestTimeout                 =408,
		scConflict                       =409,
		scGone                           =410,
		scLengthRequired                 =411,
		scPreconditionFailed             =412,
		scRequestEntityTooLarge          =413,
		scRequestURITooLong              =414,
		scUnsupportedMediaType           =415,
		scRequestedRangeNotSatisfiable   =416,
		scExpectationFailed              =417,
		scInternalServerError            =500,
		scNotImplemented                 =501,
		scBadGateway                     =502,
		scServiceUnavailable             =503,
		scGatewayTimeout                 =504,
		scHTTPVersionNotSupported        =505,

		errNullBundary    = 1000,
		errBodyDecode     = 1001,
		errMIMEPart       = 1002,
		errBodyEnd        = 1003,
		errHTTPParse      = 1004,
		errUnknown        = 2000,

		// exec error code
		errBackendProcess   = -100,
		errConnectionLost   = -101,
		errInvalidParams    = -102,
		errSendFail         = -103,
		errBindFail         = -104,
		errConnectFail      = -105,
		errConnectTimeout   = -106,
		errSendConflict     = -107,
		errBadData          = -108,

		// async handling
		errAsyncInProgress  = -200,
	} StatusCodeEx;

#define HTTP_STATUSCODE(ErrCode) (ErrCode>=100 && ErrCode <=999)

#define HTTP_SUCC(ErrCode) (ErrCode>=200 && ErrCode < 300)
#define HTTP_PROCESS_SUCC(ErrCode) (HTTP_SUCC(ErrCode) || errAsyncInProgress == ErrCode)

public:
	HttpMessage(MessgeType type, const std::string& connId="");
	virtual ~HttpMessage();

	static const char* method2str(HttpMethod method);
	static const char* code2status(int statusCode);
	static std::string httpdate(int secsInFuture = 0);
	static std::string uint2hex(unsigned long u, size_t alignLen = 0, char paddingChar = '0');
	static bool chopURI(const std::string& uristr, std::string& host, int& port, std::string& uri, std::string& qstr);

	HttpMethod  method() const { return _method; }
	// const std::string& url() const { return _url; }
	int		statusCode() const { return _statusCode; }
	MessagingPhase phase() const { return _phase; }
	std::string getConnId() const { return _connId; }
	
	//@return the URI of message according to the status line
	virtual std::string uri();

	const Headers headers() const { return _headers; }
	const std::string& header( const std::string& key) const;
	template<typename T>
	void	header(const std::string& key, const T& value){
		std::ostringstream oss;
		oss<<value;
		ZQ::common::MutexGuard gd(_locker);

		_headers[key] = oss.str();
	}
	int     elapsed() const;

	void	eraseHeader( const std::string& key );

	bool	keepAlive() const;	//check if keepAlive is set
	bool	chunked() const;    //check if chunked is set

	int64	contentLength() const; //get Content-Length, 0 if chunked is enabled
	bool	hasContentBody( ) const { return chunked() || contentLength() > 0; }

	// std::string toRaw(); // generate raw http message, only start line and headers are generated

	void getHTTPVersion(uint& major, uint& minor) const { major = _httpVMajor; minor = _httpVMinor; }
	// void setHTTPVersion(uint major, uint minor) { _httpVMajor = major;	_httpVMinor = minor; }

protected:
	friend class HttpConnection;
	friend class HttpPassiveConn;
	friend class NestedParser;

	ZQ::common::Mutex	_locker;
	std::string         _connId;
	MessgeType          _type; //request or response
	HttpMethod			_method;
	std::string			_uri;
	std::string         _qstr;
	Headers				_headers;
	std::string         _simpleBody;
	int64				_stampCreated;
	int64				_declaredBodyLength; // for the incoming message, only valid if _bChunked == false

	MessagingPhase		_phase;
	unsigned int		_httpVMajor;
	unsigned int		_httpVMinor;

	int					_statusCode;//status code
	std::string         _status;
	std::string			_boundary; // for multipart/form-data only

	Encoding			_encoding;

	// std::string			_bodyBuf;
	uint32              _flags; // combination of MsgFlag
};

// ---------------------------------------
// class HttpConnection
// ---------------------------------------
// extends TCPConnection with a HTTP parser and generator
// Note: this HttpConnection doesn't support HTTP pipeline
class HttpConnection: public TCPConnection // , public IHttpReceive
{
public:
	class PayloadChunk: public ZQ::common::SharedObject, protected std::string 
	{
	public:
		typedef ZQ::common::Pointer <PayloadChunk> Ptr;
		typedef ZQ::common::Pointer <Ptr> List;

		PayloadChunk(const uint8* data, size_t len) : std::string((const char*)data, len) {}
		const uint8* data() const { return (const uint8*) std::string::c_str(); }
		size_t len() const { return std::string::size(); }
	};

private: // forbid copier constructor
	HttpConnection(const HttpConnection&);
	HttpConnection& operator=( const HttpConnection&);

protected:
	HttpConnection(ZQ::common::Log& logger, const char* connId = NULL, TCPServer* tcpServer = NULL);
	
protected: // impl of TCPConnection, no more overwriteable
	void OnConnected(ElpeError status);
	void OnRead(ssize_t nread, const char *buf);
	void OnWrote(int status);
	/// error occured on TCP connection
	void OnConnectionError(int error, const char* errorDescription);
	
	virtual void OnTimer() {}

public:
	virtual ~HttpConnection();

	void resetReceiving(); // IHttpReceive* callback = NULL);

	static std::string formatMsgHeaders(HttpMessage::Ptr msg);
	static std::string formatStartLine(HttpMessage::Ptr msg);

	HttpMessage::MessagingPhase sendingPhase() const;
	HttpMessage::MessagingPhase receivingPhase() const;

	PayloadChunk::Ptr pushOutgoingPayload(const void* data, size_t len, bool bEnd =false);
	PayloadChunk::Ptr popReceivedPayload();

	HttpMessage::StatusCodeEx sendMessage(HttpMessage::Ptr msg);

protected: // new overwriteable entry points 
	// about message receiving triggered by HTTP parser
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnHeadersReceived(const HttpMessage::Ptr msg) { return HttpMessage::errAsyncInProgress; }
	
	//@return expect errAsyncInProgress to continue receiving 
	virtual HttpMessage::StatusCodeEx OnBodyPayloadReceived(const char* data, size_t size);

	virtual void OnMessageReceived(const HttpMessage::Ptr msg) {}
	virtual void OnMessagingError(int error, const char* errorDescription ) {}

	// about message sending
	virtual void OnBodyPayloadSubmitted(size_t bytesPushed, uint64 offsetBodyPayload) {}
	virtual void OnMessageSubmitted(HttpMessage::Ptr msg) {} 

protected:
	bool sendByPhase(); // triggered by OnWrote/Connected()
	virtual void poll(); // triggered by the management party periodically, such as HttpServer as the server-side or HttpUserAgent as the client-side

protected: // callbacks from HTTP parser
	virtual int	onParser_MessageBegin() { return 0; }
	virtual int	onParser_HeadersCompleted();
	virtual int	onParser_MessageComplete();
	virtual int	onParser_Url(const char* at, size_t size);
	virtual int	onParser_Status(const char* at, size_t size);
	virtual int	onParser_HeaderField(const char* at, size_t size);
	virtual int	onParser_HeaderValue(const char* at, size_t size);
	virtual int	onParser_Body(const char* at, size_t size);

	// in chunked mode, the call back seq will be onParser_ChunkStart()->onParser_Body()->onParser_ChunkCompleted()->onParser_ChunkStart()->onParser_Body()->onParser_ChunkCompleted()...
	virtual int	onParser_ChunkStart(size_t chunkSize) { return 0; }
	virtual int	onParser_ChunkCompleted(size_t chunkSize) { return 0; }

protected:
	// ZQ::common::Mutex	    _locker; // to protect the _payloadOutgoing/_payloadReceived data
	typedef std::queue< PayloadChunk::Ptr > Payload;

	HttpMessage::Ptr        _msgOutgoing;
	Payload				    _payloadOutgoing;
	int64				    _offsetBodyPlayloadSent;
	bool                    _bOutgoingOpen;

	Payload				    _payloadReceived; // its message part is saved in _nestedParser
	int64					_stampLastRecv;
	// HttpMessage::StatusCodeEx _lastRecvError;

private:
	friend class NestedParser;
	void*  _nestedParser;
	// IHttpReceive*  _extParseSink;
	// HttpMessage::StatusCodeEx   _lastRecvError;

private:
	static int _parserCb_message_begin(void* ctx);
	static int _parserCb_headers_complete(void* ctx);
	static int _parserCb_message_complete(void* ctx);
	static int _parserCb_uri(void* ctx,const char* at,size_t size);
	static int _parserCb_status(void* ctx, const char* at, size_t size);
	static int _parserCb_header_field(void* ctx, const char* at, size_t size);
	static int _parserCb_header_value(void* ctx, const char* at, size_t size);
	static int _parserCb_body(void* ctx, const char* at, size_t size);
	static int _parserCb_chunk_header(void* ctx);
	static int _parserCb_chunk_complete(void* ctx);
};
} }//namespace ZQ::eloop

#endif