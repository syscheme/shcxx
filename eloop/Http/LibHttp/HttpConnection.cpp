#include "HttpConnection.h"
#include "http_parser.h"
#include "HttpMessage.h"
#include <assert.h>

namespace ZQ {
namespace eloop {
// ---------------------------------------
// class HttpConnection
// ---------------------------------------
HttpConnection::HttpConnection(ZQ::common::Log& logger,const char* connId,TCPServer* tcpServer)
		:TCPConnection(logger, connId, tcpServer),
		_parser(NULL), _respState(RESP_COMPLETE), _parserSettings(NULL)
{
	_parser = (http_parser*)malloc(sizeof(http_parser));
	_parserSettings = (http_parser_settings*)malloc(sizeof(http_parser_settings));
	_parserSettings->on_body				= HttpConnection::on_body;
	_parserSettings->on_header_field		= HttpConnection::on_header_field;
	_parserSettings->on_header_value		= HttpConnection::on_header_value;
	_parserSettings->on_headers_complete	= HttpConnection::on_headers_complete;
	_parserSettings->on_message_begin		= HttpConnection::on_message_begin;
	_parserSettings->on_message_complete	= HttpConnection::on_message_complete;
	_parserSettings->on_status				= HttpConnection::on_status;
	_parserSettings->on_url					= HttpConnection::on_uri;
	_parserSettings->on_chunk_header		= HttpConnection::on_chunk_header;
	_parserSettings->on_chunk_complete		= HttpConnection::on_chunk_complete;

	reset();
}


HttpConnection::~HttpConnection()
{
	free(_parser);
	free(_parserSettings);
}

void HttpConnection::reset(IHttpParseSink* p)
{
	if(!p)
		p = dynamic_cast<IHttpParseSink*>(this);

	_cbParse = p;

	http_parser_init(_parser, (isPassive() ? HTTP_REQUEST : HTTP_RESPONSE));

	_parser->data = reinterpret_cast<void*>(this);
	_headerName.clear();
	_HeaderValue = NULL;
	_ParserState = STATE_INIT;

	_msgBeingParsed = new HttpMessage(isPassive() ? HttpMessage::MSG_REQUEST : HttpMessage::MSG_RESPONSE);
}
/*
const char* HttpConnection::httpError(int& err)
{
	// step 1. fixup some uv errors
	switch(err)
	{
	case elpuEOF:
			if (in chunked-mode)
			{
				if (!last chunk size=0)
				{
					err = http_connection_lost;
					break;
				}
			}

			break;
	}

	// step 2. the desc string the error after fix-up
	switch(err)
	{
		case http_connection_lost: return "connect lost";
		default: // others forward to uv error
			return errDesc(nread)
	}
}
*/
void HttpConnection::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0)
	{
		std::string desc = "Read error:";
		desc.append(errDesc(nread));
		OnConnectionError(nread, desc.c_str());
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
		std::string desc = std::string("send failed: ") + _linkstr + " " + errDesc(status);
		OnConnectionError(status,desc.c_str());
		return;
	}
	
	onHttpDataSent(status);

	if ((_respState == RESP_COMPLETE) && (!_listpipe.empty()))
	{
		AsyncBuf::Ptr bufptr = _listpipe.front();
		_listpipe.pop_front();
		parse(bufptr->_base,bufptr->_len);
	}
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

void HttpConnection::parse( const char* data, size_t size)
{

	if (_respState != RESP_COMPLETE)
	{
		AsyncBuf::Ptr inflowPtr = new AsyncBuf(data,size);
		_listpipe.push_back(inflowPtr);
		return;
	}

	if( _ParserState >= STATE_COMPLETE) {
		reset();
		//_logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"reset,state = %d"),_ParserState);
	}
//	_logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"parse data [%d][%s]"),size,data);

	size_t nparsed = http_parser_execute(_parser, _parserSettings, data, size);

//	_logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"nparsed[%d] size[%d] parseMsg[%s]"), nparsed, size, data);


//	_logger(ZQ::common::Log::L_INFO,CLOGFMT(HttpConnection,"parsed = %d,size = %d"),nparsed,size);

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
	parsedesc.append(http_errno_description((http_errno)_parser->http_errno));
	OnConnectionError((int)_parser->http_errno, parsedesc.c_str());
}

int HttpConnection::beginSend(HttpMessage::Ptr resp)
{
	assert(resp != NULL && "msg can not be NULL");
	_respMsg = resp;
	std::string head = _respMsg->toRaw();
	
	int ret = (head.c_str(),head.length());
	onRespHeader();
	return ret;
}

int HttpConnection::SendBody(char *buf, size_t length)
{
	if(!_respMsg->hasContentBody() ) 
	{
		assert( false && "http message do not have a content body");
		return -1;
	}

	int ret = 0;
	if(!_respMsg->chunked() )
		ret = enqueueSend((const uint8*)buf,length);
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
		ret = enqueueSend((const uint8*)chunkbuf,3);
	}

	onRespBody();
	return ret;
}

int HttpConnection::endSend()
{
	int ret = 0;
	if(_respMsg->chunked())
	{
		char* chunkEnd = "0\r\n\r\n";
		ret = enqueueSend((const uint8*)chunkEnd,strlen(chunkEnd));
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
	assert(_cbParse != NULL);
	_msgBeingParsed->contentLength((int64)_parser->content_length);
	_msgBeingParsed->keepAlive((_parser->flags & F_CONNECTION_KEEP_ALIVE) != 0);
	_msgBeingParsed->chunked((_parser->flags & F_CHUNKED) != 0 );
	_msgBeingParsed->code((int)_parser->status_code);
	_msgBeingParsed->method((HttpMessage::HttpMethod)_parser->method);
	_msgBeingParsed->setVersion(_parser->http_major, _parser->http_minor);

	int r = _msgBeingParsed->onHeadersComplete();
	if (r != 0)
	{
		_cbParse->OnConnectionError(r, _msgBeingParsed->errorCode2Desc(r));
		return -2;
	}

	_ParserState = STATE_BODY;
	if(_parser->http_errno == 0)
	{
		assert(_cbParse != NULL);
		if(!_cbParse->onHeadersEnd(_msgBeingParsed))
			return -1;//user cancel parsing procedure

		return 0;
	}

	return -2;//failed to parse http raw message
}

int	HttpConnection::onMessageComplete()
{
	assert(_cbParse != NULL);

	int r = _msgBeingParsed->onMessageComplete();
	if (r != 0)
	{
		_cbParse->OnConnectionError(r, _msgBeingParsed->errorCode2Desc(r));
		return -1;
	}
	
	_cbParse->onMessageCompleted();
	_ParserState = STATE_COMPLETE;
	_msgBeingParsed = NULL;

	return -1;//cancel parsing procedure
//	return 0;
}

int	HttpConnection::onUri(const char* at, size_t size)
{
	_msgBeingParsed->_Uri.append(at, size);
	return 0;
}

int	HttpConnection::onStatus(const char* at, size_t size)
{
	_msgBeingParsed->_Status.append(at, size);
	return 0;
}

int	HttpConnection::onHeaderField(const char* at, size_t size)
{
	_HeaderValue = NULL;
	_headerName.append(at,size);
	return 0;
}

int	HttpConnection::onHeaderValue(const char* at, size_t size)
{
	if(NULL != _HeaderValue) 
		_HeaderValue->append(at, size);
	else
	{
		std::pair<HttpMessage::Headers::iterator,bool> ret = _msgBeingParsed->_Headers.insert(HttpMessage::Headers::value_type(_headerName, std::string(at,size)));
		if(!ret.second)
		{
			_msgBeingParsed->_Headers.erase(_headerName);
			ret = _msgBeingParsed->_Headers.insert(HttpMessage::Headers::value_type(_headerName, std::string(at,size)));
			assert(ret.second);
		}

		_HeaderValue = &ret.first->second;
		_headerName.clear();
	} 

	return 0;
}

int	HttpConnection::onBody(const char* at, size_t size)
{
	if (0 == _msgBeingParsed->onBody(at,size))
		return 0;

	assert(_cbParse!=NULL);
	if(!_cbParse->onBodyData(at, size))
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
