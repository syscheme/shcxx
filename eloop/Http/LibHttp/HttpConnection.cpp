#include "HttpConnection.h"
#include "http_parser.h"
#include "HttpMessage.h"
#include <assert.h>

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpConnection
// ---------------------------------------
HttpConnection::HttpConnection(bool clientSide,ZQ::common::Log& logger,TCPServer* tcpServer)
		:TCPConnection(logger,tcpServer),
		_Type(clientSide?HttpMessage::MSG_RESPONSE:HttpMessage::MSG_REQUEST),
		_Parser(NULL), _RespState(RESP_COMPLETE), _ParserSettings(NULL)
{
	_Parser = (http_parser*)malloc(sizeof(http_parser));
	_ParserSettings = (http_parser_settings*)malloc(sizeof(http_parser_settings));
	_ParserSettings->on_body				= HttpConnection::on_body;
	_ParserSettings->on_header_field		= HttpConnection::on_header_field;
	_ParserSettings->on_header_value		= HttpConnection::on_header_value;
	_ParserSettings->on_headers_complete	= HttpConnection::on_headers_complete;
	_ParserSettings->on_message_begin		= HttpConnection::on_message_begin;
	_ParserSettings->on_message_complete	= HttpConnection::on_message_complete;
	_ParserSettings->on_status				= HttpConnection::on_status;
	_ParserSettings->on_url					= HttpConnection::on_uri;
	_ParserSettings->on_chunk_header		= HttpConnection::on_chunk_header;
	_ParserSettings->on_chunk_complete		= HttpConnection::on_chunk_complete;

	reset();
}


HttpConnection::~HttpConnection()
{
	free(_Parser);
	free(_ParserSettings);
}

void HttpConnection::reset(IHttpParseSink* p)
{
	if(!p)
		p = dynamic_cast<IHttpParseSink*>(this);

	_Callback	= p;

	http_parser_init(_Parser,(http_parser_type)_Type);

	_Parser->data = reinterpret_cast<void*>(this);
	_HeaderField.clear();
	_HeaderValue = NULL;
	_ParserState = STATE_INIT;

	_CurrentParseMsg = new HttpMessage(_Type);
}

void HttpConnection::OnRead(int nread, const char *buf)
{
	if (nread < 0)
	{
		std::string desc = "Read error:";
		desc.append(errDesc(nread));
		onError(nread,desc.c_str());
		return;
	}

	if (nread == 0)
		return;

	onHttpDataReceived(nread);
	
	parse(buf, nread);
}

void HttpConnection::OnWrote(int status)
{
	if (status != elpeSuccess)
	{
		std::string desc = "send error:";
		desc.append(errDesc(status));
		onError(status,desc.c_str());
		return;
	}
	
	onHttpDataSent(status);

	if ((_RespState == RESP_COMPLETE) && (!_listpipe.empty()))
	{
		AsyncBuf::Ptr bufptr = _listpipe.front();
		_listpipe.pop_front();
		parse(bufptr->_base,bufptr->_len);
	}
}

void HttpConnection::OnClose()
{
	delete this;
}

void HttpConnection::OnShutdown(ElpeError status)
{
	if (status != elpeSuccess)
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(HttpConnection,"shutdown error code[%d] Description[%s]"),status,errDesc(status));

	close();
}

void HttpConnection::parse( const char* data, size_t size)
{

	if (_RespState != RESP_COMPLETE)
	{
		AsyncBuf::Ptr inflowPtr = new AsyncBuf(data,size);
		_listpipe.push_back(inflowPtr);
		return;
	}

	if( _ParserState >= STATE_COMPLETE) {
		reset();
		//_Logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"reset,state = %d"),_ParserState);
	}
//	_Logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"parse data [%d][%s]"),size,data);

	size_t nparsed = http_parser_execute(_Parser, _ParserSettings, data, size);

//	_Logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"parsed = %d,size = %d"),nparsed,size);

	if (nparsed == size)
		return;

	if (_ParserState >= STATE_COMPLETE)
	{
		const char* tempbuf = data + nparsed;
		size_t templen = size - nparsed;
		parse(tempbuf, templen);
		return;
	}

	std::string parsedesc = "parse error:";
	parsedesc.append(http_errno_description((http_errno)_Parser->http_errno));
	onError((int)_Parser->http_errno,parsedesc.c_str());
}

int HttpConnection::beginSend(HttpMessage::Ptr resp)
{
	assert(resp != NULL && "msg can not be NULL");
	_RespMsg = resp;
	std::string head = _RespMsg->toRaw();
	
	int ret = write(head.c_str(),head.length());
	onRespHeader();
	return ret;
}

int HttpConnection::SendBody(char *buf, size_t length)
{
	if(!_RespMsg->hasContentBody() ) 
	{
		assert( false && "http message do not have a content body");
		return -1;
	}

	int ret = 0;
	if(!_RespMsg->chunked() )
		ret = write(buf,length);
	else
	{
		eloop_buf_t chunkbuf[3];
		char chhead[16];
		sprintf(chhead, "%x\r\n",length);

		chunkbuf[0].base = chhead;
		chunkbuf[0].len = strlen(chhead);
		chunkbuf[1].base = buf;
		chunkbuf[1].len = length;
		chunkbuf[2].base = "\r\n";
		chunkbuf[2].len = 2;
		ret = write(chunkbuf,3);
	}

	onRespBody();
	return ret;
}

int HttpConnection::endSend()
{
	int ret = 0;
	if(_RespMsg->chunked())
	{
		char* chunkEnd = "0\r\n\r\n";
		ret = write(chunkEnd,strlen(chunkEnd));
	}

	onRespComplete();
	return ret;
}

int	HttpConnection::onMessageBegin( )
{
	_ParserState = STATE_HEADERS;
	return 0;
}

int	HttpConnection::onHeadersComplete()
{
	assert(_Callback != NULL);
	_CurrentParseMsg->contentLength((int64)_Parser->content_length);
	_CurrentParseMsg->keepAlive((_Parser->flags & F_CONNECTION_KEEP_ALIVE) != 0);
	_CurrentParseMsg->chunked((_Parser->flags & F_CHUNKED) != 0 );
	_CurrentParseMsg->code((int)_Parser->status_code);
	_CurrentParseMsg->method((HttpMessage::HttpMethod)_Parser->method);
	_CurrentParseMsg->setVersion(_Parser->http_major, _Parser->http_minor);

	int r = _CurrentParseMsg->onHeadersComplete();
	if (r != 0)
	{
		_Callback->onError(r,_CurrentParseMsg->errorCode2Desc(r));
		return -2;
	}

	_ParserState = STATE_BODY;
	if(_Parser->http_errno == 0)
	{
		assert(_Callback != NULL);
		if(!_Callback->onHeadersEnd(_CurrentParseMsg))
			return -1;//user cancel parsing procedure

		return 0;
	}

	return -2;//failed to parse http raw message
}

int	HttpConnection::onMessageComplete()
{
	assert(_Callback != NULL);

	int r = _CurrentParseMsg->onMessageComplete();
	if (r != 0)
	{
		_Callback->onError(r,_CurrentParseMsg->errorCode2Desc(r));
		return -1;
	}
	
	_Callback->onMessageCompleted();
	_ParserState = STATE_COMPLETE;
	_CurrentParseMsg = NULL;

	return -1;//cancel parsing procedure
//	return 0;
}

int	HttpConnection::onUri(const char* at, size_t size)
{
	_CurrentParseMsg->_Uri.append(at, size);
	return 0;
}

int	HttpConnection::onStatus(const char* at, size_t size)
{
	_CurrentParseMsg->_Status.append(at, size);
	return 0;
}

int	HttpConnection::onHeaderField(const char* at, size_t size)
{
	_HeaderValue = NULL;
	_HeaderField.append(at,size);
	return 0;
}

int	HttpConnection::onHeaderValue(const char* at, size_t size)
{
	if(NULL != _HeaderValue) 
		_HeaderValue->append(at, size);
	else
	{
		std::pair<HttpMessage::Headers::iterator,bool> ret = _CurrentParseMsg->_Headers.insert(HttpMessage::Headers::value_type(_HeaderField, std::string(at,size)));
		if(!ret.second)
		{
			_CurrentParseMsg->_Headers.erase(_HeaderField);
			ret = _CurrentParseMsg->_Headers.insert(HttpMessage::Headers::value_type(_HeaderField, std::string(at,size)));
			assert(ret.second);
		}

		_HeaderValue = &ret.first->second;
		_HeaderField.clear();
	} 

	return 0;
}

int	HttpConnection::onBody(const char* at, size_t size)
{
	if (0 == _CurrentParseMsg->onBody(at,size))
		return 0;

	assert(_Callback!=NULL);
	if(!_Callback->onBodyData(at, size))
		return -1; //user cancel parsing procedure

	return 0;
}

int	HttpConnection::onChunkHeader(http_parser* parser)
{
	return 0;
}

int	HttpConnection::onChunkComplete(http_parser* parser)
{
	return 0;
}

int HttpConnection::on_message_begin(http_parser* parser)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onMessageBegin();
}

int HttpConnection::on_headers_complete(http_parser* parser)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onHeadersComplete();
}

int HttpConnection::on_message_complete(http_parser* parser)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onMessageComplete();
}

int HttpConnection::on_uri(http_parser* parser,const char* at,size_t size)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onUri(at, size);
}

int HttpConnection::on_status(http_parser* parser, const char* at, size_t size)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onStatus(at, size);
}

int HttpConnection::on_header_field(http_parser* parser, const char* at, size_t size)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onHeaderField(at, size);
}

int HttpConnection::on_header_value(http_parser* parser, const char* at, size_t size)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onHeaderValue(at, size);
}

int HttpConnection::on_body(http_parser* parser, const char* at, size_t size)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onBody(at, size);
}

int HttpConnection::on_chunk_header(http_parser* parser)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onChunkHeader(parser);
}

int HttpConnection::on_chunk_complete(http_parser* parser)
{
	HttpConnection* pThis = reinterpret_cast<HttpConnection*>(parser->data);
	return pThis->onChunkComplete(parser);
}

}} //namespace ZQ::eloop
