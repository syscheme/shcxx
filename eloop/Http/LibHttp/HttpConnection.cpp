#include "HttpConnection.h"
#include "TimeUtil.h"
#include "urlstr.h"
#include "http_parser.h"
#include <assert.h>

#include <functional>
#include <boost/algorithm/string/trim.hpp>

#define EOL "\r\n"
#define MAX_BYTES_BODY_SEND (1024*16)

namespace ZQ {
namespace eloop {

// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
HttpMessage::HttpMessage(MessgeType type, const std::string& connId)
: _type(type), _method(GET), _statusCode(0), _flags(0), _stampCreated(ZQ::common::now()), _declaredBodyLength(0), _connId(connId),
_httpVMajor(1), _httpVMinor(0), _phase(hmpNil)
{
}

HttpMessage::~HttpMessage()
{
}

int HttpMessage::elapsed() const
{
	return (int) (ZQ::common::now() - _stampCreated); 
}

const char* HttpMessage::method2str(HttpMethod method)
{
	return http_method_str((http_method)method);
}

const char* HttpMessage::code2status(int statusCode)
{
	switch(statusCode)
	{
	case scContinue                       : return "Continue";
	case scSwitchingProtocols             : return "Switching Protocols";
	case scOK                             : return "OK";
	case scCreated                        : return "Created";
	case scAccepted                       : return "Accepted";
	case scNonAuthoritativeInformation    : return "Non-Authoritative Information";
	case scNoContent                      : return "No Content";
	case scResetContent                   : return "Reset Content";
	case scPartialContent                 : return "Partial Content";
	case scMultipleChoices                : return "Multiple Choices";
	case scMovedPermanently               : return "Moved Permanently";
	case scFound                          : return "Found";
	case scSeeOther                       : return "See Other";
	case scNotModified                    : return "Not Modified";
	case scUseProxy                       : return "Use Proxy";
	case scTemporaryRedirect              : return "Temporary Redirect";
	case scBadRequest                     : return "Bad Request";
	case scUnauthorized                   : return "Unauthorized";
	case scPaymentRequired                : return "Payment Required";
	case scForbidden                      : return "Forbidden";
	case scNotFound                       : return "Not Found";
	case scMethodNotAllowed               : return "Method Not Allowed";
	case scNotAcceptable                  : return "Not Acceptable";
	case scProxyAuthenticationRequired    : return "Proxy Authentication Required";
	case scRequestTimeout                 : return "Request Timeout";
	case scConflict                       : return "Conflict";
	case scGone                           : return "Gone";
	case scLengthRequired                 : return "Length Required";
	case scPreconditionFailed             : return "Precondition Failed";
	case scRequestEntityTooLarge          : return "Request Entity Too Large";
	case scRequestURITooLong              : return "Request-URI Too Long";
	case scUnsupportedMediaType           : return "Unsupported Media Type";
	case scRequestedRangeNotSatisfiable   : return "Requested Range Not Satisfiable";
	case scExpectationFailed              : return "Expectation Failed";
	case scInternalServerError            : return "Internal Server Error";
	case scNotImplemented                 : return "Not Implemented";
	case scBadGateway                     : return "Bad Gateway";
	case scServiceUnavailable             : return "Service Unavailable";
	case scGatewayTimeout                 : return "Gateway Timeout";
	case scHTTPVersionNotSupported        : return "HTTP Version Not Supported";
	}

	switch(statusCode)
	{
	case errNullBundary:
		return "boundary is null";
	case errBodyDecode:
		return "Bad post data that can't be decoded. encoding=application/x-www-form-urlencoded";
	case errMIMEPart:
		return "Bad format of post data. Can't position the begin of the body part. encoding=multipart/form-data";
	case errBodyEnd:
		return "Bad format of post data. Can't find the body end.";
	case errUnknown:
	default:
		break;
	}

	return "errUnknown";
}

static const char* httpDateStrWeekDay[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* httpDateStrMonth[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

#define CRLF "\r\n"
#define ChunkTail "0\r\n\r\n"

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

bool HttpMessage::chopURI(const std::string& uristr, std::string& host, int& port, std::string& uri, std::string& qstr)
{
	host = qstr ="";
	port =-1;
	size_t pos = uristr.find_first_of("?&");
	if (std::string::npos != pos)
	{
		qstr = uristr.substr(pos+1);
		uri = uristr.substr(0, pos);
	}
	else uri = uristr;

	pos = uri.find_first_of(":");
	if (std::string::npos != pos)
	{
		// sounds like a full URL with leading proto://
		ZQ::common::URLStr parser(uri.c_str());
		host = parser.getHost();
		port = parser.getPort();
		uri  = parser.getPath();
	}

	return true;
}

std::string HttpMessage::uri()
{
	if (_qstr.length() <= 0 )
		return _uri;

	return _uri +"?" + _qstr;
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
static std::string NIL;
const std::string& HttpMessage::header( const std::string& key) const
{
	ZQ::common::MutexGuard gd(_locker);
	Headers::const_iterator it = _headers.find(key);
	if( it == _headers.end())
		return NIL;
	return it->second;
}

void HttpMessage::eraseHeader( const std::string& key ) {
	ZQ::common::MutexGuard gd(_locker);
	_headers.erase( key );
}

int64 HttpMessage::contentLength() const
{
	if (chunked())
		return 0;

	return _declaredBodyLength;
}	

bool HttpMessage::keepAlive() const
{
	return (0 != (_flags & F_CONNECTION_KEEP_ALIVE));
}	//check if keepAlive is set

bool HttpMessage::chunked() const
{ 
	return (0 != (_flags & F_CHUNKED));
}	//check if chunked is set

// ---------------------------------------
// class NestedParser
// ---------------------------------------
class NestedParser
{
public:
	http_parser parser;
	http_parser_settings settings;
	HttpMessage::Ptr    msg;
	std::string			headerName, headerValue;
	HttpConnection&     _conn;
	bool  _passive;
	std::string         url;

	NestedParser(HttpConnection& conn, bool passive) : _conn(conn), _passive(passive)
	{
		memset(&settings, 0x00, sizeof(settings));
		parser.data = reinterpret_cast<void*>(this);

		settings.on_body				= _parserCb_body;
		settings.on_header_field		= _parserCb_header_field;
		settings.on_header_value		= _parserCb_header_value;
		settings.on_headers_complete	= _parserCb_headers_complete;
		settings.on_message_begin		= _parserCb_message_begin;
		settings.on_message_complete	= _parserCb_message_complete;
		settings.on_status				= _parserCb_status;
		settings.on_url					= _parserCb_uri;
		settings.on_chunk_header		= _parserCb_chunk_header;  
		settings.on_chunk_complete		= _parserCb_chunk_complete;
	}

	void reset()
	{
		http_parser_init(&parser, (_passive ? HTTP_REQUEST : HTTP_RESPONSE));
		headerName.clear();
		headerValue = "";

		msg = new HttpMessage(_passive ? HttpMessage::MSG_REQUEST : HttpMessage::MSG_RESPONSE, _conn.connId());
		if(msg)
			msg->_phase = HttpMessage::hmpStarted;
	}

	void parse(const char* data, size_t size)
	{
		//if (_outgoingPhase != hmpCompleted)
		//{
		//	AsyncBuf::Ptr inflowPtr = new AsyncBuf(data,size);
		//	_listpipe.push_back(inflowPtr);
		//	return;
		//}

		if (!msg || msg->_phase >= HttpMessage::hmpCompleted)
			reset();

		size_t nparsed = http_parser_execute(&parser, &settings, data, size);

		if (nparsed == size)
			return;

		if (msg->_phase >= HttpMessage::hmpCompleted)
		{
			const char* tempbuf = data + nparsed;
			size_t templen = size - nparsed;
			parse(tempbuf, templen);
			return;
		}

		std::string parsedesc = "parse error:";
		parsedesc.append(http_errno_description((http_errno)parser.http_errno));
		_conn.OnMessagingError((int)parser.http_errno, parsedesc.c_str());
	}

private:
	static int _parserCb_message_begin(http_parser* parser);
	static int _parserCb_headers_complete(http_parser* parser);
	static int _parserCb_message_complete(http_parser* parser);
	static int _parserCb_uri(http_parser* parser,const char* at,size_t size);
	static int _parserCb_status(http_parser* parser, const char* at, size_t size);
	static int _parserCb_header_field(http_parser* parser, const char* at, size_t size);
	static int _parserCb_header_value(http_parser* parser, const char* at, size_t size);
	static int _parserCb_body(http_parser* parser, const char* at, size_t size);
	static int _parserCb_chunk_header(http_parser* parser);
	static int _parserCb_chunk_complete(http_parser* parser);
};

// ---------------------------------------
// string utils
// ---------------------------------------
static void split(std::vector<std::string>& v, const std::string& s, const std::string d = " ")
{
	v.clear();

	std::string::size_type pos_from = 0;
	while((pos_from = s.find_first_not_of(d, pos_from)) != std::string::npos)
	{
		std::string::size_type pos_to = s.find_first_of(d, pos_from);
		if(pos_to == std::string::npos)
		{
			v.push_back(s.substr(pos_from));
			break;
		}

		v.push_back(s.substr(pos_from, pos_to - pos_from));
		pos_from = pos_to;
	}
}

std::string trim(const std::string& s, const std::string& d = " ")
{
	return boost::trim_copy_if(s, boost::is_any_of(d));
}

// ---------------------------------------
// MIME Reader
// ---------------------------------------
class MIMEReader
{
public:
	struct Data
	{
		const char* data;
		size_t size;

		Data():data(NULL), size(0){}
		void clear() { data = NULL;	size = 0; }
	};

	struct SearchResult
	{
		Data released;
		Data prefix;
		Data locked;
		Data suffix;

		void clear() { released.clear(); prefix.clear(); locked.clear(); suffix.clear(); }
	};

public:
	MIMEReader() : _got(false) {	setTarget("\r\n"); }
	const char* getLine(const char* &data, size_t &len);
	void clear();

	// the search result won't include null pointer unless the input data is null.
	// return true for reach the target
	bool search(const char* data, size_t len, SearchResult& result);

	static void KnuthMorrisPratt(const std::vector<char>& x, std::vector<int>& kmpNext);

private:
	std::string _line;
	bool _got;
	std::string _target;

	std::vector<int> _kmpNext;
	size_t _nLocked;

	void setTarget(const std::string& target)
	{
		_nLocked = 0;
		_target = target;
		KnuthMorrisPratt(std::vector<char>(_target.begin(), _target.end()), _kmpNext);
	}
};

const char* MIMEReader::getLine(const char* &data, size_t &len)
{
	if(_got)
		return _line.c_str();

	SearchResult sr;
	_got = search(data, len, sr);
	_line.append(sr.released.data, sr.released.size);
	_line.append(sr.prefix.data, sr.prefix.size);
	data = sr.suffix.data;
	len = sr.suffix.size;
	return _got ? _line.c_str() : NULL;
}

void MIMEReader::clear()
{
	_line.clear();
	_got = false;
	_nLocked = 0;;
}

// implement the Knuth-Morris-Pratt algorithm
void MIMEReader::KnuthMorrisPratt(const std::vector<char>& x, std::vector<int>& kmpNext)
{
	size_t m = x.size();
	kmpNext.clear();
	kmpNext.resize(m + 1);

	size_t i = 0;
	int j = kmpNext[0] = -1;

	while(i < m)
	{
		while(j > -1 && x[i] != x[j])
			j = kmpNext[j];
		++i;
		++j;

		if(i < m && x[i] == x[j])
			kmpNext[i] = kmpNext[j];
		else
			kmpNext[i] = j;
	}
}

// the search result won't include null pointer unless the input data is null.
// return true for reach the target
bool MIMEReader::search(const char* data, size_t len, SearchResult& result)
{
	size_t m = _target.size();
	size_t i = _nLocked;
	size_t j = 0;
	// kmp algorithm
	while(j < len)
	{
		while(i > -1 && _target[i] != data[j])
			i = _kmpNext[i];

		++i;
		++j;
		if(i >= m) // found
			break;
	}

	// not found
	size_t vPos = _nLocked + j - i;
	if(vPos > (int)_nLocked)
	{ // all locked part released
		result.released.data = _target.data();
		result.released.size = _nLocked;
		result.prefix.data = data;
		result.prefix.size = j - i;
	}
	else
	{ // only first bytes of the locked part released
		result.released.data = _target.data();
		result.released.size = vPos;
		result.prefix.data = data;
		result.prefix.size = 0;
	}

	result.suffix.data = data + j;
	result.suffix.size = len - j;

	result.locked.data = _target.data();
	result.locked.size = i;

	if(i >= m) // found
	{ 
		_nLocked = 0; // discard the locked part
		return true;
	}

	// not found
	_nLocked = i;
	return false;
}

// ---------------------------------------
// class HttpConnection
// ---------------------------------------
#define SEND_GUARD()  ZQ::common::MutexGuard sg(_lkSend)
#define RECV_GUARD()  // ZQ::common::MutexGuard rg(_locker)

#define CONNFMT(_FUNC, _X) CLOGFMT(HttpConn, "%s " #_FUNC "() " _X), connId().c_str()

HttpConnection::HttpConnection(ZQ::common::Log& logger,const char* connId, TCPServer* tcpServer)
		: TCPConnection(logger, connId, tcpServer), _nestedParser(NULL), _bOutgoingOpen(true), _stampLastRecv(0) // , _lastRecvError(HttpMessage::scOK)
{
	RECV_GUARD();
	_nestedParser = new NestedParser(*this, isPassive());
	resetReceiving();
}

HttpConnection::~HttpConnection()
{
	if (_nestedParser)
		delete ((NestedParser*)_nestedParser);

	_nestedParser = NULL;
}

void HttpConnection::resetReceiving() // IHttpReceive* p)
{
	RECV_GUARD();
//	if(!p)
//		p = dynamic_cast<IHttpReceive*>(this);

	// _extParseSink = p;
	((NestedParser*)_nestedParser)->reset();
	_stampLastRecv = ZQ::common::now();
	// _lastRecvError = HttpMessage::scOK;
}

HttpMessage::MessagingPhase HttpConnection::sendingPhase() const
{
	SEND_GUARD();
	return _msgOutgoing ? _msgOutgoing->phase() : HttpMessage::hmpNil;
} 

HttpMessage::MessagingPhase HttpConnection::receivingPhase() const
{
	RECV_GUARD();
	return ((NestedParser*)_nestedParser)->msg ? ((NestedParser*)_nestedParser)->msg->phase() : HttpMessage::hmpNil;
} 

HttpMessage::StatusCodeEx HttpConnection::sendMessage(HttpMessage::Ptr msg)
{
	SEND_GUARD();
	if (_msgOutgoing)
		return HttpMessage::errSendConflict;

	_msgOutgoing = msg;
//Payload empty;
 //   std::swap(_payloadOutgoing, empty);
//	return (HttpMessage::StatusCodeEx)((_msgOutgoing) ? _msgOutgoing->statusCode() : HttpMessage::errSendConflict);
	if (!_msgOutgoing)
		return HttpMessage::scInternalServerError;

	sendByPhase();
	return (HttpMessage::StatusCodeEx)_msgOutgoing->statusCode();
}

#define MAX_CHUNK_PER_SEND (20)
#ifndef ULONG
#  define ULONG unsigned long
#endif
bool HttpConnection::sendByPhase()
{
	int64 payloadOffset = 0;
	int payloadBytesSent =0;

	HttpMessage::Ptr msgCompleted;

	{
		SEND_GUARD();
		if (!_msgOutgoing)
			return false; // nothing to do about sending request

		int ret = 0;
		payloadOffset = _offsetBodyPlayloadSent;
		switch (_msgOutgoing->phase())
		{
		case HttpMessage::hmpNil:
		case HttpMessage::hmpStarted:
			{
				std::string startLine = formatStartLine(_msgOutgoing) + formatMsgHeaders(_msgOutgoing);
				_logger.hexDump(ZQ::common::Log::L_DEBUG, startLine.c_str(), startLine.length(), "sendByPhase() headers:", true);
				ret = TCPConnection::_enqueueSend((const uint8*)startLine.c_str(), startLine.length());
			}

			payloadOffset =_offsetBodyPlayloadSent =0;
			_msgOutgoing->_phase = HttpMessage::hmpHeaders;
			break;

		case HttpMessage::hmpHeaders:
			if(!_msgOutgoing->hasContentBody())
			{
				_msgOutgoing->_phase = HttpMessage::hmpCompleted;
				break;
			}

			_msgOutgoing->_phase = HttpMessage::hmpBody;
			// no break here, continue move to the next case

		case HttpMessage::hmpBody:
			{
				Payload tmpQueue;
				int bytesToWrite =0;

				if(!_msgOutgoing->chunked())
				{
					for (; !_payloadOutgoing.empty(); _payloadOutgoing.pop())
					{
						PayloadChunk::Ptr& chunk = _payloadOutgoing.front();
						if (!chunk)
							continue;

						ret = TCPConnection::_enqueueSend((const uint8*)chunk->data(), chunk->len());
						_logger.hexDump(ZQ::common::Log::L_DEBUG, chunk->data(), chunk->len(), "sendByPhase() body:");
						if (ret >0)
							payloadBytesSent += ret;
					}

					_msgOutgoing->_phase = HttpMessage::hmpCompleted;
					break;
				}

				// now deal with chunked
				size_t cChunks = 0;
				if (_payloadOutgoing.empty())
				{
					static const char* chunkEnd = "0\r\n\r\n";
					ret = TCPConnection::_enqueueSend((const uint8*)chunkEnd, strlen(chunkEnd));
					_msgOutgoing->_phase = HttpMessage::hmpCompleted;
					break;
				}

				for (; !_payloadOutgoing.empty(); _payloadOutgoing.pop(), cChunks++)
				{
					PayloadChunk::Ptr& chunk = _payloadOutgoing.front();
					size_t len = chunk->len();
					if (len <=0)
						continue;

					char chunkHdr[16];
					snprintf(chunkHdr, sizeof(chunkHdr) -2, "%x\r\n", len);
					ret = TCPConnection::_enqueueSend((const uint8*)chunkHdr, strlen(chunkHdr));
					ret = TCPConnection::_enqueueSend((const uint8*)chunk->data(), chunk->len());
					_logger.hexDump(ZQ::common::Log::L_DEBUG, chunk->data(), chunk->len(), "sendByPhase() chunk:");
					if (ret >0)
						payloadBytesSent += ret;
				}
			}

			break;

		case HttpMessage::hmpCompleted:
			// reset to init
			// _msgOutgoing->_phase = HttpMessage::hmpStarted;
			msgCompleted = _msgOutgoing;
			_msgOutgoing = NULL;

		    _logger(ZQ::common::Log::L_DEBUG, CONNFMT(sendByPhase, "message sent"));
			break;
		}

		if (payloadBytesSent >0)
		{
			_offsetBodyPlayloadSent += payloadBytesSent;
			payloadOffset = _offsetBodyPlayloadSent;
		}
	}

	if (payloadBytesSent >0)
		OnBodyPayloadSubmitted(payloadBytesSent, payloadOffset);

	if (msgCompleted)
		OnMessageSubmitted(msgCompleted);

	return true;
}

void HttpConnection::OnConnected(ElpeError status)
{
	TCPConnection::OnConnected(status);

	{
		SEND_GUARD();
		if (_msgOutgoing)
			_msgOutgoing->_phase = HttpMessage::hmpStarted; // HTTP doesn't support continue at reconnected, so always restart the request once connected
	}

	sendByPhase();
}

void HttpConnection::OnConnectionError(int error, const char* errorDescription) 
{ 
	OnMessagingError(error, errorDescription); 

	_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpConnection, "disconnecting due to error(%d): %s"), error, errorDescription);
	disconnect(false); 
}

void HttpConnection::OnWrote(int status)
{
	TCPConnection::OnWrote(status);
	sendByPhase();
}

void HttpConnection::OnRead(ssize_t nread, const char *buf)
{
	TCPConnection::OnRead(nread, buf);
	if (nread <= 0)
		return;

	_stampLastRecv = ZQ::common::now();
	((NestedParser*)_nestedParser)->parse(buf, nread);
}

// void HttpConnection::OnClose()
// {
// 	delete this;
// }
// 
// void HttpConnection::OnShutdown(ElpeError status)
// {
// 	if (status != elpeSuccess)
// 		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpConnection,"shutdown error code[%d] Description[%s]"),status,errDesc(status));
// 
// 	close();
// }

int	HttpConnection::onParser_HeadersCompleted()
{
	HttpMessage::Ptr msg;
	{
		RECV_GUARD();
		msg = ((NestedParser*)_nestedParser)->msg;
	}

	assert(msg);
	HttpMessage::StatusCodeEx recvResult = HttpMessage::scOK;
	ZQ::common::MutexGuard g(msg->_locker);

	msg->_declaredBodyLength =(int64)((NestedParser*)_nestedParser)->parser.content_length;
	msg->_flags = ((NestedParser*)_nestedParser)->parser.flags;
	//msg->_bKeepAlive = (0 != (msg->_flags & F_CONNECTION_KEEP_ALIVE));
	//msg->_bChunked   = (0 != (msg->_flags & F_CHUNKED));
	msg->_statusCode = ((int)((NestedParser*)_nestedParser)->parser.status_code);
	msg->_method = ((HttpMessage::HttpMethod)((NestedParser*)_nestedParser)->parser.method);
	msg->_httpVMajor = ((NestedParser*)_nestedParser)->parser.http_major;
	msg->_httpVMinor =((NestedParser*)_nestedParser)->parser.http_minor;

	std::string host, uri, qstr, uristr = ((NestedParser*)_nestedParser)->url;
	int port =-1;
	HttpMessage::chopURI(uristr, host, port, msg->_uri, msg->_qstr);
	_logger(ZQ::common::Log::L_DEBUG, CONNFMT(onParser_HeadersCompleted, "result(%d) uristr[%s]=> uri[%s] qstr[%s]"), msg->_statusCode, uristr.c_str(), msg->_uri.c_str(), msg->_qstr.c_str());

	// validating the received headers
	do {
		//ZQ::common::URLStr decoder(NULL, true); // case sensitive
		//if(decoder.parse(((NestedParser*)_nestedParser)->msg->_url.c_str()))
		//	msg->_qstr = decoder.getEnumVars();

		//// cut off the paramesters
		//size_t pos = msg->_url.find_first_of("?#");
		//if (std::string::npos != pos)
		//	msg->_url = msg->_url.substr(0, pos);

		for (HttpMessage::Headers::iterator it = msg->_headers.begin(); HTTP_SUCC(recvResult) && it!=msg->_headers.end(); it++)
		{
			if (strcmp(it->first.c_str(), "Content-Type") == 0)
			{
				// determining _encoding
				msg->_encoding = HttpMessage::heNone;
				if(strstr(it->second.c_str(), "application/x-www-form-urlencoded"))
				{
					msg->_encoding = HttpMessage::heFormUrlEncoded;
					continue;
				}

				if(strstr(it->second.c_str(), "multipart/form-data"))
				{ 
					// search for boundary
					msg->_encoding = HttpMessage::heFormMultiparts;
					msg->_boundary.clear();
					const char* boundary = strstr(it->second.c_str(), "boundary=");
					if(boundary)
						msg->_boundary = ZQ::eloop::trim(boundary + 9); // 9 == strlen("boundary=")

					if( msg->_boundary.empty())
						recvResult = HttpMessage::errNullBundary;
				}

				continue;
			} // if Content-Type
		}

	} while(0);

	if (!HTTP_SUCC(recvResult))
	{
		OnMessagingError(recvResult, HttpMessage::code2status(recvResult));
		return -2;
	}

	int http_err = ((NestedParser*)_nestedParser)->parser.http_errno;
	if (http_err != 0)
	{
		char tmp[60];
		snprintf(tmp, sizeof(tmp)-2, "http parse error(%d)", http_err);
		_logger(ZQ::common::Log::L_ERROR, CONNFMT(onParser_HeadersCompleted, "%s"), tmp);
		OnMessagingError(HttpMessage::errHTTPParse, tmp);
		return -2;

	}

	if(!OnHeadersReceived(((NestedParser*)_nestedParser)->msg))
	{
		_logger(ZQ::common::Log::L_DEBUG, CONNFMT(onParser_HeadersCompleted, "app cancelled receiving"));
		return HttpMessage::errBackendProcess;//user cancel parsing procedure
	}

	return 0;
}

int	HttpConnection::onParser_MessageComplete()
{
	HttpMessage::Ptr msg;
	{
		RECV_GUARD();
		msg = ((NestedParser*)_nestedParser)->msg;
	}

	assert(msg);
	ZQ::common::MutexGuard g(msg->_locker);
	HttpMessage::StatusCodeEx recvResult = HttpMessage::scOK;

	/////////////////////////int r = ((NestedParser*)_nestedParser)->msg->onMessageComplete();
	switch(msg->_encoding)
	{
	case HttpMessage::heFormUrlEncoded: // decode the post data in the buffer
		{
			// decode the body payload into msg->_qstr
			RECV_GUARD();
			if (!_payloadReceived.empty() && _payloadReceived.front())
			{ 
				ZQ::common::URLStr decoder(NULL, true); // case sensitive
				if(!decoder.parse((const char*)_payloadReceived.front()->data()))
				{
					_logger(ZQ::common::Log::L_ERROR, CONNFMT(onParser_MessageComplete, "decord FormUrl failed"));
					recvResult = HttpMessage::errBodyDecode;
					break;
				}

				std::map<std::string, std::string> vars = decoder.getEnumVars();
				std::map<std::string, std::string>::const_iterator it;
				for(it = vars.begin(); it != vars.end(); ++it)
					msg->_qstr += it->first + "=" + it->second +"&";
				//				queryString(it->first, it->second);
			}
		}

		break;

	case HttpMessage::heFormMultiparts:
		{
			RECV_GUARD();
			if (!_payloadReceived.empty() && _payloadReceived.front())
			{
				// decode the body data into msg->_qstr
				PayloadChunk::Ptr& payload = _payloadReceived.front();
				// processing the multipart content
				std::string dashBoundary = std::string("--") + msg->_boundary;
				std::string delimeter = std::string("\r\n") + dashBoundary;
				const char* p = (const char*)payload->data(); // the current position
				const char* pEnd = p + payload->len(); // the end position
				p = std::search(p, pEnd, dashBoundary.begin(), dashBoundary.end());
				while(p < pEnd)
				{
					if(0 == memcmp(p + dashBoundary.size(), "--", 2)) // end of the multipart, normal quit
						break;

					ZQ::eloop::MIMEReader lineReader;
					size_t len = pEnd - p;
					if(!lineReader.getLine(p, len))
					{ 
						_logger(ZQ::common::Log::L_ERROR, CONNFMT(onParser_MessageComplete, "MIME failed to find line"));
						recvResult = HttpMessage::errMIMEPart;
						break;
					}

					lineReader.clear();

					std::string name, filename, contentType, transferEncoding;
					const char* hdr = NULL;
					while((hdr = lineReader.getLine(p, len)))
					{
						if ('\0' == *hdr)
							break; // blank line, header end

						if (0 == strncmp(hdr, "Content-disposition:", strlen("Content-disposition:")))
						{   // parse the content-disposition
							// get the name and file name
							std::vector<std::string> params;
							ZQ::eloop::split(params, hdr + strlen("Content-disposition:"), ";");
							// parameter's format: key=value
							for(size_t i = 0; i < params.size(); ++i)
							{
								std::string k, v;
								std::string::size_type pos = params[i].find('=');
								if(pos != std::string::npos)
								{
									k = ZQ::eloop::trim(params[i].substr(0, pos));
									v = ZQ::eloop::trim(params[i].substr(pos + 1), " \"");
								}

								if(0 == stricmp(k.c_str(), "name"))
									name = v;
								else if(0 == stricmp(k.c_str(), "filename"))
									filename = v;
							} // for end (parameter)
						}
						else if(0 == strncmp(hdr, "Content-Type:", strlen("Content-Type:")))
							contentType = ZQ::eloop::trim(hdr + strlen("Content-Type:"));
						else if(0 == strncmp(hdr, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")))
							transferEncoding = ZQ::eloop::trim(hdr + strlen("Content-Transfer-Encoding:"));

						lineReader.clear();
					}

					// find the end of the multi-part body
					const char* end = std::search(p, pEnd, delimeter.begin(), delimeter.end());
					if(end == pEnd) // can't find the body end
					{ 
						_logger(ZQ::common::Log::L_ERROR, CONNFMT(onParser_MessageComplete, "MIME failed to chop by %s"), msg->_boundary.c_str());
						recvResult = HttpMessage::errBodyEnd; 
						break;
					}

					msg->_qstr += name + "=" + (p?p:"") +"&"; // queryString(name,p);
					p = end + 2; // point to the boundary, continue the next iteration
				} // while end (multi-part)
			}
		}
		break;

	case HttpMessage::heNone: // pass through
	default:
		break;
	}

	if (!HTTP_SUCC(recvResult))
	{
		OnMessagingError(recvResult, HttpMessage::code2status(recvResult));
		return -1; //cancel parsing procedure
	}
	
	OnMessageReceived(((NestedParser*)_nestedParser)->msg);
	((NestedParser*)_nestedParser)->msg = NULL;

	return -1; //cancel parsing procedure
}

std::string HttpConnection::formatStartLine(HttpMessage::Ptr msg) 
{
	if (!msg)
		return "";

	std::ostringstream oss;
	ZQ::common::MutexGuard gd(msg->_locker);

	if (msg->_type == HttpMessage::MSG_RESPONSE)
		oss<<"HTTP/1.1" <<" " << msg->_statusCode <<" " << HttpMessage::code2status(msg->_statusCode) << EOL;
	else
	{
		std::string uristr = msg->_uri;
		if (HttpMessage::POST != msg->_method && msg->_qstr.length() >0 )
			uristr += msg->_qstr;

		oss<< HttpMessage::method2str(msg->_method) << " " << uristr << " " << "HTTP/1.1"<< EOL;
	}

	return oss.str();
}

std::string HttpConnection::formatMsgHeaders(HttpMessage::Ptr msg) 
{
	if (!msg)
		return "";

	std::ostringstream oss;
	ZQ::common::MutexGuard gd(msg->_locker);

	if (msg->chunked())
	{
		msg->_headers["Transfer-Encoding"] = "chunked";
		msg->_headers.erase("Content-Length");
	}
	else 
	{
		msg->_headers.erase("Transfer-Encoding");
		int64 clen = msg->contentLength();
		if(clen <= 0 )
			msg->_headers.erase("Content-Length");
		else
		{
			std::ostringstream ossBL; ossBL << clen;
			msg->_headers["Content-Length"] = ossBL.str();
		}
	}

	msg->_headers["Connection"] = msg->keepAlive() ? "keep-alive" : "close";
	
	for(HttpMessage::Headers::const_iterator it = msg->_headers.begin(); it != msg->_headers.end(); it ++ )
		oss<<it->first<<": "<<it->second<<EOL;

	oss<<EOL;
	return oss.str();
}


int	HttpConnection::onParser_Url(const char* at, size_t size)
{
	RECV_GUARD();
	((NestedParser*)_nestedParser)->url.append(at, size);
	return 0;
}

int	HttpConnection::onParser_Status(const char* at, size_t size)
{
	RECV_GUARD();
	((NestedParser*)_nestedParser)->msg->_status.append(at, size);
	return 0;
}

int	HttpConnection::onParser_HeaderField(const char* at, size_t size)
{
	RECV_GUARD();
	((NestedParser*)_nestedParser)->headerValue = "";
	((NestedParser*)_nestedParser)->headerName.append(at,size);
	return 0;
}

int	HttpConnection::onParser_HeaderValue(const char* at, size_t size)
{
	RECV_GUARD();
	if (!((NestedParser*)_nestedParser)->headerValue.empty())
	{
		((NestedParser*)_nestedParser)->headerValue.append(at, size);
		return 0;
	}

	std::pair<HttpMessage::Headers::iterator, bool> ret = ((NestedParser*)_nestedParser)->msg->_headers.insert(HttpMessage::Headers::value_type(((NestedParser*)_nestedParser)->headerName, std::string(at,size)));
	if(!ret.second)
	{
		((NestedParser*)_nestedParser)->msg->_headers.erase(((NestedParser*)_nestedParser)->headerName);
		ret = ((NestedParser*)_nestedParser)->msg->_headers.insert(HttpMessage::Headers::value_type(((NestedParser*)_nestedParser)->headerName, std::string(at,size)));
		assert(ret.second);
	}

	((NestedParser*)_nestedParser)->headerValue = ret.first->second;
	((NestedParser*)_nestedParser)->headerName.clear();
	return 0;
}

int	HttpConnection::onParser_Body(const char* at, size_t size)
{
	switch(OnBodyPayloadReceived(at, size))
	{
	case 0:
	case HttpMessage::errAsyncInProgress:
		return 0;

	default:
		break;
	}

	return -1; // user cancelled resceiving
}

//@return expect errAsyncInProgress to continue receiving 
HttpMessage::StatusCodeEx HttpConnection::OnBodyPayloadReceived(const char* data, size_t size)
{
	HttpMessage::Ptr msg;
	{
		RECV_GUARD();
		msg = ((NestedParser*)_nestedParser)->msg;
	}

	assert(msg);
	RECV_GUARD();
	PayloadChunk::Ptr payload = new PayloadChunk((const uint8 *) data, size);
	_payloadReceived.push(payload);
	return HttpMessage::errAsyncInProgress; 

	//switch(((NestedParser*)_nestedParser)->msg->_encoding)
	//{
	//case HttpMessage::heFormUrlEncoded:
	//case HttpMessage::heFormMultiparts:
	//	return 0; // done at this case

	//case HttpMessage::heNone: // pass through
	//default:
	//	break;
	//}

	//return HttpMessage::errAsyncInProgress; 
}


HttpConnection::PayloadChunk::Ptr HttpConnection::pushOutgoingPayload(const void* data, size_t len, bool bEnd)
{
	SEND_GUARD();
	if (NULL == data || len<=0 || !_bOutgoingOpen)
		return NULL;

	PayloadChunk::Ptr ret = new PayloadChunk((const uint8 *)data, len);
	_payloadOutgoing.push(ret);
	_bOutgoingOpen = !bEnd;

	return ret;
}

HttpConnection::PayloadChunk::Ptr HttpConnection::popReceivedPayload()
{
	PayloadChunk::Ptr ret;
	RECV_GUARD();
	while (!_payloadReceived.empty())
	{
		ret = _payloadReceived.front();
		_payloadReceived.pop();
		if (ret)
			break;
	}

	return ret;
}

// ---------------------------------------
// class NestedParser
// ---------------------------------------
int NestedParser::_parserCb_message_begin(http_parser* parser)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	pThis->msg->_phase = HttpMessage::hmpStarted;
	return pThis->_conn.onParser_MessageBegin();
}

int NestedParser::_parserCb_headers_complete(http_parser* parser)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	pThis->msg->_phase = HttpMessage::hmpBody;
	return pThis->_conn.onParser_HeadersCompleted();
}

int NestedParser::_parserCb_message_complete(http_parser* parser)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	pThis->msg->_phase = HttpMessage::hmpCompleted;
	return pThis->_conn.onParser_MessageComplete();
}

int NestedParser::_parserCb_uri(http_parser* parser,const char* at,size_t size)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_Url(at, size);
}

int NestedParser::_parserCb_status(http_parser* parser, const char* at, size_t size)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_Status(at, size);
}

int NestedParser::_parserCb_header_field(http_parser* parser, const char* at, size_t size)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_HeaderField(at, size);
}

int NestedParser::_parserCb_header_value(http_parser* parser, const char* at, size_t size)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_HeaderValue(at, size);
}

int NestedParser::_parserCb_body(http_parser* parser, const char* at, size_t size)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_Body(at, size);
}

int NestedParser::_parserCb_chunk_header(http_parser* parser)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_ChunkStart(parser->content_length);
}

int NestedParser::_parserCb_chunk_complete(http_parser* parser)
{
	NestedParser* pThis = reinterpret_cast<NestedParser*>(parser->data); if (!pThis) return 0;
	return pThis->_conn.onParser_ChunkCompleted(parser->content_length);
}

}} //namespace ZQ::eloop
