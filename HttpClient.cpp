// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: Exception.h,v 1.8 2008/04/17 09:32:35 yixin tian Exp $
// Branch: $Name:  $
// Author: YiXin Tian
// Desc  : impls a simple http client-side, refers the impl of gsoap
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/HttpClient.cpp $
// 
// 17    1/12/16 2:44p Hui.shao
// 
// 16    1/12/16 2:40p Hui.shao
// logging
// 
// 14    1/26/15 11:33a Zhiqiang.niu
// add http proxy supported
// 
// 13    5/27/14 2:57p Hongquan.zhang
// 
// 12    5/27/14 12:03p Hongquan.zhang
// 
// 11    5/27/14 12:01p Hongquan.zhang
// add a new method for fetching body content
// 
// 9     12/19/12 3:43p Li.huang
// 
// 8     8/03/12 3:33p Li.huang
// 
// 7     8/03/12 2:04p Li.huang
// 
// 6     8/03/12 1:59p Li.huang
// 
// 5     8/03/12 1:49p Li.huang
// fix log error
// 
// 4     6/13/12 4:37p Li.huang
// 
// 3     5/31/11 2:25p Li.huang
// modify receive data
// 
// 2     4/02/11 1:16p Li.huang
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 5     10-10-12 16:26 Haoyuan.lu
// 
// 4     10-09-08 15:24 Hui.shao
// added file-desc
// ===========================================================================


#include "HttpClient.h"
#include "SystemUtils.h"
#include "TimeUtil.h"
#ifdef ZQ_OS_MSWIN
	#include <Ws2tcpip.h>
#else
extern "C"
{
	#include <sys/socket.h>
}
#endif

#ifdef ZQ_OS_MSWIN
#  pragma comment(lib, "wsock32.lib")
#  pragma warning(disable : 4996) // disable deprecation warnings
# endif

#define blank(c)		((c) >= 0 && (c) <= 32)

#if defined(ZQ_OS_MSWIN)
  #define HTTP_SOCKBLOCK(fd) \
  { u_long blocking = 0; \
    ioctlsocket(fd, FIONBIO, &blocking); \
  }
  #define HTTP_SOCKNONBLOCK(fd) \
  { u_long nonblocking = 1; \
    ioctlsocket(fd, FIONBIO, &nonblocking); \
  }
#else
  #define HTTP_SOCKBLOCK(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
  #define HTTP_SOCKNONBLOCK(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
#endif

static const char http_base64o[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char http_base64i[81] = "\76XXX\77\64\65\66\67\70\71\72\73\74\75XXXXXXX\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31XXXXXX\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60\61\62\63";

#ifndef HTTP_CANARY
# define HTTP_CANARY (0xC0DE)
#endif

static const char http_padding[4] = "\0\0\0";
#define HTTP_STR_EOS (http_padding)
#define HTTP_NON_NULL (http_padding)

#ifdef HTTP_DEBUG
# ifndef HTTP_MESSAGE
#  define HTTP_MESSAGE fprintf
# endif
# ifndef DBGLOG
#  define DBGLOG(DBGFILE, CMD) \
{ if (this)\
  { if (!this->fdebug[HTTP_INDEX_##DBGFILE])\
      open_logfile( HTTP_INDEX_##DBGFILE);\
    if (this->fdebug[HTTP_INDEX_##DBGFILE])\
    { FILE *fdebug = this->fdebug[HTTP_INDEX_##DBGFILE];\
      CMD;\
      fflush(fdebug);\
    }\
  }\
}
# endif
# ifndef DBGMSG
#  define DBGMSG(DBGFILE, MSG, LEN) \
{ if (this)\
  { if (!this->fdebug[HTTP_INDEX_##DBGFILE])\
      open_logfile( HTTP_INDEX_##DBGFILE);\
    if (this->fdebug[HTTP_INDEX_##DBGFILE])\
    { fwrite((MSG), 1, (LEN), this->fdebug[HTTP_INDEX_##DBGFILE]);\
      fflush(this->fdebug[HTTP_INDEX_##DBGFILE]);\
    }\
  }\
}
# endif
# ifndef DBGHEX
#  define DBGHEX(DBGFILE, MSG, LEN) \
{ if (this)\
  { if (!this->fdebug[HTTP_INDEX_##DBGFILE])\
      open_logfile( HTTP_INDEX_##DBGFILE);\
    if (this->fdebug[HTTP_INDEX_##DBGFILE])\
    { int i; char *s;\
      for (s = (char*)(MSG), i = (LEN); i; i--)\
        fprintf(this->fdebug[HTTP_INDEX_##DBGFILE], "%2.2X  ", (int)*s++&0xFF);\
      fflush(this->fdebug[HTTP_INDEX_##DBGFILE]);\
    }\
  }\
}
# endif
#else
# define DBGLOG(DBGFILE, CMD)
# define DBGMSG(DBGFILE, MSG, LEN)
# define DBGHEX(DBGFILE, MSG, LEN)
#endif

#define HLOG if(_pHttpLog)(*_pHttpLog)
ZQ::common::Log httpNullLOg;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace ZQ{
namespace common{

HttpClient::HttpClient(ZQ::common::Log* pLog, char* pLocalIP, int nPort)
{
	_pHttpLog = (pLog != NULL) ? pLog : &httpNullLOg;
	if(pLocalIP != NULL)
		_strLocalIP = pLocalIP;
	_nLocalPort = nPort;
	if(_nLocalPort < 0)
		_nLocalPort = 0;
}

HttpClient::~HttpClient()
{
	uninit();
}

/******************************************************************************/
void HttpClient::init(int iomode)
{
	_bContinue = false;
	_bDump = true;
	_send_msg = "";
	_needrecv = 0;
	_strContent = "";
	_state = HTTP_INIT;

	_mode = iomode | HTTP_IO_DEFAULT;
	_imode = iomode | HTTP_IO_DEFAULT;
	_omode = iomode | HTTP_IO_DEFAULT;

	_userid = NULL;
	_passwd = NULL;	

	_http_version = "1.1";
	_proxy_http_version = "1.0";
	_http_content = NULL;
	_keep_alive = 0;
	_tcp_keep_alive = 0;
	_tcp_keep_idle = 0;
	_tcp_keep_intvl = 0;
	_tcp_keep_cnt = 0;
	_recv_timeout = 0;
	_send_timeout = 0;
	_connect_timeout = 0;
	_accept_timeout = 0;
	_socket_flags = 0;
	_connect_flags = 0;
	
	_alist = NULL;
	_sock = HTTP_INVALID_SOCKET;
	
	_recvfd = 0;
	_sendfd = 1;

	_host[0] = '\0';
	_port = 0;
	_action = NULL;
	_proxy_host = NULL;
	_proxy_port = 8080;
	_proxy_userid = NULL;
	_proxy_passwd = NULL;
	_authrealm = NULL;
	
#ifdef WITH_ZLIB
	_mode |= HTTP_ENC_ZLIB;
	_imode |= HTTP_ENC_ZLIB;
	_omode |= HTTP_ENC_ZLIB;
	_zlib_state = HTTP_ZLIB_NONE;
	_zlib_in = HTTP_ZLIB_NONE;
	_zlib_out = HTTP_ZLIB_NONE;
	_d_stream = (z_stream*)HTTP_MALLOC( sizeof(z_stream));
	_d_stream->zalloc = Z_NULL;
	_d_stream->zfree = Z_NULL;
	_d_stream->opaque = Z_NULL;
	_z_buf = NULL;
	_z_level = 6;
	_z_buflen = HTTP_BUFLEN;
#endif
	
#ifdef HTTP_DEBUG
	init_logs();
#endif
	
#ifdef HTTP_DEBUG
	set_recv_logfile( "RECV.log");
	set_sent_logfile( "SENT.log");
	set_test_logfile( "TEST.log");
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Initializing context\n"));
#endif

#ifdef WITH_OPENSSL
	if (!http_ssl_init_done)
	{ 
		ssl_init();
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Initializing OpenSSL, version=%ld\n", (long)OPENSSL_VERSION_NUMBER));
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Initializing OpenSSL, version=%ld"),(long)OPENSSL_VERSION_NUMBER);
	}
	fsslauth = ssl_auth_init;
	fsslverify = ssl_verify_callback;
	_bio = NULL;
	_ssl = NULL;
	_ctx = NULL;
	_ssl_flags = HTTP_SSL_DEFAULT;
	_session = NULL;
#endif
#ifndef WITH_LEAN
	initErrorCode();
#endif
	
	begin();
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "HttpClient init"));
}

/******************************************************************************/
void HttpClient::uninit()
{
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"HttpClient uninit"));
	
	if(_headmap.size())
		_headmap.clear();
	httpCloseSock();
	end();
	done();	
}

/******************************************************************************/
bool HttpClient::getContent(char* _buf, size_t bufLen )
{
	if(_buf == NULL || bufLen < _strContent.length())
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "getContent() buffer is NULL"));
		return false;
	}

	//memset(_buf,0,bufLen);
	memcpy(_buf,_strContent.c_str(),_strContent.length());
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "getContent() copied the received content to the buffer"));

	_strContent = "";
	return true;
}

size_t HttpClient::getBodyData( char* buf, size_t bufLen ) {
	if( buf == NULL || bufLen == 0)
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "getBodyData() buffer is NULL"));
		return 0;
	}

	bufLen = bufLen < _strContent.length() ? bufLen:_strContent.length();
	memcpy(buf,_strContent.c_str(),bufLen);
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"getBodyData() copied the received content to the buffer"));

	return bufLen;
}

/******************************************************************************/
void HttpClient::setLog(ZQ::common::Log* pLog)
{
	_pHttpLog = (pLog != NULL) ? pLog : &httpNullLOg;
}

/******************************************************************************/
void HttpClient::setLocalHost(std::string& strLocalIP, int nPort)
{
	_strLocalIP = strLocalIP;
	_nLocalPort = nPort;
	if(_nLocalPort < 0)
		_nLocalPort = 0;
}

/******************************************************************************/
void HttpClient::getContent(std::string& strContent)
{
	strContent = _strContent;

	_strContent = "";
}

/******************************************************************************/
size_t HttpClient::getContentLength()
{
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"getContentLength() %d bytes"),_strContent.length());
	return _strContent.length();
}

/******************************************************************************/
int HttpClient::getStatusCode()
{
	return this->_status;
}

/******************************************************************************/
void HttpClient::setConnectTimeout(int timeout)
{
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set connect timeout: %d"),timeout);
	this->_connect_timeout = timeout;
}

/******************************************************************************/
void HttpClient::setSendTimeout(int timeout)
{ 
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set send timeout: %d"),timeout);
	this->_send_timeout = timeout;
}

/******************************************************************************/
void HttpClient::setRecvTimeout(int timeout)
{ 
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set receive timeout: %d"),timeout);
	this->_recv_timeout = timeout;
}

/******************************************************************************/
void HttpClient::setKeepAlive(bool bKeep)
{
	if(bKeep)
	{
		_mode |= HTTP_IO_KEEPALIVE;			
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set connection=keep alive"));
	}
	else
	{
		_mode &= ~HTTP_IO_KEEPALIVE;
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set connection=close"));
	}
}

/******************************************************************************/
char* HttpClient::getMsg()
{
	return _msgbuf;
}

/******************************************************************************/
std::map<std::string,std::string> HttpClient::getHeader()
{
	return _headmap;
}

/******************************************************************************/
int HttpClient::getErrorcode()
{
	return _error;
}

/******************************************************************************/
const char* HttpClient::getErrorstr()
{
	return _errorstr.c_str();
}

/******************************************************************************/
bool HttpClient::isEOF()
{
	return (_needrecv > 0) ? false : true; 
}

/******************************************************************************/
void HttpClient::setHeader(char* key,char* val)
{
	//add or update header
	if(key && val)
		_headmap[key] = val;

	//erase header
	else if(val == NULL && key)
	{
		std::map<std::string,std::string>::iterator it = _headmap.find(key);
		if(it != _headmap.end())
			_headmap.erase(it);
	}
	else // clean map
	{
		if(_headmap.size())
			_headmap.clear();
	}
}

/******************************************************************************/
bool HttpClient::setRangeHeader(int nBegin, int nEnd)
{
	//format is error
	if(nBegin < 0 && nEnd < 0)
		return false;

	char chRan[30] = {0};
	if(nBegin < 0)
		sprintf(chRan,"bytes=-%d",nEnd);		
	else if(nEnd < 0)
		sprintf(chRan,"bytes=%d-",nBegin);
	else
		sprintf(chRan,"bytes=%d-%d",nBegin,nEnd);
	
	setHeader("Range",chRan);
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"set header Range: %s"),chRan);
	return true;	
}

/******************************************************************************/
bool HttpClient::getRangeHeader(int& nTotal)
{
	std::map<std::string,std::string>::const_iterator it;
	for(it = _headmap.begin();it != _headmap.end(); it++)
	{
		if(stricmp(it->first.c_str(),"Content-Range") == 0)
		{
			char chR[256] = {0};
			memcpy(chR,it->second.c_str(),it->second.size());
			char* p = strrchr(chR, '/');
			if(p)
			{
				nTotal = atoi(p+1);
				return true;
			}
			else
				return false;

		}
	}
	return false;
}

/******************************************************************************/
void HttpClient::done()
{ 
#ifdef HTTP_DEBUG
	int i;
#endif

	if(check_state(this))
		return;

	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "done with context\n"));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "Done with HttpClient"));
	_keep_alive = 0; // to force close the socket 
	httpCloseSock();
	
#ifdef WITH_OPENSSL
	if (_session)
	{ 
		SSL_SESSION_free(_session);
		_session = NULL;
	}
#endif

	if (_state == HTTP_INIT)
	{ 	
#ifdef WITH_OPENSSL
		if(_ctx)
		{ 
			SSL_CTX_free(_ctx);
			_ctx = NULL;
		}
#endif
	}

#ifdef WITH_ZLIB
	if(_d_stream)
	{ 
		HTTP_FREE((void*)_d_stream);
		_d_stream = NULL;
	}
	if (_z_buf)
	{ 
		HTTP_FREE((void*)_z_buf);
		_z_buf = NULL;
	}
#endif

#ifdef HTTP_DEBUG
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Free logfiles\n"));
	for(i = 0; i < HTTP_MAXLOGS; i++)
	{ 
		if (logfile[i])
		{ 
			HTTP_FREE( (void*)logfile[i]);
			logfile[i] = NULL;
		}
		close_logfile( i);
	}
	_state = 0;
#endif

}

/******************************************************************************/
int32 HttpClient::httpGetChar()
{ 
	register int32 c;
	c = this->_ahead;
	if (c)
	{ 
		if (c != EOF)
			this->_ahead = 0;
		return c;
	}
	return get1(this);
}

/******************************************************************************/
int HttpClient::httpBeginRecv()
{ 
	try
	{
	int32 c;
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Initializing for input\n"));
//	if(!_bContinue)
//		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "begin receiving"));
	this->_error = HTTP_OK;

	if ((this->_imode & HTTP_IO) == HTTP_IO_CHUNK)
		this->_omode |= HTTP_IO_CHUNK;
	this->_imode &= ~HTTP_IO;
	this->_mode = this->_imode;

	this->_buflen = 0;
	this->_bufidx = 0;

	memset(_buf,0,sizeof(_buf));

	if(!_bContinue && _headmap.size())
		_headmap.clear();

	if(!_bContinue && this->_totalLenth != 0)
	{
		this->_totalLenth = 0;
		this->_totalChunkSize = 0;
	}

	if (!(this->_mode & HTTP_IO_KEEPALIVE))
		this->_keep_alive = 0;
	this->_ahead = 0;
	this->_count = 0;
	*this->_endpoint = '\0';
	this->_action = NULL;

#ifdef ZQ_OS_MSWIN
	if (!valid_socket(this->_sock))
		_setmode(this->_recvfd, _O_BINARY);
#endif
#ifdef WITH_ZLIB
	if(!_bContinue)
	{	
		this->_mode &= ~HTTP_ENC_ZLIB;
		this->_zlib_in = HTTP_ZLIB_NONE;
		this->_zlib_out = HTTP_ZLIB_NONE;
		this->_d_stream->next_in = Z_NULL;
		this->_d_stream->avail_in = 0;
		this->_d_stream->next_out = (Byte*)this->_buf;
		this->_d_stream->avail_out = HTTP_BUFLEN;
	}
#endif

	c = httpGetChar();

	if ((int)c == EOF)
	{
		if( _needrecv == 1)//maybe is chunk packet
		{
			_needrecv = 0;
			return HTTP_OK;
		}
		setError("Read 0 bytes,Server has close the socket",NULL,HTTP_EOF);
		return this->_error;
	}

	this->_ahead = c;

	this->_mode &= ~HTTP_IO;
  //if it is  the first receive ,parse it
	if(!_bContinue)
	{
			if((this->_error = httpParse()))
			return this->_error;
		
		//chunk data
		if((this->_mode & HTTP_IO) == HTTP_IO_CHUNK)
		{
			this->_error = recvChunk();
			return this->_error;
		}

		if(_length != 0 && _length != (size_t)-1)
			_needrecv = (int)(_length + _bufidx - _count);
		else if(_length == 0)
			_needrecv = 0;
		else //do not known the need receive number
			_needrecv = 1;
		
		if(this->_bufidx < _count)
		{
			_strContent += std::string(this->_buf+_bufidx, _count - _bufidx);
		}
	}
	else
	{
		if(this->_length != 0 && _length != (size_t)-1)
			_needrecv -= (int)_count;
		else
			_needrecv = 1;

//		if(strlen(this->_buf) > 0)
		{
			_strContent += std::string(this->_buf,this->_count);
		}
	}

	if (this->_error && this->_error < HTTP_STOP)
	{ 
		this->_keep_alive = 0; // force close later 
		return this->_error;
	}
	if (this->_error == HTTP_STOP)
		return this->_error;


    if (this->_error)
		return this->_error;
	}
	catch (...)
	{
		return HTTP_ERR;
	}


	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpContinueRecv()
{
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"continue receiving"));
	_bContinue = true;

	if((this->_mode & HTTP_IO) == HTTP_IO_CHUNK)
	{
		this->_error = recvChunk();
		return this->_error;
	}
	else
		return httpBeginRecv();
}

/******************************************************************************/
int HttpClient::httpSendContent(const char *content, size_t contentLen)
{
	if(content == NULL || contentLen == 0)
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"send content is NULL or content _length is 0"));
		return HTTP_ERR;
	}
	_send_msg = "";
	
#ifdef WITH_ZLIB
	size_t compLen = 0;
	if(this->_error = httpDeflate((char*)content,contentLen,&compLen))
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"HttpDeflate is _error,_error code is %d"),this->_error);
		return this->_error;
	}

////////////////////test inflate///////////////////
//	_strContent = std::string(this->_z_buf,compLen);
//	if(this->_error = httpInflate((char*)_strContent.c_str(),_strContent._length()))
//		return this->_error;
///////////////////////////////////////////////////

	if ((this->_error = httpPost(this->_endpoint, this->_host, this->_port, this->_path, compLen)))
		return this->_error;
	return sendRaw(_z_buf,compLen);
#else
	if ((this->_error = httpPost(this->_endpoint, this->_host, this->_port, this->_path, contentLen)))
		return this->_error;
	return sendRaw(content,contentLen);
#endif
}

/******************************************************************************/
int HttpClient::sendRaw(const char *s, size_t n)
{ 
//	if (_pHttpLog && _bDump)
//		_pHttpLog->hexDump(ZQ::common::Log::L_DEBUG, s, n);
	if (!n)
		return HTTP_OK;
	for (size_t i = 0; i < n ; i++)
	{
		if(s[i] == '\r' || s[i] == '\n')
			_send_msg += '.';
		else
			_send_msg += s[i];
	}
	if (this->_mode & HTTP_IO_LENGTH)
	{ 
		this->_count += n;
		return HTTP_OK;
	}
	if (this->_mode & HTTP_IO)
	{ 
		register size_t i = HTTP_BUFLEN - this->_bufidx;
		while (n >= i)
		{ 
			memcpy(this->_buf + this->_bufidx, s, i);
			this->_bufidx = HTTP_BUFLEN;
			if (flush())
				return this->_error;
			s += i;
			n -= i;
			i = HTTP_BUFLEN;
		}
		memcpy(this->_buf + this->_bufidx, s, n);
		this->_bufidx += n;
		return HTTP_OK;
	}
	return flushRaw(s, n);
}

/******************************************************************************/
int HttpClient::httpCloseSock()
{ 
	register int nstatus = this->_error;
	
#ifdef WITH_ZLIB
	if (this->_zlib_state == HTTP_ZLIB_DEFLATE)
		deflateEnd(this->_d_stream);
	else if (this->_zlib_state == HTTP_ZLIB_INFLATE)
		inflateEnd(this->_d_stream);
	this->_zlib_state = HTTP_ZLIB_NONE;
#endif
	
	if (nstatus == HTTP_EOF || nstatus == HTTP_TCP_ERROR || nstatus == HTTP_SSL_ERROR || !this->_keep_alive)
	{
		if(this->_error == tcpDisconnect())
			return this->_error;
		this->_keep_alive = 0;
	}

	return this->_error = nstatus;
}

/******************************************************************************/
int HttpClient::tcpCloseSocket(HTTP_SOCKET fd)
{ 
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Close socket %d\n", (int)fd));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"close socket %d"),(int)fd);
	return http_closesocket(fd);
}

/******************************************************************************/
int HttpClient::tcpDisconnect()
{
#ifdef WITH_OPENSSL
	if (this->_ssl)
	{
		int r, s = 0;
		if (this->_session)
		{ 
			SSL_SESSION_free(this->_session);
			this->_session = NULL;
		}
		if (*(this->_host))
		{ 
			this->_session = SSL_get1_session(this->_ssl);
			if (this->_session)
			{ 
				strcpy(this->_session_host, this->_host);
				this->_session_port = this->_port;
			}
		}
		r = SSL_shutdown(this->_ssl);
		if (r == 0)
		{ 
			if (valid_socket(this->_sock))
			{ 
				struct timeval timeout;
				fd_set fd;
				if (tcpShutDownSocket( this->_sock, 1))
				{ 
		
				//  wait up to 10 seconds for close_notify to be sent by _peer (if _peer not
				//  present, this avoids calling SSL_shutdown() which has a lengthy return
				//  timeout)
		 
#ifndef ZQ_OS_MSWIN
					if ((int)this->_sock < (int)FD_SETSIZE)
					{
#endif
						timeout.tv_sec = 10;
						timeout.tv_usec = 0;
						FD_ZERO(&fd);
						FD_SET(this->_sock, &fd);
						r = select((int)this->_sock + 1, &fd, NULL, &fd, &timeout);
						if (r <= 0 && socket_errno(this->_sock) != HTTP_EINTR)
						{ 
							this->_errnum = 0;
							DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Connection lost...\n"));
							HLOG(ZQ::common::Log::L_WARNING, CLOGFMT(HttpClient,"connection lost ..."));
							tcpCloseSocket(this->_sock);
							this->_sock = HTTP_INVALID_SOCKET;
							SSL_free(this->_ssl);
							this->_ssl = NULL;
							ERR_remove_state(0);
							return HTTP_OK;
						}
#ifndef ZQ_OS_MSWIN
					}
#endif
				}
			}
			r = SSL_shutdown(this->_ssl);
		}
		if (r != 1)
		{ 
			s = ERR_get_error();
			if (s)
			{ 
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Shutdown failed: %d\n", SSL_get_error(this->_ssl, r)));
				HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"shutdown failed: %d"),SSL_get_error(this->_ssl, r));
				if (valid_socket(this->_sock) && !(this->_omode & HTTP_IO_UDP))
				{ 
					tcpCloseSocket(this->_sock);
					this->_sock = HTTP_INVALID_SOCKET;
				}
				SSL_free(this->_ssl);
				this->_ssl = NULL;
				ERR_remove_state(0);
				return HTTP_SSL_ERROR;
			}
		}
		SSL_free(this->_ssl);
		this->_ssl = NULL;
		ERR_remove_state(0);
	}
#endif
	if (valid_socket(this->_sock) && !(this->_omode & HTTP_IO_UDP))
	{ 
		tcpShutDownSocket(this->_sock, 2);
		tcpCloseSocket(this->_sock);

		this->_sock = HTTP_INVALID_SOCKET;
	}
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::tcpShutDownSocket(HTTP_SOCKET fd, int how)
{ 
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Shutdown socket %d how=%d\n", (int)fd, how));

	struct sockaddr_in sin; 
	socklen_t len = sizeof(sin); 
	int nBindPort  = _nLocalPort;
	std::string bindIp = "0.0.0.0";
	if (getsockname(fd, (struct sockaddr *)&sin, &len) != -1) 
	{
		nBindPort = ntohs(sin.sin_port); 
		bindIp = inet_ntoa(sin.sin_addr);
	}
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"socket[%d,%s:%d]=>peer[%d] closed, reason:%d"), (int)fd, bindIp.c_str(), nBindPort, (int)_recvfd, how);
	return shutdown(fd, how);
}

/******************************************************************************/
int HttpClient::setSenderError(const char *faultstring, const char *faultdetailXML, int httperror)
{ 
	return setError(faultstring, faultdetailXML, httperror);
}

/******************************************************************************/
void* HttpClient::httpMalloc(size_t n)
{ 
	register char *p;
	if(!n)
		return (void*)HTTP_NON_NULL;

	n += sizeof(short);
	n += (-(long)n) & (sizeof(void*)-1); // align at 4-, 8- or 16-byte boundary 
	if (!(p = (char*)HTTP_MALLOC( n + sizeof(void*) + sizeof(size_t))))
	{ 
		this->_error = HTTP_EOM;
		return NULL;
	}
	// set the canary to detect corruption 
	*(short*)(p + n - sizeof(short)) = (short)HTTP_CANARY;
	// keep chain of alloced cells for destruction 
	*(void**)(p + n) = this->_alist;
	*(size_t*)(p + n + sizeof(void*)) = n;
	this->_alist = p + n;

	return p;
}

/******************************************************************************/
int HttpClient::flush()
{ 
	register size_t n = this->_bufidx;
	if (n)
	{ 
		this->_bufidx = 0;

		return flushRaw(this->_buf, n);
	}
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::flushRaw(const char *s, size_t n)
{ 
#ifndef WITH_LEANER
	if ((this->_mode & HTTP_IO) == HTTP_IO_CHUNK)
	{ 
		char t[16];
		sprintf(t, "\r\n%lX\r\n" + (this->_chunksize ? 0 : 2), (unsigned long)n);
		DBGMSG(SENT, t, strlen(t));
		if ((this->_error = this->fSend(t, strlen(t))))
	   		return this->_error;
		this->_chunksize += n;
	}
//	DBGMSG(SENT, s, n);
#endif
	DBGMSG(SENT, s, n);
	return this->_error = this->fSend(s, n);
}

/******************************************************************************/
const char* HttpClient::tcpError()
{ 
	register const char *msg = NULL;
	switch (this->_errmode)
	{ 
	case 0:
		msg = httpStrError();
		break;
	case 1:
		msg = "WSAStartup failed";
		 break;
	case 2:
	{
#ifndef WITH_LEAN
		msg = codeStr(_error_codes, this->_errnum);
		if (!msg)
#endif
		{ 
			sprintf(this->_msgbuf, "TCP/UDP IP _error %d", this->_errnum);
			msg = this->_msgbuf;
		}
	}
	}
	return msg;
}

/******************************************************************************/
const char* HttpClient::httpStrError()
{ 
	register int err = this->_errnum;
	if (err)
	{
#ifndef ZQ_OS_MSWIN
		return strerror(err);
#else
		DWORD len;
		*this->_msgbuf = '\0';
		len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)this->_msgbuf, (DWORD)sizeof(this->_msgbuf), NULL);
		return this->_msgbuf;
#endif
	}
#ifndef WITH_LEAN
	if (this->_recv_timeout > 0)
	{ 
		if (this->_send_timeout > 0)
			sprintf(this->_msgbuf, "Operation interrupted or timed out after %ds send or %ds receive delay", this->_send_timeout, this->_recv_timeout);
		else
			sprintf(this->_msgbuf, "Operation interrupted or timed out after %ds receive delay", this->_recv_timeout);
		return this->_msgbuf;
	}
#endif
	return "Operation interrupted or timed out";
}

/******************************************************************************/
int HttpClient::fSend(const char *s, size_t n)
{ 
	register int nwritten, err;

	while (n)
	{ 
		if (valid_socket(this->_sock))
		{ 
#ifndef WITH_LEAN
			if (this->_send_timeout)
			{
#ifndef ZQ_OS_MSWIN
				if ((int)this->_sock >= (int)FD_SETSIZE)
					return HTTP_FD_EXCEEDED;	// Hint: MUST increase FD_SETSIZE 
#endif
				for (;;)
				{ 
					struct timeval timeout;
					fd_set fd;
					register int r;
					if (this->_send_timeout > 0)
					{ 
						timeout.tv_sec = this->_send_timeout;
						timeout.tv_usec = 0;
					}
					else
					{ 
						timeout.tv_sec = -this->_send_timeout/1000000;
						timeout.tv_usec = -this->_send_timeout%1000000;
					}
					FD_ZERO(&fd);
					FD_SET(this->_sock, &fd);
#ifdef WITH_OPENSSL
					if (this->_ssl)
						r = select((int)this->_sock + 1, &fd, &fd, &fd, &timeout);
					else
#endif
						r = select((int)this->_sock + 1, NULL, &fd, &fd, &timeout);
					if (r > 0)
						break;
					if (!r)
					{
						setSenderError("Send timeout", NULL,HTTP_TCP_ERROR);
						HLOG(ZQ::common::Log::L_WARNING, CLOGFMT(HttpClient,"send timeout"));
						this->_errnum = 0;
						return HTTP_EOF;
					}
					err = socket_errno(this->_sock);
					if (err != HTTP_EINTR && err != HTTP_EAGAIN && err != HTTP_EWOULDBLOCK)
					{ 
						this->_errnum = err;
						return HTTP_EOF;
					}
			}
      }
#endif
#ifdef WITH_OPENSSL
	if (this->_ssl)
		nwritten = SSL_write(this->_ssl, s, (int)n);
	else if (this->_bio)
        nwritten = BIO_write(this->_bio, s, (int)n);
    else
#endif
#ifndef WITH_LEAN
    if ((this->_omode & HTTP_IO_UDP))
    { 
		if (this->_peerlen)
			nwritten = sendto(this->_sock, s, (HTTP_WINSOCKINT)n, this->_socket_flags, (struct sockaddr*)&this->_peer, (HTTP_WINSOCKINT)this->_peerlen);
        else
			nwritten = send(this->_sock, s, (HTTP_WINSOCKINT)n, this->_socket_flags);
        // retry and back-off algorithm 
        // TODO: this is not very clear from specs so verify and limit conditions under which we should loop (e.g. ENOBUFS) 
        if (nwritten < 0)
        { 
			struct timeval timeout;
			fd_set fd;
			int udp_repeat;
			int udp_delay;
#ifndef ZQ_OS_MSWIN
			if ((int)this->_sock >= (int)FD_SETSIZE)
				return HTTP_FD_EXCEEDED;	// Hint: MUST increase FD_SETSIZE 
#endif
			if ((this->_connect_flags & SO_BROADCAST))
				udp_repeat = 3; // HTTP-over-UDP MULTICAST_UDP_REPEAT - 1 
			else
				udp_repeat = 1; // HTTP-over-UDP UNICAST_UDP_REPEAT - 1 
			udp_delay = (rand() % 201) + 50; // UDP_MIN_DELAY .. UDP_MAX_DELAY 
			do
			{ 
				timeout.tv_sec = 0;
				timeout.tv_usec = 1000 * udp_delay; // ms 
				FD_ZERO(&fd);
				FD_SET(this->_sock, &fd);
				select((int)this->_sock + 1, NULL, NULL, &fd, &timeout);
				if (this->_peerlen)
					nwritten = sendto(this->_sock, s, (HTTP_WINSOCKINT)n, this->_socket_flags, (struct sockaddr*)&this->_peer, (HTTP_WINSOCKINT)this->_peerlen);
				else
					nwritten = send(this->_sock, s, (HTTP_WINSOCKINT)n, this->_socket_flags);
				udp_delay <<= 1;
				if (udp_delay > 500) // UDP_UPPER_DELAY 
					udp_delay = 500;
			}while (nwritten < 0 && --udp_repeat > 0);
		}
	}
    else
#endif

  	nwritten = send(this->_sock, s, (int)n, this->_socket_flags);

	if (nwritten <= 0)
    {
#if defined(WITH_OPENSSL) || !defined(WITH_LEAN)
		register int r = 0;
#endif
        err = socket_errno(this->_sock);
#ifdef WITH_OPENSSL
        if (this->_ssl && (r = SSL_get_error(this->_ssl, nwritten)) != SSL_ERROR_NONE && r != SSL_ERROR_WANT_READ && r != SSL_ERROR_WANT_WRITE)
        { 
			this->_errnum = err;
			return HTTP_EOF;
        }
#endif
        if (err == HTTP_EWOULDBLOCK || err == HTTP_EAGAIN)
        {
#ifndef WITH_LEAN
			struct timeval timeout;
			fd_set fd;
#ifndef ZQ_OS_MSWIN
			if ((int)this->_sock >= (int)FD_SETSIZE)
				return HTTP_FD_EXCEEDED; // Hint: MUST increase FD_SETSIZE 
#endif
			if (this->_send_timeout > 0)
			{ 
				timeout.tv_sec = this->_send_timeout;
				timeout.tv_usec = 0;
			}
			else if (this->_send_timeout < 0)
			{ 
				timeout.tv_sec = -this->_send_timeout/1000000;
				timeout.tv_usec = -this->_send_timeout%1000000;
			}
			else
			{ 
				timeout.tv_sec = 0;
				timeout.tv_usec = 10000;
			}
			FD_ZERO(&fd);
			FD_SET(this->_sock, &fd);
#ifdef WITH_OPENSSL
			if (this->_ssl && r == SSL_ERROR_WANT_READ)
				r = select((int)this->_sock + 1, &fd, NULL, &fd, &timeout);
			else
				r = select((int)this->_sock + 1, NULL, &fd, &fd, &timeout);
#else
			r = select((int)this->_sock + 1, NULL, &fd, &fd, &timeout);
#endif
			if (r < 0 && (r = socket_errno(this->_sock)) != HTTP_EINTR)
			{
				HLOG(ZQ::common::Log::L_WARNING, CLOGFMT(HttpClient,"send time out"));
				this->_errnum = r;
				return HTTP_EOF;
			}
#endif
		}
        else if (err && err != HTTP_EINTR)
        { 
			this->_errnum = err;
			return HTTP_EOF;
        }
        nwritten = 0; // and call write() again 
		}
	}
    else
    {
			nwritten = write(this->_sendfd, s, (unsigned int)n);
			if (nwritten <= 0)
			{ 
				err = http_errno;
				if (err && err != HTTP_EINTR && err != HTTP_EWOULDBLOCK && err != HTTP_EAGAIN)
				{ 
					this->_errnum = err;
					return HTTP_EOF;
				}
				nwritten = 0; // and call write() again 
			}
		}
		n -= nwritten;
		s += nwritten;
	}
  return HTTP_OK;
}

/******************************************************************************/
size_t HttpClient::fRecv(char *s, size_t n)
{ 
	register int r;
#ifndef WITH_LEAN
	register int retries = 100; // max 100 retries with non-blocking sockets */
#endif
	this->_errnum = 0;

	int64 stampExp = 0;
	if (this->_recv_timeout > 0)
		stampExp = ZQ::common::now() + this->_recv_timeout * 1000;

	if (valid_socket(this->_sock))
	{ 
		for (;;)
		{ 
#ifdef WITH_OPENSSL
			register int err = 0;
#endif
#ifndef WITH_LEAN
#ifdef WITH_OPENSSL
			if (this->_recv_timeout && !this->_ssl) // SSL: sockets are nonblocking 
#else
			if (this->_recv_timeout)
#endif
			{
#ifndef ZQ_OS_MSWIN
				if ((int)this->_sock >= (int)FD_SETSIZE)
				{ 
					this->_error = HTTP_FD_EXCEEDED;
					return 0;	// Hint: MUST increase FD_SETSIZE 
				}
#endif
				for (;;)
				{ 
					struct timeval timeout;
					memset(&timeout, 0, sizeof(timeout));

					fd_set fdRead, fdErr;
					if (stampExp > 0)
					{
						int64 timeLeft = stampExp - ZQ::common::now();
						if (timeLeft < 1)
							timeLeft = 1;
						timeout.tv_sec = (long) (timeLeft / 1000);
						timeout.tv_usec = (long) (timeLeft %1000) *1000;
					}

					FD_ZERO(&fdRead);
					FD_SET(this->_sock, &fdRead);
					FD_ZERO(&fdErr);
					FD_SET(this->_sock, &fdErr);
					r = select((int)this->_sock + 1, &fdRead, NULL, &fdErr, &timeout);

					if (r <0)
					{
						r = socket_errno(this->_sock);
						if (r != HTTP_EINTR && r != HTTP_EAGAIN && r != HTTP_EWOULDBLOCK)
						{ 
							HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"socket error, errorcode=%d"),r);
							this->_errnum = r;
							return 0;
						}
					}
					else if (0 == r)
					{
						if (stampExp >0 && now() > stampExp)
						{
							r = socket_errno(this->_sock);
							setReceiverError("Receive timeout", NULL,HTTP_TCP_ERROR);
							HLOG(ZQ::common::Log::L_WARNING, CLOGFMT(HttpClient, "recv timeout (%d)"), r);
							this->_errnum = 0;
							return 0;
						}

						continue;
					}
					else //	if (r > 0)
						break;

				} // 2nd for
			} 
#endif
#ifdef WITH_OPENSSL
			if (this->_ssl)
			{ 
				r = SSL_read(this->_ssl, s, (int)n);
				if (r > 0)
					return (size_t)r;
				err = SSL_get_error(this->_ssl, r);
				if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
					return 0;
			}
			else if (this->_bio)
			{ 
				r = BIO_read(this->_bio, s, (int)n);
				if (r > 0)
					return (size_t)r;
				return 0;
			}
			else
#endif
			{ 
#ifndef WITH_LEAN
				if ((this->_omode & HTTP_IO_UDP))
				{ 
					HTTP_SOCKLEN_T k = (HTTP_SOCKLEN_T)sizeof(this->_peer);
					memset((void*)&this->_peer, 0, sizeof(this->_peer));
					r = recvfrom(this->_sock, s, (HTTP_WINSOCKINT)n, this->_socket_flags, (struct sockaddr*)&this->_peer, &k);	// portability note: see HTTP_SOCKLEN_T definition
					this->_peerlen = (size_t)k;
				}
				else
#endif
					r = recv(this->_sock, s, (int)n, this->_socket_flags);

				if (r >= 0)
					return (size_t)r;
				r = socket_errno(this->_sock);
				if (r != HTTP_EINTR && r != HTTP_EAGAIN && r != HTTP_EWOULDBLOCK)
				{ 
					this->_errnum = r;
					return 0;
				}
			}
#ifndef WITH_LEAN
			{			
				struct timeval timeout;
				fd_set fd;
				if (this->_recv_timeout > 0)
				{ 
					timeout.tv_sec = this->_recv_timeout;
					timeout.tv_usec = 0;
				}
				else if (this->_recv_timeout < 0)
				{ 
					timeout.tv_sec = -this->_recv_timeout/1000000;
					timeout.tv_usec = -this->_recv_timeout%1000000;
				}
				else
				{ 
					timeout.tv_sec = 5;
					timeout.tv_usec = 0;
				}
#ifndef ZQ_OS_MSWIN
				if ((int)this->_sock >= (int)FD_SETSIZE)
				{ 
					this->_error = HTTP_FD_EXCEEDED;
					return 0;	// Hint: MUST increase FD_SETSIZE 
				}
#endif
				FD_ZERO(&fd);
				FD_SET(this->_sock, &fd);
#ifdef WITH_OPENSSL
				if (this->_ssl && err == SSL_ERROR_WANT_WRITE)
					r = select((int)this->_sock + 1, NULL, &fd, &fd, &timeout);
				else
					r = select((int)this->_sock + 1, &fd, NULL, &fd, &timeout);
#else
				r = select((int)this->_sock + 1, &fd, NULL, &fd, &timeout);
#endif
				if (!r && this->_recv_timeout)
				{ 
					this->_errnum = 0;
					return 0;
				}
				if (r < 0)
				{ 
					r = socket_errno(this->_sock);
					if (r != HTTP_EINTR && r != HTTP_EAGAIN && r != HTTP_EWOULDBLOCK)
					{ 
						this->_errnum = r;
						return 0;
					}
				}
				if (retries-- <= 0)
				{ 
					this->_errnum = socket_errno(this->_sock);
					return 0;
				}
			}
#endif

		} // 1st for
	}
	r = read(this->_recvfd, s, (unsigned int)n);
	if (r >= 0)
		return (size_t)r;
	this->_errnum = http_errno;
	return 0;
}

#ifndef WITH_LEAN
/******************************************************************************/
void HttpClient::initErrorCode()
{
	int nidx = 0;
#ifdef HOST_NOT_FOUND   
	_error_codes[nidx].code = HOST_NOT_FOUND;
	_error_codes[nidx].errstr = "Host not found";
	nidx ++;
#endif
#ifdef TRY_AGAIN
	_error_codes[nidx].code = TRY_AGAIN;
	_error_codes[nidx].errstr = "Try Again";
	nidx ++;
#endif
#ifdef NO_RECOVERY
	_error_codes[nidx].code = NO_RECOVERY;
	_error_codes[nidx].errstr = "No Recovery";
	nidx ++;
#endif
#ifdef NO_DATA
	_error_codes[nidx].code = NO_DATA;
	_error_codes[nidx].errstr = "No Data";
	nidx ++;
 #endif
#ifdef NO_ADDRESS
	_error_codes[nidx].code = NO_ADDRESS;
	_error_codes[nidx].errstr = "No Address";
	nidx ++;
#endif
	_error_codes[nidx].code = 0;
	_error_codes[nidx].errstr = "";
}
/******************************************************************************/
const char* HttpClient::base642s(const char *s, char *t, size_t l, int *n)
{ 
	register int i, j, c;
	register unsigned long m;
	register const char *p;
	if (!s || !*s)
	{ 
		if (n)
			*n = 0;
		if (this->_error)
			return NULL;
		return HTTP_NON_NULL;
	}
	if (!t)
	{ 
		l = (strlen(s) + 3) / 4 * 3;
		t = (char*)httpMalloc( l);
	}
	if (!t)
	{ 
		this->_error = HTTP_EOM;
		return NULL;
	}
	p = t;
	if (n)
	*n = 0;
	for (;;)
	{ 
		for (i = 0; i < HTTP_BLKLEN; i++)
		{ 
			m = 0;
			j = 0;
			while (j < 4)
			{ 
				c = *s++;
				if (c == '=' || !c)
				{ 
					i *= 3;
					switch (j)
					{ 
					case 2:
						*t++ = (char)((m >> 4) & 0xFF);
						i++;
						break;
					case 3:
						*t++ = (char)((m >> 10) & 0xFF);
						*t++ = (char)((m >> 2) & 0xFF);
						i += 2;
					}
				if (n)
					*n += i;
				return p;
			}
			c -= '+';
			if (c >= 0 && c <= 79)
			{ 
				int b = http_base64i[c];
				if (b >= 64)
				{ 
					this->_error = HTTP_TYPE;
					return NULL;  
				}
				m = (m << 6) + b;
				j++;
			}
			else if (!blank(c + '+'))
			{ 
				this->_error = HTTP_TYPE;
				return NULL;  
			}
			}
			*t++ = (char)((m >> 16) & 0xFF);
			*t++ = (char)((m >> 8) & 0xFF);
			*t++ = (char)(m & 0xFF);
			if (l < 3)
			{ 
				if (n)
					*n += i;
				return p;
			}
			l -= 3;
		}
		if (n)
			*n += 3 * HTTP_BLKLEN;
	}
}

/******************************************************************************/
char* HttpClient::s2base64( const unsigned char *s, char *t, int n)
{ 
	register int i;
	register unsigned long m;
	register char *p;
	if (!t)
		t = (char*)httpMalloc((n + 2) / 3 * 4 + 1);
	if (!t)
	{ 
		this->_error = HTTP_EOM;
		return NULL;
	}
	p = t;
	t[0] = '\0';
	if (!s)
	return p;
	for (; n > 2; n -= 3, s += 3)
	{ 
		m = s[0];
		m = (m << 8) | s[1];
		m = (m << 8) | s[2];
		for (i = 4; i > 0; m >>= 6)
			t[--i] = http_base64o[m & 0x3F];
		t += 4;
	}
	t[0] = '\0';
	if (n > 0)
	{ 
		m = 0;
		for (i = 0; i < n; i++)
			m = (m << 8) | *s++;
		for (; i < 3; i++)
			m <<= 8;
		for (i++; i > 0; m >>= 6)
			t[--i] = http_base64o[m & 0x3F];
		for (i = 3; i > n; i--)
			t[i] = '=';
		t[4] = '\0';
	}
	return p;
}
#endif //ifndef WITH_LEAN

/******************************************************************************/
const char* HttpClient::int2s( int n)
{ 
	return long2s( (long)n);
}

/******************************************************************************/
const char* HttpClient::long2s( long n)
{ 
	sprintf(this->_tmpbuf, "%ld", n);
	return this->_tmpbuf;
}

/******************************************************************************/
void HttpClient::httpDealloc( void *p)
{ 
	if (check_state(this))
		return;
	if (p)
	{
//		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"free all httpMalloc data"));
		register char **q;
		for (q = (char**)&this->_alist; *q; q = *(char***)q)
		{ 
			if (*(short*)(char*)(*q - sizeof(short)) != (short)HTTP_CANARY)
			{
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Data corruption:\n"));
				DBGHEX(TEST, *q - 200, 200);
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "\n"));
				this->_error = HTTP_MOE;
				return;
			}
			if (p == (void*)(*q - *(size_t*)(*q + sizeof(void*))))
			{ 
				*q = **(char***)q;
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Freed data at %p\n", p));
				HTTP_FREE( p);
				return;
			}
		}
	}
	else
	{ 
		register char *q;
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Free all httpMalloc() data\n"));
//		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "free all httpMalloc data"));
		while (this->_alist)
		{ 
			q = (char*)this->_alist;
			if (*(short*)(char*)(q - sizeof(short)) != (short)HTTP_CANARY)
			{
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Data corruption:\n"));
				DBGHEX(TEST, q - 200, 200);
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "\n"));
				this->_error = HTTP_MOE;
				return;
			}
			this->_alist = *(void**)q;
			q -= *(size_t*)(q + sizeof(void*));
			HTTP_FREE( q);
		}
		// we must assume these were deallocated: 
		this->_action = NULL;
		this->_userid = NULL;
		this->_passwd = NULL;
		this->_authrealm = NULL;
		this->_http_content = NULL;
	}
}

/******************************************************************************/
void  HttpClient::begin()
{ 
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Reinitializing context\n"));
	memset(_buf,0,sizeof(_buf));
	
	this->_buflen = 0;
	this->_bufidx = 0;
	this->_totalLenth = 0;
	
	this->_keep_alive = (((this->_imode | this->_omode) & HTTP_IO_KEEPALIVE) != 0);

//	this->_mode = 0;
	this->_count = 0;
	this->_length = 0;
	this->_error = HTTP_OK;
	this->_ahead = 0;
	this->_endpoint[0] = '\0';

}

/******************************************************************************/
void HttpClient::end()
{ 
	if(check_state(this))
		return;

	httpDealloc(NULL);
	httpCloseSock();
#ifdef HTTP_DEBUG
	close_logfiles();
#endif

}

/******************************************************************************/
int HttpClient::tagCmp(const char *s, const char *t)
{ 
	for (;;)
	{ 
		register int c1 = *s;
		register int c2 = *t;
		if (!c1 || c1 == '"')
			break;
		if (c2 != '-')
		{ 
			if (c1 != c2)
			{ 
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
			}
			if (c1 != c2)
			{ 
				if (c2 != '*')
					return 1;
				c2 = *++t;
				if (!c2)
					return 0;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				for (;;)
				{ 
					c1 = *s;
					if (!c1 || c1 == '"')
						break;
					if (c1 >= 'A' && c1 <= 'Z')
						c1 += 'a' - 'A';
					if (c1 == c2 && !tagCmp(s + 1, t + 1))
						return 0;
					s++;
				}
				break;
			}
		}
		s++;
		t++;
	}
	if (*t == '*' && !t[1])
		return 0;
	return *t;
}

/******************************************************************************/
int HttpClient::httpConnect(const char *endpoint, int command)
{
	_bContinue = false;
	if(endpoint == NULL || *endpoint == '\0')
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"httpConnect() error, endpoint is NULL"));
		setError("Endpoint is NULL",NULL,HTTP_ERR);
		return HTTP_ERR;
	}
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"connecting endpoint %s, method id=%d"), endpoint, command);
	return connectCommand(command, endpoint);
}

/******************************************************************************/
int HttpClient::connectCommand(int http_command, const char *endpoint)
{
	this->_keep_alive = ((this->_mode & HTTP_IO_KEEPALIVE) != 0); 

	char host[sizeof(this->_host)];
	int port;
	size_t ncount;
	this->_error = HTTP_OK;
	strcpy(host, this->_host); // save previous host name to compare 
	port = this->_port; // save previous port to compare 
	this->_status = http_command;
	setEndpoint(endpoint);

	if ( *this->_host)
	{ 
		if (!this->_keep_alive || !valid_socket(this->_sock) || strcmp(this->_host, host) || this->_port != port || poll())
		{ 
			this->_keep_alive = 0; // to force close 
			this->_omode &= ~HTTP_IO_UDP; // to force close 
			httpCloseSock();
			DBGLOG(TEST,HTTP_MESSAGE(fdebug, "connect/reconnecting to host='%s' path='%s' port=%d\n", this->_host, this->_path, this->_port));
//			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"connect/reconnecting to host=%s, path=%s, port=%d"),this->_host, this->_path, this->_port);

			this->_sock = tcpConnect(endpoint,this->_host,this->_port);
			if (this->_error)
				return this->_error;
			this->_keep_alive = ((this->_mode & HTTP_IO_KEEPALIVE) != 0);
		}
	}
	ncount = this->_count;
	if (beginSend())
		return this->_error;
	if (http_command != HTTP_POST)
	{ 
		this->_mode &= ~HTTP_IO;
		this->_mode |= HTTP_IO_BUFFER;
	}

	if ((this->_mode & HTTP_IO) != HTTP_IO_STORE && !(this->_mode & HTTP_ENC_XML) && endpoint)
	{ 
		unsigned int k = this->_mode;
		this->_mode &= ~(HTTP_IO | HTTP_ENC_ZLIB);
		if ((k & HTTP_IO) != HTTP_IO_FLUSH)
			this->_mode |= HTTP_IO_BUFFER;
		
		if(http_command != HTTP_POST)
		{
			if ((this->_error = httpPost(endpoint, this->_host, this->_port, this->_path, ncount)))
				return this->_error;
		}

		this->_mode = k;
	}

	return HTTP_OK;
}

/******************************************************************************/
void HttpClient::setEndpoint( const char *endpoint)
{ 
	register const char *s;
	register size_t i, n;
	this->_endpoint[0] = '\0';
	this->_host[0] = '\0';
	this->_path[0] = '/';
	this->_path[1] = '\0';
	this->_port = 80;
	if (!endpoint || !*endpoint)
		return;
#ifdef WITH_OPENSSL
	if (!tagCmp(endpoint, "https:*"))
		this->_port = 443;
#endif
	strncpy(this->_endpoint, endpoint, sizeof(this->_endpoint) - 1);
	this->_endpoint[sizeof(this->_endpoint) - 1] = '\0';
	s = strchr(endpoint, ':');
	if (s && s[1] == '/' && s[2] == '/')
		s += 3;
	else
		s = endpoint;
	n = strlen(s);
	if (n >= sizeof(this->_host))
		n = sizeof(this->_host) - 1;
#ifdef WITH_IPV6
	if (s[0] == '[')
	{ 
		s++;
		for (i = 0; i < n; i++)
		{ 
			if (s[i] == ']')
			{ 
				s++;
				--n;
				break; 
			}
			this->_host[i] = s[i];
		}
	}
	else
	{ 
		for (i = 0; i < n; i++)
		{ 
			this->_host[i] = s[i];
			if (s[i] == '/' || s[i] == ':')
				break; 
		}
	}
	#else
	for (i = 0; i < n; i++)
	{ 
		this->_host[i] = s[i];
		if (s[i] == '/' || s[i] == ':')
			break; 
	}
#endif
	this->_host[i] = '\0';
	if (s[i] == ':')
	{ 
		this->_port = (int)atol(s + i + 1);
		for (i++; i < n; i++)
			if (s[i] == '/')
				break;
	}
	if (i < n && s[i])
	{ 
		strncpy(this->_path, s + i, sizeof(this->_path));
		this->_path[sizeof(this->_path) - 1] = '\0';
	}
}

/******************************************************************************/
int HttpClient::poll()
{ 
#ifndef WITH_LEAN
	struct timeval timeout;
	fd_set rfd, sfd, xfd;
	register int r;
#ifndef ZQ_OS_MSWIN
	if ((int)this->_sock >= (int)FD_SETSIZE)
		return HTTP_FD_EXCEEDED;	// Hint: MUST increase FD_SETSIZE 
#endif
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_ZERO(&sfd);
	FD_ZERO(&xfd);
	if (valid_socket(this->_sock))
	{ 
		FD_SET(this->_sock, &rfd);
		FD_SET(this->_sock, &sfd);
		FD_SET(this->_sock, &xfd);
		r = select((int)this->_sock + 1, &rfd, &sfd, &xfd, &timeout);
		if (r > 0 && FD_ISSET(this->_sock, &xfd))
			r = -1;
	}
	else
		return HTTP_OK;
	if (r > 0)
	{
#ifdef WITH_OPENSSL
		if (this->_imode & HTTP_ENC_SSL)
		{
			if (valid_socket(this->_sock)
			&& FD_ISSET(this->_sock, &sfd)
			&& (!FD_ISSET(this->_sock, &rfd)
			|| SSL_peek(this->_ssl, this->_tmpbuf, 1) > 0))
				return HTTP_OK;
		}
		else
#endif
		if (valid_socket(this->_sock)
		&& FD_ISSET(this->_sock, &sfd)
		&& (!FD_ISSET(this->_sock, &rfd)
        || recv(this->_sock, this->_tmpbuf, 1, MSG_PEEK) > 0))
			return HTTP_OK;
	}
	else if (r < 0)
	{ 
		if ( valid_socket(this->_sock))
		{ 
			setReceiverError(tcpError(), "select failed in http_poll()", HTTP_TCP_ERROR);
			return this->_error = HTTP_TCP_ERROR;
		}
	}
	else
		this->_errnum = 0;
	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "polling: reset by peer on socket=%d select=%d\n", this->_sock, r));
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"poll socket=%d select=%d, reset by peer"),this->_sock, r);
	return HTTP_EOF;
#else
	return HTTP_OK;
#endif
}

/******************************************************************************/
HTTP_SOCKET HttpClient::tcpConnect(const char *endpoint, const char *host, int port)
{
	struct addrinfo hints;
	memset((void*)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_CANONNAME | AI_PASSIVE;
#ifdef WITH_IPV6
	struct addrinfo *res, *ressave;
#endif
	HTTP_SOCKET fd;
	int err = 0;
#ifndef WITH_LEAN
	int retry = 10;
	int len = HTTP_BUFLEN;
	int set = 1;
#endif
	if (valid_socket(this->_sock))
		tcpCloseSocket(this->_sock);
	this->_sock = HTTP_INVALID_SOCKET;
	
	this->_errmode = 0;
#ifdef WITH_IPV6
//	memset((void*)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
#ifndef WITH_LEAN
	if ((this->_omode & HTTP_IO_UDP))
		hints.ai_socktype = SOCK_DGRAM;
	else
#endif
		hints.ai_socktype = SOCK_STREAM;
	this->_errmode = 2;
	if (this->_proxy_host)
		err = getaddrinfo(this->_proxy_host, int2s( this->_proxy_port), &hints, &res);
	else
		err = getaddrinfo(host, int2s( port), &hints, &res);
	if (err)
	{ 
	  setSenderError( HTTP_GAI_STRERROR(err), "getaddrinfo failed in tcpConnect()", HTTP_TCP_ERROR);
	  return HTTP_INVALID_SOCKET;
	}
	ressave = res;
again:
	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	this->_errmode = 0;
#else
#ifndef WITH_LEAN
again:
#endif
#ifndef WITH_LEAN
	if ((this->_omode & HTTP_IO_UDP))
	{
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
	}
	else
#endif
		fd = socket(AF_INET, SOCK_STREAM, 0);
#endif	

	if (!valid_socket(fd))
	{
#ifdef WITH_IPV6
		if (res->ai_next)
		{ 
			res = res->ai_next;
			goto again;
		}
#endif
		this->_errnum = socket_errno(fd);
		setSenderError( tcpError(), "socket failed in tcpConnect()", HTTP_TCP_ERROR);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
	}

	//bind ip port
	if(_strLocalIP.length() > 0)
	{
		char chport[20] = {0};
		sprintf(chport, "%d", _nLocalPort);
		struct addrinfo *pAddrInfo = NULL;
		//convert address
		int rc = getaddrinfo( _strLocalIP.c_str() , chport , &hints , &pAddrInfo);
		if(rc != 0)
		{
			HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "getaddrinfo() failed code[%d] string[%s] localIP[%s] port[%d]"), rc, gai_strerror(rc), _strLocalIP.c_str(), _nLocalPort);
			tcpCloseSocket(fd);
			this->_errnum = rc;
			setSenderError( tcpError(), "getaddrinfo() failed", HTTP_TCP_ERROR);
			return HTTP_INVALID_SOCKET;
		}

		//bind address
		rc = bind( fd, pAddrInfo->ai_addr ,(int)pAddrInfo->ai_addrlen);
		if(rc != 0)
		{
			HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"bind failed code[%d] string[%s] localIP[%s] port[%d]"), SYS::getLastErr(), SYS::getErrorMessage().c_str(), _strLocalIP.c_str(), _nLocalPort);
			tcpCloseSocket(fd);
			freeaddrinfo(pAddrInfo);
			this->_errnum = SYS::getLastErr();
			setSenderError( tcpError(), "socket failed bind", HTTP_TCP_ERROR);
			return HTTP_INVALID_SOCKET;
		}

		struct sockaddr_in sin; 
		socklen_t len = sizeof(sin); 
		int nBindPort = _nLocalPort;
		if (getsockname(fd, (struct sockaddr *)&sin, &len) != -1)   
			nBindPort = ntohs(sin.sin_port); 

		HLOG(ZQ::common::Log::L_INFO, CLOGFMT(HttpClient,"socket[%d]bind localIP[%s] port[%d]successful"),fd,  _strLocalIP.c_str(), nBindPort);

		freeaddrinfo(pAddrInfo);
	}

#ifdef SOCKET_CLOSE_ON_EXEC
#ifdef ZQ_OS_MSWIN
	SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0);
#else
	fcntl(fd, F_SETFD, 1);
#endif
#endif
#ifndef WITH_LEAN

    // set the reuseaddr
    {
        int reuse = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(int)))
        {
            this->_errnum = socket_errno(fd);
            setSenderError( tcpError(), "setsockopt SO_REUSEADDR failed in tcpConnect()", HTTP_TCP_ERROR);
            tcpCloseSocket(fd);			
#ifdef WITH_IPV6
            freeaddrinfo(ressave);
#endif
            return HTTP_INVALID_SOCKET;
        }
    }
    // set the default linger
    { 
        struct linger linger;
        memset((void*)&linger, 0, sizeof(linger));
        linger.l_onoff = 1;
        linger.l_linger = 30;
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(struct linger)))
        { 
            this->_errnum = socket_errno(fd);
            setSenderError( tcpError(), "setsockopt SO_LINGER failed in tcpConnect()", HTTP_TCP_ERROR);
            tcpCloseSocket(fd);			
#ifdef WITH_IPV6
            freeaddrinfo(ressave);
#endif
            return HTTP_INVALID_SOCKET;
        }
    }

	if (this->_connect_flags == SO_LINGER)
	{ 
		struct linger linger;
		memset((void*)&linger, 0, sizeof(linger));
		linger.l_onoff = 1;
		linger.l_linger = 0;
		if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(struct linger)))
		{ 
			this->_errnum = socket_errno(fd);
			setSenderError( tcpError(), "setsockopt SO_LINGER failed in tcpConnect()", HTTP_TCP_ERROR);
			tcpCloseSocket(fd);			
#ifdef WITH_IPV6
			freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
		}
	}
	else if (this->_connect_flags && setsockopt(fd, SOL_SOCKET, this->_connect_flags, (char*)&set, sizeof(int)))
	{ 
		this->_errnum = socket_errno(fd);
		setSenderError(tcpError(), "setsockopt failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket(fd);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
	}
	if ((this->_keep_alive || this->_tcp_keep_alive) && setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&set, sizeof(int)))
	{ 
		this->_errnum = socket_errno(fd);
		setSenderError(tcpError(), "setsockopt SO_KEEPALIVE failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket(fd);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&len, sizeof(int)))
  { 
	  this->_errnum = socket_errno(fd);
	  setSenderError(tcpError(), "setsockopt SO_SNDBUF failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket(fd);
#ifdef WITH_IPV6
    freeaddrinfo(ressave);
#endif
    return HTTP_INVALID_SOCKET;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&len, sizeof(int)))
  { 
	  this->_errnum = socket_errno(fd);
	  setSenderError(tcpError(), "setsockopt SO_RCVBUF failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket(fd);
#ifdef WITH_IPV6
    freeaddrinfo(ressave);
#endif
    return HTTP_INVALID_SOCKET;
  }
#ifdef TCP_KEEPIDLE
  if (this->_tcp_keep_idle && setsockopt((HTTP_SOCKET)fd, IPPROTO_TCP, TCP_KEEPIDLE, (unsigned int*)&(this->_tcp_keep_idle), sizeof(int)))
  { 
	  this->_errnum = socket_errno(fd);
	  setSenderError( tcpError(), "setsockopt TCP_KEEPIDLE failed in tcpConnect()", HTTP_TCP_ERROR);
	tcpCloseSocket((HTTP_SOCKET)fd);
#ifdef WITH_IPV6
    freeaddrinfo(ressave);
#endif
    return HTTP_INVALID_SOCKET;
  }
#endif
#ifdef TCP_KEEPINTVL
	if (this->_tcp_keep_intvl && setsockopt((HTTP_SOCKET)fd, IPPROTO_TCP, TCP_KEEPINTVL, (unsigned int*)&(this->_tcp_keep_intvl), sizeof(int)))
	{ 
		this->_errnum = socket_errno(fd);
		setSenderError(tcpError(), "setsockopt TCP_KEEPINTVL failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket((HTTP_SOCKET)fd);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
	}
#endif
#ifdef TCP_KEEPCNT
	if (this->_tcp_keep_cnt && setsockopt((HTTP_SOCKET)fd, IPPROTO_TCP, TCP_KEEPCNT, (unsigned int*)&(this->_tcp_keep_cnt), sizeof(int)))
	{ 
		this->_errnum = socket_errno(fd);  
		setSenderError( tcpError(), "setsockopt TCP_KEEPCNT failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket((HTTP_SOCKET)fd);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
	}
#endif
#ifdef TCP_NODELAY
	if (!(this->_omode & HTTP_IO_UDP) && setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&set, sizeof(int)))
	{ 
		this->_errnum = socket_errno(fd);   
		setSenderError(tcpError(), "setsockopt TCP_NODELAY failed in tcpConnect()", HTTP_TCP_ERROR);
		tcpCloseSocket(fd);
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return HTTP_INVALID_SOCKET;
	}
#endif
#endif
	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "Opening socket %d to host='%s' port=%d\n", fd, host, port));
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Opening socket[%d] to peer[%s:%d]"),fd,host,port);
#ifndef WITH_IPV6
	this->_peerlen = sizeof(this->_peer);
	memset((void*)&this->_peer, 0, sizeof(this->_peer));
	this->_peer.sin_family = AF_INET;
	this->_errmode = 2;
	if (this->_proxy_host)
	{ 
		if (tcpGetHost(this->_proxy_host, &this->_peer.sin_addr))
		{
			setSenderError(tcpError(), "get proxy host by name failed in tcpConnect()", HTTP_TCP_ERROR);
			tcpCloseSocket(fd);
#ifdef WITH_IPV6
			freeaddrinfo(ressave);
#endif
			return HTTP_INVALID_SOCKET;
		}
		this->_peer.sin_port = htons((short)this->_proxy_port);
	}
	else
	{ 
		if (tcpGetHost(host, &this->_peer.sin_addr))  
		{ 
			setSenderError(tcpError(), "get host by name failed in tcpConnect()", HTTP_TCP_ERROR);
			tcpCloseSocket(fd);
#ifdef WITH_IPV6
			freeaddrinfo(ressave);
#endif
			return HTTP_INVALID_SOCKET;
		}
		this->_peer.sin_port = htons((short)port);
	}
	this->_errmode = 0;
#ifndef WITH_LEAN
	if ((this->_omode & HTTP_IO_UDP))
	{
#ifdef WITH_IPV6
		freeaddrinfo(ressave);
#endif
		return fd;
	}
#endif
#endif
#ifndef WITH_LEAN
	if (this->_connect_timeout)
		HTTP_SOCKNONBLOCK(fd)
	else
		HTTP_SOCKBLOCK(fd)
#endif
	for (;;)
	{
#ifdef WITH_IPV6
		if (connect(fd, res->ai_addr, (int)res->ai_addrlen))
#else
		if (connect(fd, (struct sockaddr*)&this->_peer, sizeof(this->_peer)))
#endif
		{
			err = socket_errno(fd);
#ifndef WITH_LEAN
			if (err == HTTP_EADDRINUSE)
			{
				tcpCloseSocket(fd);
				if (retry-- > 0)
					goto again;
			}
			else if (this->_connect_timeout && (err == HTTP_EINPROGRESS || err == HTTP_EAGAIN || err == HTTP_EWOULDBLOCK))
			{
				HTTP_SOCKLEN_T k;
#ifndef ZQ_OS_MSWIN
				if ((int)this->_sock >= (int)FD_SETSIZE)
				{ 
					this->_error = HTTP_FD_EXCEEDED;
#ifdef WITH_IPV6
					freeaddrinfo(ressave);
#endif
					return HTTP_INVALID_SOCKET;	// Hint: MUST increase FD_SETSIZE 
				}
#endif
				for (;;)
				{ 
					struct timeval timeout;
					fd_set fds;
					register int r;
					if (this->_connect_timeout > 0)
					{ 
						timeout.tv_sec = this->_connect_timeout;
						timeout.tv_usec = 0;
					}
					else
					{ 
						timeout.tv_sec = -this->_connect_timeout/1000000;
						timeout.tv_usec = -this->_connect_timeout%1000000;
					}
					FD_ZERO(&fds);
					FD_SET(fd, &fds);
					r = select((int)fd + 1, NULL, &fds, NULL, &timeout);
					if (r > 0)
						break;
					if (!r)
					{ 
						this->_errnum = 0;
						DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Connect timeout\n"));
						HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http connect time out"));
						setSenderError("Connect Timeout", "connect failed in tcpConnect()", HTTP_TCP_ERROR);
						tcpCloseSocket(fd);
#ifdef WITH_IPV6
						freeaddrinfo(ressave);
#endif
						return HTTP_INVALID_SOCKET;
					}
					r = socket_errno(fd);
					if (r != HTTP_EINTR)
					{ 
						this->_errnum = r;
						DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Could not connect to host\n"));
						HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http could not connect to host: %s"),host);
						setSenderError(tcpError(), "connect failed in tcpConnect()", HTTP_TCP_ERROR);
						tcpCloseSocket(fd);
#ifdef WITH_IPV6
						freeaddrinfo(ressave);
#endif
						return HTTP_INVALID_SOCKET;
					}
				}
				k = (HTTP_SOCKLEN_T)sizeof(this->_errnum);
				if (!getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&this->_errnum, &k) && !this->_errnum)	// portability note: see HTTP_SOCKLEN_T
					break;
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Could not connect to host\n"));
				HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http could not connect to host: %s"),host);
				if (!this->_errnum)
					this->_errnum = socket_errno(fd);
				setSenderError( tcpError(), "connect failed in tcpConnect()", HTTP_TCP_ERROR);
			tcpCloseSocket(fd);
#ifdef WITH_IPV6
			freeaddrinfo(ressave);
#endif
			return HTTP_INVALID_SOCKET;
		}
#endif
#ifdef WITH_IPV6
		if (res->ai_next)
		{ 
			res = res->ai_next;
			tcpCloseSocket(fd);
			goto again;
		}
#endif
		if (err && err != HTTP_EINTR)
		{ 
			this->_errnum = err;
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Could not connect to host\n"));  
			HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http could not connect to host: %s"),host);
			setSenderError(tcpError(), "connect failed in tcpConnect()", HTTP_TCP_ERROR);
			tcpCloseSocket(fd);
#ifdef WITH_IPV6
			freeaddrinfo(ressave);
#endif
			return HTTP_INVALID_SOCKET;
		}
    }  
    else
		break;
	}
#ifdef WITH_IPV6
	this->_peerlen = 0; // IPv6: already connected so use send() 
	freeaddrinfo(ressave);
#endif
#ifndef WITH_LEAN
	if (this->_recv_timeout || this->_send_timeout)
		HTTP_SOCKNONBLOCK(fd)
	else
		HTTP_SOCKBLOCK(fd)
#endif
	this->_sock = fd;
	this->_imode &= ~HTTP_ENC_SSL;
	this->_omode &= ~HTTP_ENC_SSL;
	if (!tagCmp(endpoint, "https:*"))
	{
#ifdef WITH_OPENSSL
		BIO *_bio;
		int r;
		if (this->_proxy_host)
		{ 
			unsigned int k = this->_omode; // make sure we only parse HTTP 
			size_t n = this->_count; // save the content _length 
			char *_userid, *_passwd;
			this->_omode &= ~HTTP_ENC; // mask IO and ENC 
			this->_omode |= HTTP_IO_BUFFER;
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Connecting to %s proxy server\n", this->proxy_http_version));
			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http connect to %s proxy server"),this->proxy_http_version);
			sprintf(this->_tmpbuf, "CONNECT %s:%d HTTP/%s", host, port, this->proxy_http_version);

			if (beginSend()
				|| (this->_error = httpPostHeader(this->_tmpbuf, NULL)))
			{ 
				tcpCloseSocket(fd);
				return HTTP_INVALID_SOCKET;
			}
#ifndef WITH_LEAN
		if (this->_proxy_userid && this->_proxy_passwd && strlen(this->_proxy_userid) + strlen(this->_proxy_passwd) < 761)
		{ 
			sprintf(this->_tmpbuf + 262, "%s:%s", this->_proxy_userid, this->_proxy_passwd);
			strcpy(this->_tmpbuf, "Basic ");
			s2base64( (const unsigned char*)(this->_tmpbuf + 262), this->_tmpbuf + 6, (int)strlen(this->_tmpbuf + 262));

			if ((this->_error = httpPostHeader("Proxy-Authorization", this->_tmpbuf)))
			{ 
				tcpCloseSocket(fd);
				return this->_error;
			}
		}
#endif
		if ((this->_error = httpPostHeader(NULL, NULL))
		|| flush())
		{
			tcpCloseSocket(fd);
			return HTTP_INVALID_SOCKET;
		}
		this->_omode = k;
		k = this->_imode;
		this->_imode &= ~HTTP_ENC; // mask IO and ENC 
		_userid = this->_userid; // preserve 
		_passwd = this->_passwd; // preserve 

		if ((this->_error == httpParse())
		{ 
			tcpCloseSocket(fd);
			return HTTP_INVALID_SOCKET;
		}
		this->_userid = _userid; // restore 
		this->_passwd = _passwd; // restore 
		this->_imode = k; // restore 
		this->_count = n; // restore 
		if (beginSend())
		{ 
			tcpCloseSocket(fd);
			return HTTP_INVALID_SOCKET;
		}
		if (endpoint)
			strncpy(this->_endpoint, endpoint, sizeof(this->_endpoint)-1); // restore 
	}
    if (!this->_ctx && (this->_error = this->fsslauth()))
    { 
		setSenderError( "SSL error", "SSL authentication failed in tcpConnect(): check password, key file, and ca file.", HTTP_SSL_ERROR);
		tcpCloseSocket(fd);
		return HTTP_INVALID_SOCKET;
    }
    this->_ssl = SSL_new(this->_ctx);
    if (!this->_ssl)
    { 
		tcpConnect(fd);
		this->_error = HTTP_SSL_ERROR;
		return HTTP_INVALID_SOCKET;
    }
    if (this->_session)
    { 
		if (!strcmp(this->_session_host, host) && this->_session_port == port)
        SSL_set_session(this->_ssl, this->_session);
		SSL_SESSION_free(this->_session);
		this->_session = NULL;
    }
    this->_imode |= HTTP_ENC_SSL;
    this->_omode |= HTTP_ENC_SSL;
    _bio = BIO_new_socket(fd, BIO_NOCLOSE);
    SSL_set_bio(this->_ssl, _bio, _bio);
#ifndef WITH_LEAN
    // Connect timeout: set SSL sockets to non-blocking 
    if (this->_connect_timeout)
		HTTP_SOCKNONBLOCK(fd)
    else
		HTTP_SOCKBLOCK(fd)
    // Try connecting until success or timeout 
    for (;;)
    { 
		if ((r = SSL_connect(this->_ssl)) <= 0)
		{ 
			int err = SSL_get_error(this->_ssl, r);
			if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
			{ 
				setSenderError(ssl_error( r), "SSL connect failed in tcpConnect()", HTTP_SSL_ERROR);
				tcpCloseSocket(fd);
				return HTTP_INVALID_SOCKET;
			}
			if (this->_connect_timeout)
			{
#ifndef ZQ_OS_MSWIN
				if ((int)this->_sock >= (int)FD_SETSIZE)
				{ 
					this->_error = HTTP_FD_EXCEEDED;
					return HTTP_INVALID_SOCKET;	// Hint: MUST increase FD_SETSIZE 
				}
#endif
				for (;;)
				{ 
					struct timeval timeout;
					fd_set fds;
					register int r;
					if (this->_connect_timeout > 0)
					{ 
						timeout.tv_sec = this->_connect_timeout;
						timeout.tv_usec = 0;
					}
					else
					{ 
						timeout.tv_sec = -this->_connect_timeout/1000000;
						timeout.tv_usec = -this->_connect_timeout%1000000;
					}
					FD_ZERO(&fds);
					FD_SET(fd, &fds);
					r = select((int)fd + 1, &fds, &fds, &fds, &timeout);
					if (r > 0)
						break;
					if (!r)
					{ 
						this->_errnum = 0;
						DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Connect timeout\n"));
						HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http connect timeout"));
						setSenderError( "Connect Timeout", "connect failed in tcpConnect()", HTTP_TCP_ERROR);
						tcpCloseSocket(fd);
						return HTTP_INVALID_SOCKET;
					}
				}
				continue;
			}
		}
		break;
	}
    // Set SSL sockets to nonblocking 
    HTTP_SOCKNONBLOCK(fd)
#endif
    // Check server credentials when required 
    if ((this->_ssl_flags & HTTP_SSL_REQUIRE_SERVER_AUTHENTICATION))
    { 
		int err;
		if ((err = SSL_get_verify_result(this->_ssl)) != X509_V_OK)
		{ 
			setSenderError( X509_verify_cert_error_string(err), "SSL certificate presented by _peer cannot be verified in tcpConnect()", HTTP_SSL_ERROR);
			tcpCloseSocket(fd);
			return HTTP_INVALID_SOCKET;
		}
		if (!(this->_ssl_flags & HTTP_SSL_SKIP_HOST_CHECK))
		{ 
			X509_NAME *subj;
			int ext_count;
			int ok = 0;
			X509 *_peer;
			_peer = SSL_get_peer_certificate(this->_ssl);
			if (!_peer)
			{ 
				setSenderError( "SSL error", "No SSL certificate was presented by the _peer in tcpConnect()", HTTP_SSL_ERROR);
				tcpCloseSocket(fd);
				return HTTP_INVALID_SOCKET;
			}
			ext_count = X509_get_ext_count(_peer);
			if (ext_count > 0)
			{ 
				int i;
				for (i = 0; i < ext_count; i++)
				{ 
					X509_EXTENSION *ext = X509_get_ext(_peer, i);
					const char *ext_str = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
					if (ext_str && !strcmp(ext_str, "subjectAltName"))
					{ 
						X509V3_EXT_METHOD *meth = X509V3_EXT_get(ext);
						void *ext_data;
#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL)
						const unsigned char *data;
#else
						unsigned char *data;
#endif
						STACK_OF(CONF_VALUE) *val;
						int j;
						if (!meth)
							break;
						data = ext->value->data;
#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
					if (meth->it) 
						ext_data = ASN1_item_d2i(NULL, &data, ext->value->_length, ASN1_ITEM_ptr(meth->it));
					else
					{
					// OpenSSL not perfectly portable at this point (?):
					//   Some compilers appear to prefer
					//   meth->d2i(NULL, (const unsigned char**)&data, ...
					//   or
					//   meth->d2i(NULL, &data, ext->value->_length);
				
						ext_data = meth->d2i(NULL, &data, ext->value->_length);
					}
#else
					ext_data = meth->d2i(NULL, &data, ext->value->_length);
#endif
					val = meth->i2v(meth, ext_data, NULL);
					for (j = 0; j < sk_CONF_VALUE_num(val); j++)
					{ 
						CONF_VALUE *nval = sk_CONF_VALUE_value(val, j);
						if (nval && !strcmp(nval->name, "DNS") && !strcmp(nval->value, host))
						{
							ok = 1;
							break;
						}
					}
				}
            if (ok)
				break;
			}
		}
        if (!ok && (subj = X509_get_subject_name(_peer)))
        { 
			int i = -1;
			do
			{ 
				ASN1_STRING *name;
				i = X509_NAME_get_index_by_NID(subj, NID_commonName, i);
				if (i == -1)
					break;
				name = X509_NAME_ENTRY_get_data(X509_NAME_get_entry(subj, i));
				if (name)
				{
					if (!tagCmp(host, (const char*)name))
						ok = 1;
					else
					{ 
						unsigned char *tmp = NULL;
						ASN1_STRING_to_UTF8(&tmp, name);
						if (tmp)
						{ 
							if (!tagCmp(host, (const char*)tmp))
							ok = 1;
							OPENSSL_free(tmp);
						}
					}
				}
			} while (!ok);
		}
		X509_free(_peer);
        if (!ok)
        {
			setSenderError("SSL error", "SSL certificate host name mismatch in tcpConnect()", HTTP_SSL_ERROR);
			tcpCloseSocket(fd);
			return HTTP_INVALID_SOCKET;
        }
	}
    }
#else
	tcpCloseSocket(fd);
    this->_error = HTTP_SSL_ERROR;
    return HTTP_INVALID_SOCKET;
#endif
	}
	//printf localbind ip and port
	{
		struct sockaddr_in sin; 
		socklen_t len = sizeof(sin); 
		int nBindPort  = _nLocalPort;
		std::string bindIp = "0.0.0.0";
		if (getsockname(fd, (struct sockaddr *)&sin, &len) != -1) 
		{
			nBindPort = ntohs(sin.sin_port); 
			bindIp = inet_ntoa(sin.sin_addr);
		}

		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"socket[%d,%s:%d]=>peer[%s:%d] connected for endpoint[%s]"),fd, bindIp.c_str(),nBindPort, host,port, endpoint);
	}
	return fd;
}

/******************************************************************************/
int HttpClient::beginSend()
{ 
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Initializing for output\n"));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http initializing for output"));
	this->_error = HTTP_OK;
	this->_mode = this->_omode | (this->_mode & (HTTP_IO_LENGTH | HTTP_ENC_DIME));

#ifndef WITH_LEAN
	if ((this->_mode & HTTP_IO_UDP))
	{ 
		this->_mode |= HTTP_ENC_XML;
		if (this->_count > HTTP_BUFLEN)
			return this->_error = HTTP_UDP_ERROR;
	}
#endif
	if ((this->_mode & HTTP_IO) == HTTP_IO_FLUSH && valid_socket(this->_sock))
	{ 
		this->_mode |= HTTP_IO_BUFFER;
	}
	this->_mode &= ~HTTP_IO_LENGTH;
	if (!(this->_mode & HTTP_IO_KEEPALIVE))
		this->_keep_alive = 0;

#ifndef WITH_LEANER
	if ((this->_mode & HTTP_ENC_MTOM) && (this->_mode & HTTP_ENC_DIME))
	{ 
		this->_mode |= HTTP_ENC_MIME;
		this->_mode &= ~HTTP_ENC_DIME;
	}
	else
		this->_mode &= ~HTTP_ENC_MTOM;

#	ifdef ZQ_OS_MSWIN
	if (!valid_socket(this->_sock)) // Set ZQ_OS_MSWIN stdout or this->_sendfd to BINARY, e.g. to support DIME 
		_setmode(this->_sendfd, _O_BINARY);

#	endif
#endif
	if (this->_mode & HTTP_IO)
	{ 
		this->_bufidx = 0;
		this->_buflen = 0;
	}
	this->_chunksize = 0;

	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "beginSend() socket[%d] mode=0x%x\n", this->_sock, this->_mode));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"beginSend() socket[%d] mode=0x%x"),this->_sock, this->_mode);
	return HTTP_OK;
}

/******************************************************************************/
char* HttpClient::strdup(const char *s)
{ 
	char *t = NULL;
	if (s && (t = (char*)httpMalloc(strlen(s) + 1)))
		strcpy(t, s);
	return t;
}

/******************************************************************************/
int HttpClient::httpEndSend()
{
	_strContent = "";
	_length = (size_t)-1;
	memset(this->_msgbuf,0,sizeof(this->_msgbuf));
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "End send\n"));
	if(_bDump)
	{
		struct sockaddr_in sin; 
		socklen_t len = sizeof(sin); 
		int nBindPort  = this->_nLocalPort;
		std::string bindIp = "0.0.0.0";
		if (getsockname(this->_sock, (struct sockaddr *)&sin, &len) != -1) 
		{
			nBindPort = ntohs(sin.sin_port); 
			bindIp = inet_ntoa(sin.sin_addr);
		}
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"socket[%d,%s:%d] sent: %s"),this->_sock, bindIp.c_str(), nBindPort, _send_msg.c_str());
	}
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http end send"));
	_send_msg = "";
	if (this->_mode & HTTP_IO) // need to flush the remaining data in buffer 
	{  
		if (flush())
			return this->_error;

#ifndef WITH_LEANER
    if ((this->_mode & HTTP_IO) == HTTP_IO_CHUNK)
    { 
		DBGMSG(SENT, "\r\n0\r\n\r\n", 7);
		if ((this->_error = this->fSend("\r\n0\r\n\r\n", 7)))
			return this->_error;
    }
#endif
	}
#ifdef WITH_TCPFIN
#ifdef WITH_OPENSSL
	if (!this->_ssl && valid_socket(this->_sock) && !this->_keep_alive && !(this->_omode & HTTP_IO_UDP))
		this->fshutdownsocket( this->_sock, 1); // Send TCP FIN 
#else
	if (valid_socket(this->_sock) && !this->_keep_alive && !(this->_omode & HTTP_IO_UDP))
		this->fshutdownsocket( this->_sock, 1); // Send TCP FIN 
#endif
#endif
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "httpEndSend() contentcount=%lu\n",this->_count));
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"socket[%d] sent contentcount=%lu"),this->_sock, this->_count);
	DBGMSG(SENT, "\n\n", 2);
	this->_count = 0;
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpEndRecv()
{
	_bContinue = false;
	_needrecv = 0;
	this->_count = 0;
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http end of receive message, Read %u bytes from socket %d in all"), (unsigned int)this->_totalLenth, this->_sock);
	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpEndRecv()\n"));
	DBGMSG(RECV, "\n\n", 2);

	this->_totalLenth = 0;

#ifdef WITH_ZLIB
	if(this->_error =  httpInflate((char*)_strContent.c_str(),_strContent.length()))
		return this->_error;
	this->_length = _strContent.length();
#endif
	
	httpDealloc();
	
	return HTTP_OK;
}

void HttpClient::setProxy(const char* host, const int port)
{
    _proxy_host = host;
    _proxy_port = port;
}

/******************************************************************************/
const char* HttpClient::codeStr(struct http_code_map* code_map, long code)
{ 
	if (!code_map)
		return NULL;
	while (code_map->code != code && code_map->errstr != "")
		code_map++;
	return code_map->errstr.c_str();
}

/******************************************************************************/
int32 HttpClient::getChunkChar()
{ 
	if (this->_bufidx < this->_buflen)
		return this->_buf[this->_bufidx++];
	this->_bufidx = 0;
	this->_count = this->_buflen = fRecv(this->_buf, HTTP_BUFLEN-1);
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "getChunkChar() read %u bytes from socket %d\n", (unsigned int)this->_buflen, this->_sock));
	DBGMSG(RECV, this->_buf, this->_buflen);
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "getChunkChar() read %u bytes from socket %d"),(unsigned int)this->_buflen, this->_sock);
	if (this->_buflen)
	{
		this->_totalLenth += (unsigned int)this->_buflen;
		return this->_buf[this->_bufidx++];
	}
	return EOF;
}

/******************************************************************************/
int HttpClient::httpPost(const char *endpoint, const char *host, int port, const char *path, size_t ncount)
{ 
	register const char *s;
	register int err;
	this->_count = ncount;
	if (this->_status == HTTP_GET)
		s = "GET";
	else
		s = "POST";

	if (strlen(endpoint) + strlen(this->_http_version) > sizeof(this->_tmpbuf) - 80)
		return this->_error = HTTP_EOM;
	if (this->_proxy_host && tagCmp(endpoint, "https:*"))
		sprintf(this->_tmpbuf, "%s %s HTTP/%s", s, endpoint, this->_http_version);
	else
		sprintf(this->_tmpbuf, "%s /%s HTTP/%s", s, (*path == '/' ? path + 1 : path), this->_http_version);

  	if ((err = httpPostHeader(this->_tmpbuf, NULL)))
		return err;
#ifdef WITH_OPENSSL
	if ((this->_ssl && this->_port != 443) || (!this->_ssl && this->_port != 80))
		sprintf(this->_tmpbuf, "%s:%d", host, port);
	else
		strcpy(this->_tmpbuf, host); 
#else
	if (port != 80)
		sprintf(this->_tmpbuf, "%s:%d", host, port);
	else
		strcpy(this->_tmpbuf, host); 
#endif
	if ((err = httpPostHeader("Host", this->_tmpbuf))
	|| (err = putHttpHdr(HTTP_OK, ncount)))
		return err;
#ifdef WITH_ZLIB
#ifdef WITH_GZIP
	if((err = httpPostHeader("Accept-Encoding", "gzip, deflate")))
#else
	if((err = httpPostHeader("Accept-Encoding", "deflate")))
#endif
		return err;
#endif
#ifndef WITH_LEAN
	if (this->_userid && this->_passwd && strlen(this->_userid) + strlen(this->_passwd) < 761)
	{ 
		sprintf(this->_tmpbuf + 262, "%s:%s", this->_userid, this->_passwd);
		strcpy(this->_tmpbuf, "Basic ");
		s2base64((const unsigned char*)(this->_tmpbuf + 262), this->_tmpbuf + 6, (int)strlen(this->_tmpbuf + 262));
		if ((err = httpPostHeader( "Authorization", this->_tmpbuf)))
		    return err;
	}
	if (this->_proxy_userid && this->_proxy_passwd && strlen(this->_proxy_userid) + strlen(this->_proxy_passwd) < 761)
	{ 
		sprintf(this->_tmpbuf + 262, "%s:%s", this->_proxy_userid, this->_proxy_passwd);
		strcpy(this->_tmpbuf, "Basic ");
		s2base64((const unsigned char*)(this->_tmpbuf + 262), this->_tmpbuf + 6, (int)strlen(this->_tmpbuf + 262));
		if ((err = httpPostHeader("Proxy-Authorization", this->_tmpbuf)))
			return err;
	}
#endif

//additional header
	if(_headmap.size())
	{
		std::map<std::string,std::string>::iterator it;
		for(it = _headmap.begin(); it != _headmap.end(); it++)
		{
			if((err = httpPostHeader(it->first.c_str(),it->second.c_str())))
				return err;
		}
	}
	  return httpPostHeader( NULL, NULL);
}

/******************************************************************************/
int HttpClient::httpPostHeader(const char *key, const char *val)
{ 
	if (key)
	{ 
		if (httpSendHeader( key))
			return this->_error;
		if (val && (sendRaw(": ", 2) || httpSendHeader( val)))
			return this->_error;
	}
	return sendRaw("\r\n", 2);
}

/******************************************************************************/
int HttpClient::httpSendHeader( const char *s)
{ 
	register const char *t;
	do
	{ 
		t = strchr(s, '\n'); // disallow \n in HTTP headers
		if (!t)
			t = s + strlen(s);
		if (sendRaw(s, t - s))
		return this->_error;
		s = t + 1;
	} while (*t);
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::putHttpHdr( int nstatus, size_t ncount)
{ 
	//add date header
	{
		char format[] = {"%a, %d %b %Y %H:%M:%S GMT"};
		char buf[30] = {0};
		time_t tt;
		struct tm* stm;
		time(&tt);
		stm = gmtime(&tt);
		strftime(buf,sizeof(buf),format,stm);
		if((this->_error = httpPostHeader("Date",buf)))
			return this->_error;
	}
	
	if (this->_status != HTTP_GET)
	{ 
		register const char *s = "text/xml; charset=utf-8";
		register int err = HTTP_OK;

		if (nstatus == HTTP_FILE && this->_http_content)
			s = this->_http_content;
		else if (nstatus == HTTP_HTML)
			s = "text/html; charset=utf-8";

//		if (s && (err = httpPostHeader( "Content-Type", s)))
//			return err;
#ifdef WITH_ZLIB
		if (this->_omode & HTTP_ENC_ZLIB)
		{ 
#ifdef WITH_GZIP
			err = httpPostHeader("Content-Encoding", this->_zlib_out == HTTP_ZLIB_DEFLATE ? "deflate" : "gzip");
		#else
			err = httpPostHeader( "Content-Encoding", "deflate");
#endif
		if (err)
			return err;
	}
#endif
#ifndef WITH_LEANER
	if ((this->_omode & HTTP_IO) == HTTP_IO_CHUNK)
		err = httpPostHeader( "Transfer-Encoding", "chunked");
	else
#endif
	if (s)
	{ 
		sprintf(this->_tmpbuf, "%lu", (unsigned long)ncount);
		err = httpPostHeader("Content-Length", this->_tmpbuf);
	}
	if (err)
		return err;
	}
	return httpPostHeader("Connection", this->_keep_alive ? "keep-alive" : "close");
}

/******************************************************************************/
#ifndef WITH_IPV6
int HttpClient::tcpGetHost(const char *addr, struct in_addr *inaddr)
{ 
	int32 iadd = -1;
	struct hostent hostent, *host = &hostent;

	iadd = inet_addr(addr);

	if (iadd != -1)
	{ 
		memcpy(inaddr, &iadd, sizeof(iadd));
		return HTTP_OK;
	}
#if defined(__GLIBC__) || (defined(HAVE_GETHOSTBYNAME_R) && (defined(FREEBSD) || defined(__FreeBSD__)))
	if (gethostbyname_r(addr, &hostent, this->_buf, HTTP_BUFLEN, &host, &this->_errnum) < 0)
		host = NULL;

#elif defined(HAVE_GETHOSTBYNAME_R)
	host = gethostbyname_r(addr, &hostent, this->_buf, HTTP_BUFLEN, &this->_errnum);

#else
	if (!(host = gethostbyname(addr)))
		this->_errnum = h_errno;
#endif
	if (!host)
	{ 
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Host name not found\n"));
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http host name not found"));
		return HTTP_ERR;
	}

	memcpy(inaddr, host->h_addr, host->h_length);

	return HTTP_OK;
}
#endif

/******************************************************************************/
int HttpClient::httpParse()
{ 
	std::string dumpString = "";
	char header[HTTP_HDRLEN], *s;
	unsigned short httpcmd = 0, nstatus = 0, k = 0;
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "waiting for HTTP request/response...\n"));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"client begin parse header"));
	*this->_endpoint = '\0';
	this->_length = (size_t)-1;
	this->_userid = NULL;
	this->_passwd = NULL;
	this->_action = NULL;
	this->_authrealm = NULL;
	this->_proxy_from = NULL;
	this->_http_content = NULL;
	this->_status = 0;
	try
	{
	do
	{ 
		if (getLine(this->_msgbuf, sizeof(this->_msgbuf)))
		{ 
			if (this->_error == HTTP_EOF)
				return HTTP_EOF;
			return this->_error = 414;
		}
		DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() status: %s\n", this->_msgbuf));
//		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "httpParse() status: %s"),this->_msgbuf);
		dumpString += this->_msgbuf  + std::string("  ");
		for (;;)
		{
			if (getLine( header, HTTP_HDRLEN))
			{ 
				if (this->_error == HTTP_EOF)
				{ 
					this->_error = HTTP_OK;
					DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() EOF in HTTP header, continue anyway\n"));
					HLOG(ZQ::common::Log::L_WARNING, CLOGFMT(HttpClient, "httpParse() EOF in Http header,continue parse"));
					break;
				}
				return this->_error;
			}

			if (!*header)
				break;
			DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() header: %s\n", header));
//			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient, "httpParse() header: %s"),header);
			dumpString += header  + std::string("  ");
			s = strchr(header, ':');
			if (s)
			{ 
				char *t;
				*s = '\0';
				do s++;
				while (*s && *s <= 32);
				if (*s == '"')
					s++;
				t = s + strlen(s) - 1;
				while (t > s && *t <= 32)
					t--;
				if (t >= s && *t == '"')
					t--;
				t[1] = '\0';
				if ((this->_error = httpParseHeader(header, s)))
				{ 
					if (this->_error < HTTP_STOP)
						return this->_error;
					nstatus = this->_error;
					this->_error = HTTP_OK;
				}
			}
		}
		if ((s = strchr(this->_msgbuf, ' ')))
		{ 
			k = (unsigned short)strtoul(s, &s, 10);
			if (!blank(*s))
				k = 0;
		}
		else
			k = 0;
	} while (k == 100);
	}
	catch (...)
	{
		DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() failed to parse HTTP header, caught exception(%s)\n", SYS::getErrorMessage().c_str()));
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpParse() failed to parse HTTP header, caught exception(%s)"), SYS::getErrorMessage().c_str());
		this->_error = HTTP_ERR;
		return HTTP_ERR;
	}

	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() finished header parsing, status = %d\n", k));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpParse() finished parse header, status = %d"),k);
	if(_pHttpLog)
		(*_pHttpLog).flush();
	s = strstr(this->_msgbuf, "HTTP/");
	if (s && s[7] != '1')
	{ 
		if (this->_keep_alive == 1)
			this->_keep_alive = 0;
		if (k == 0 && (this->_omode & HTTP_IO) == HTTP_IO_CHUNK) // k == 0 for HTTP request 
		{ 
			this->_imode |= HTTP_IO_CHUNK;
			this->_omode = (this->_omode & ~HTTP_IO) | HTTP_IO_STORE;
		}
	}
	if (this->_keep_alive < 0)
		this->_keep_alive = 1;
	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() keep alive connection = %d\n", this->_keep_alive));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http keep alive connection = %d"),this->_keep_alive);
	if (k == 0)
	{ 
		size_t l = 0;
		if (s)
		{ 
			if (!strncmp(this->_msgbuf, "POST ", l = 5))
				httpcmd = 1;
			else if (!strncmp(this->_msgbuf, "GET ", l = 4))
				httpcmd = 2;
			else if (!strncmp(this->_msgbuf, "PUT ", l = 4))
				httpcmd = 3;
			else if (!strncmp(this->_msgbuf, "DELETE ", l = 7))
				httpcmd = 4;
			else if (!strncmp(this->_msgbuf, "HEAD ", l = 5))
				httpcmd = 5;
		}
		if (s && httpcmd) 
		{ 
			size_t m = strlen(this->_endpoint);
			size_t n = m + (s - this->_msgbuf) - l - 1;
			if (m > n)
				m = n;
			if (n >= sizeof(this->_endpoint))
				n = sizeof(this->_endpoint) - 1;
			strncpy(this->_path, this->_msgbuf + l, n - m);
			this->_path[n - m] = '\0';
			strcat(this->_endpoint, this->_path);
			DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpParse() target endpoint='%s'\n", this->_endpoint));
			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpParse() target endpoint = %s"),this->_endpoint);
			if (httpcmd > 1)
			{ 
				switch (httpcmd)
				{ 
				case  2: this->_error = httpGet(); break;
				case  3: this->_error = httpPut(); break;
				case  4: this->_error = httpDel(); break;
				case  5: this->_error = httpHead(); break;
				default: this->_error = HTTP_METHOD; break;
				}
				if (this->_error == HTTP_OK)
					this->_error = HTTP_STOP; // prevents further processing 
				return this->_error;
			}
			if (nstatus)
				return this->_error = nstatus;
		}
		else if (nstatus)
			return this->_error = nstatus;
		else if (s)
			return this->_error = 405;
	}
	this->_status = k;
  // Status OK (HTTP 200) 
	if (k == 0 || k == 200)
		return HTTP_OK;
  // Status 201 (Created), 202 (Accepted), ... and HTTP 400 and 500 errors.
  //   Only keep parsing HTTP body when content-length>0 or chunked is set.
  
	if (((k > 200 && k <= 299) || k == 400 || k == 500) && ((this->_length > 0  && this->_length != (size_t)-1)|| (this->_imode & HTTP_IO) == HTTP_IO_CHUNK))
		return HTTP_OK;
  // HTTP 400 and 500 headers are supposed to set content-length or chunked.
  //   For those that don't we keep parsing the body only if content type is
  //  given and connection closes.
  
//	if ((k == 400 || k == 500) && (this->_http_content || this->_keep_alive == 0))
//		return HTTP_OK;
//	DBGLOG(TEST,HTTP_MESSAGE(fdebug, "HTTP error %d\n", k));
//	HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http error %d"),k);

//	return setReceiverError("HTTP Error", this->_msgbuf, k);
	//Http not attention the return state(200,300,400,...)

	struct sockaddr_in sin; 
	socklen_t len = sizeof(sin); 
	int nBindPort  = this->_nLocalPort;
	std::string bindIp = "0.0.0.0";
	if (getsockname(this->_sock, (struct sockaddr *)&sin, &len) != -1) 
	{
		nBindPort = ntohs(sin.sin_port); 
		bindIp = inet_ntoa(sin.sin_addr);
	}

	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpParse() socket[%d,%s:%d] received[%d]bytes: %s"), this->_sock, bindIp.c_str(), nBindPort, (unsigned int)this->_totalLenth, dumpString.c_str());
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpParseHeader(const char *key, const char *val)
{
	//add to header map
	if(key && val)
		_headmap[key] = val;

	if (!tagCmp(key, "Host"))
	{ 
#ifdef WITH_OPENSSL
		if (this->_imode & HTTP_ENC_SSL)
			strcpy(this->_endpoint, "https://");
		else
#endif
			strcpy(this->_endpoint, "http://");
		strncat(this->_endpoint, val, sizeof(this->_endpoint) - 8);
		this->_endpoint[sizeof(this->_endpoint) - 1] = '\0';
	}

#ifndef WITH_LEANER
	else if (!tagCmp(key, "Content-Type"))
	{ 
		const char *chaction;
		this->_http_content = strdup(val);
		if (getHeaderAttribute( val, "application/dime"))
			this->_mode |= HTTP_ENC_DIME;
		
		chaction = getHeaderAttribute( val, "action");
		if (chaction)
		{ 
			if (*chaction == '"')
			{ 
				this->_action = strdup( chaction + 1);
				this->_action[strlen(this->_action) - 1] = '\0';
			}
			else
				this->_action = strdup( chaction);
		}
	}
#endif
	else if (!tagCmp(key, "Content-Length"))
	{ 
		this->_length = strtoul(val, NULL, 10);
	}
	else if (!tagCmp(key, "Content-Encoding"))
	{ 
		if (!tagCmp(val, "deflate"))
#ifdef WITH_ZLIB
		this->_zlib_in = HTTP_ZLIB_DEFLATE;
#else
		 return HTTP_ZLIB_ERROR;
#endif
		else if (!tagCmp(val, "gzip"))
#ifdef WITH_GZIP
			this->_zlib_in = HTTP_ZLIB_GZIP;
#else
		return HTTP_ZLIB_ERROR;
#endif
	}
#ifdef WITH_ZLIB
	else if (!tagCmp(key, "Accept-Encoding"))
	{
#ifdef WITH_GZIP
		if (strchr(val, '*') || getHeaderAttribute(val, "gzip"))
			this->_zlib_out = HTTP_ZLIB_GZIP;
		else
#endif
		if (strchr(val, '*') || getHeaderAttribute(val, "deflate"))
			this->_zlib_out = HTTP_ZLIB_DEFLATE;
		else
			this->_zlib_out = HTTP_ZLIB_NONE;
	}
#endif
	else if (!tagCmp(key, "Transfer-Encoding"))
	{ 
		this->_mode &= ~HTTP_IO;
		if (!tagCmp(val, "chunked"))
			this->_mode |= HTTP_IO_CHUNK;
	}
	else if (!tagCmp(key, "Connection"))
	{ 
		if (!tagCmp(val, "keep-alive"))
		{
			this->_keep_alive = 1;
		}
		else if (!tagCmp(val, "close"))
		{
			this->_keep_alive = 0;
//			this->_mode &= ~HTTP_IO_KEEPALIVE;
		}
	}
#ifndef WITH_LEAN
	else if (!tagCmp(key, "Authorization"))
	{ 
		if (!tagCmp(val, "Basic *"))
		{ 
			int n;
			char *s;
			base642s( val + 6, this->_tmpbuf, sizeof(this->_tmpbuf) - 1, &n);
			this->_tmpbuf[n] = '\0';
			if ((s = strchr(this->_tmpbuf, ':')))
			{ 
				*s = '\0';
				this->_userid = strdup(this->_tmpbuf);
				this->_passwd = strdup( s + 1);
			}
		}
	}
	else if (!tagCmp(key, "WWW-Authenticate"))
	{ 
		this->_authrealm = strdup( getHeaderAttribute(val + 6, "realm"));
	}
	else if (!tagCmp(key, "Expect"))
	{ 
		if (!tagCmp(val, "100-continue"))
		{ 
			if ((this->_error = httpPostHeader("HTTP/1.1 100 Continue", NULL))
			|| (this->_error = httpPostHeader( NULL, NULL)))
				return this->_error;
		}
	}
#endif

	else if (!tagCmp(key, "Location"))
	{ 
		strncpy(this->_endpoint, val, sizeof(this->_endpoint));
		this->_endpoint[sizeof(this->_endpoint) - 1] = '\0';
	}
	else if (!tagCmp(key, "X-Forwarded-For"))
	{ 
		this->_proxy_from = strdup( val);
	}

	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpGet()
{ 
	return HTTP_GET_METHOD;
}

/******************************************************************************/
int HttpClient::httpPut()
{ 
	return HTTP_PUT_METHOD;
}

/******************************************************************************/
int HttpClient::httpDel()
{ 
	return HTTP_DEL_METHOD;
}

/******************************************************************************/
int HttpClient::httpHead()
{ 
	return HTTP_HEAD_METHOD;
}

/******************************************************************************/
int HttpClient::getLine(char *s, int len)
{ 
	int i = len;
	int32 c = 0;
	for (;;)
	{ 
		while (--i > 0)
		{ 
			c = httpGetChar();
			if (c == '\r' || c == '\n')
				break;
			if ((int)c == EOF)
				return this->_error = HTTP_EOF;
			*s++ = (char)c;
		}
		if (c != '\n')
			c = httpGetChar(); // got \r or something else, now get \n 
		if (c == '\n')
		{ 
			*s = '\0';
			if (i+1 == len) // empty line: end of HTTP/MIME header 
				break;
			c = get0(this);
			if (c != ' ' && c != '\t') // HTTP line continuation? 
				break;
		}
		else if ((int)c == EOF)
			return this->_error = HTTP_EOF;
	}
	if (i < 0)
		return this->_error = HTTP_HDR;
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpRecv(class HttpClient* httpclient)
{ 
	return recvRaw();
}

/******************************************************************************/
int HttpClient::isxdigit(int c)
{ 
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/******************************************************************************/
int HttpClient::recvRaw()
{ 
	register size_t ret;

	this->_bufidx = 0;
	this->_buflen = ret = fRecv(this->_buf, HTTP_BUFLEN-1);
	this->_totalLenth += ret;
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "recvRaw() read %u bytes from socket %d\n", (unsigned int)ret, this->_sock));
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"recvRaw() read %u bytes from socket %d"),(unsigned int)this->_buflen, this->_sock);
	DBGMSG(RECV, this->_buf, ret);
	
	this->_count = ret;
	return !ret;
}

/******************************************************************************/
int HttpClient::setReceiverError(const char *faultstring, const char *faultdetailXML, int httperror)
{ 
	return setError(faultstring, faultdetailXML, httperror);
}

/******************************************************************************/
int HttpClient::setError(const char *faultstring, const char *faultdetailXML, int nerrornum)
{
	if(faultstring != NULL)
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"setError() %s code = %d"),faultstring,nerrornum);
		this->_errorstr = faultstring;
	}
	else
	{
		HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient, "setError() %s conde = %d"), faultdetailXML, nerrornum);
		this->_errorstr = faultdetailXML;
	}
	
	return this->_error = nerrornum;
}

/******************************************************************************/
const char* HttpClient::getHeaderAttribute(const char *line, const char *key)
{ 
	register const char *s = line;
	if (s)
	{ 
		while (*s)
		{ 
			register short flag;
			s = decodeKey(this->_tmpbuf, sizeof(this->_tmpbuf), s);
			flag = tagCmp(this->_tmpbuf, key);

			s = decodeVal(this->_tmpbuf, sizeof(this->_tmpbuf), s);
			if (!flag)
				return this->_tmpbuf;
		}
  }
  return NULL;
}

/******************************************************************************/
const char* HttpClient::decodeKey(char *_buf, size_t len, const char *val)
{ 
	return decode(_buf, len, val, "=,;");
}

/******************************************************************************/
const char* HttpClient::decodeVal(char *_buf, size_t len, const char *val)
{ 
	if (*val != '=')
	{ 
		*_buf = '\0';
		return val;
	}
	return decode(_buf, len, val + 1, ",;");
}

/******************************************************************************/
const char* HttpClient::decode(char *_buf, size_t len, const char *val, const char *sep)
{ 
	const char *s;
	char *t = _buf;
	for (s = val; *s; s++)
	if (*s != ' ' && *s != '\t' && !strchr(sep, *s))
		break;
	if (*s == '"')
	{ 
		s++;
		while (*s && *s != '"' && --len)
			*t++ = *s++;
	}
	else
	{ 
		while (*s && !blank(*s) && !strchr(sep, *s) && --len)
		{ 
			if (*s == '%')
			{ 
				*t++ = ((s[1] >= 'A' ? (s[1] & 0x7) + 9 : s[1] - '0') << 4)
				+ (s[2] >= 'A' ? (s[2] & 0x7) + 9 : s[2] - '0');
				s += 3;
			}
			else
				*t++ = *s++;
		}
	}
	*t = '\0';
	while (*s && !strchr(sep, *s))
		s++;
	return s;
}

#ifdef HTTP_DEBUG
/******************************************************************************/
void HttpClient::init_logs()
{ 
	int i;
	for (i = 0; i < HTTP_MAXLOGS; i++)
	{ 
		this->logfile[i] = NULL;
		this->fdebug[i] = NULL;
	}
}

/******************************************************************************/
void HttpClient::set_recv_logfile( const char *logfile)
{ 
	set_logfile(HTTP_INDEX_RECV, logfile);
}

/******************************************************************************/
void HttpClient::set_logfile( int i, const char *logfile)
{ 
	const char *s;
	char *t = NULL;
	close_logfile( i);
	s = this->logfile[i];
	this->logfile[i] = logfile;
	if (s)
		HTTP_FREE((void*)s);
	if (logfile)
		if ((t = (char*)HTTP_MALLOC( strlen(logfile) + 1)))
			strcpy(t, logfile);
	this->logfile[i] = t;
}

/******************************************************************************/
void HttpClient::close_logfiles()
{ 
	int i;
	for (i = 0; i < HTTP_MAXLOGS; i++)
		close_logfile( i);
}

/******************************************************************************/
void HttpClient::close_logfile( int i)
{ 
	if (this->fdebug[i])
	{ 
		fclose(this->fdebug[i]);
		this->fdebug[i] = NULL;
	}
}

/******************************************************************************/
void HttpClient::set_sent_logfile( const char *logfile)
{ 
	set_logfile( HTTP_INDEX_SENT, logfile);
}

/******************************************************************************/
void HttpClient::set_test_logfile( const char *logfile)
{ 
	set_logfile( HTTP_INDEX_TEST, logfile);
}

/******************************************************************************/
void HttpClient::open_logfile( int i)
{ 
	if (this->logfile[i])
		this->fdebug[i] = fopen(this->logfile[i], i < 2 ? "ab" : "a");
}
#endif//HTTP_DEBUG

#ifdef WITH_LEAN
/******************************************************************************/
int32 HttpClient::get0(class HttpClient* HttpC)
{ 
	if (HttpC->_bufidx >= HttpC->_buflen && httpRecv(HttpC))
		return EOF;
	return (unsigned char)HttpC->_buf[HttpC->_bufidx];
}

/******************************************************************************/
int32 HttpClient::get1(class HttpClient* HttpC)
{ 
	if (HttpC->_bufidx >= HttpC->_buflen && httpRecv(HttpC))
		return EOF;
	return (unsigned char)HttpC->_buf[HttpC->_bufidx++];
}
#endif //ifdef WITH_LEAN

#ifdef WITH_ZLIB
/******************************************************************************/
int HttpClient::httpDeflate(char* content, size_t len, size_t* compLen)
{
	*compLen = 0;
	////////////////////////test//////////////
//	_mode |= HTTP_ENC_ZLIB;
//	_imode |= HTTP_ENC_ZLIB;
//	_omode |= HTTP_ENC_ZLIB;
	//////////////////////////////////////////
	
	if ((this->_mode & HTTP_ENC_ZLIB) && this->_zlib_state != HTTP_ZLIB_DEFLATE)
	{ 
		if (!this->_z_buf)
			this->_z_buf = (char*)HTTP_MALLOC( _z_buflen);
		this->_d_stream->next_out = (Byte*)this->_z_buf;
		this->_d_stream->avail_out = _z_buflen;
#	ifdef WITH_GZIP
		if (this->_zlib_out != HTTP_ZLIB_DEFLATE)
		{ 
			memcpy(this->_z_buf, "\37\213\10\0\0\0\0\0\0\377", 10);
			//*compLen = 10;
			this->_d_stream->next_out = (Byte*)this->_z_buf + 10;
			this->_d_stream->avail_out = _z_buflen - 10;
			this->_z_crc = crc32(0L, NULL, 0);
			this->_zlib_out = HTTP_ZLIB_GZIP;
			if (deflateInit2(this->_d_stream, this->_z_level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
				return this->_error = HTTP_ZLIB_ERROR;
		}
		else
#	endif
		if (deflateInit(this->_d_stream, this->_z_level) != Z_OK)
			return this->_error = HTTP_ZLIB_ERROR;
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Deflate initialized\n"));
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http deflate initialize"));
		this->_zlib_state = HTTP_ZLIB_DEFLATE;
	}

	if (this->_mode & HTTP_ENC_ZLIB)
	{ 
		this->_d_stream->next_in = (Byte*)content;
		this->_d_stream->avail_in = (unsigned int)len;
#ifdef WITH_GZIP
		this->_z_crc = crc32(this->_z_crc, (Byte*)content, (unsigned int)len);
#endif
		do
		{ 
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "httpDeflate() %u bytes\n", this->_d_stream->avail_in));
			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpDeflate() %u bytes"), this->_d_stream->avail_in);
			if (deflate(this->_d_stream, Z_NO_FLUSH) != Z_OK)
			{ 
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "httpDeflate() unable to deflate: %s\n", this->_d_stream->msg?this->_d_stream->msg:""));
				HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpDeflate() unable to deflate msg:%s"),this->_d_stream->msg?this->_d_stream->msg:"");
				return this->_error = HTTP_ZLIB_ERROR;
			}
			if (!this->_d_stream->avail_out)
			{ 
				return HTTP_ZLIB_ERROR;
			}
		} while (this->_d_stream->avail_in);
	}

	if (this->_mode & HTTP_ENC_ZLIB)
    { 
		int r;
		this->_d_stream->avail_in = 0;
		do
		{
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Deflating remainder\n"));
			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http deflating remainder"));
			r = deflate(this->_d_stream, Z_FINISH);
			if (this->_d_stream->avail_out != _z_buflen)
			{ 
				*compLen += _z_buflen - this->_d_stream->avail_out;
			}
		} while (r == Z_OK);
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Deflated total %lu->%lu bytes\n", this->_d_stream->total_in, this->_d_stream->total_out));
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http deflated total %lu -> %lu bytes"), this->_d_stream->total_in, this->_d_stream->total_out);
		this->_mode &= ~HTTP_ENC_ZLIB;
		this->_zlib_state = HTTP_ZLIB_NONE;
		if (deflateEnd(this->_d_stream) != Z_OK || r != Z_STREAM_END)
		{ 
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Unable to end deflate: %s\n", this->_d_stream->msg?this->_d_stream->msg:""));
			HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"Http unable to end deflate: %s"), this->_d_stream->msg?this->_d_stream->msg:"");
			return this->_error = HTTP_ZLIB_ERROR;
		}
#ifdef WITH_GZIP
		if (this->_zlib_out != HTTP_ZLIB_DEFLATE)
		{
			char gEnd[9] = {0};
			gEnd[0] = this->_z_crc & 0xFF;
			gEnd[1] = (this->_z_crc >> 8) & 0xFF;
			gEnd[2] = (this->_z_crc >> 16) & 0xFF;
			gEnd[3] = (this->_z_crc >> 24) & 0xFF;
			gEnd[4] = this->_d_stream->total_in & 0xFF;
			gEnd[5] = (this->_d_stream->total_in >> 8) & 0xFF;
			gEnd[6] = (this->_d_stream->total_in >> 16) & 0xFF;
			gEnd[7] = (this->_d_stream->total_in >> 24) & 0xFF;

			memcpy(this->_z_buf+(*compLen),gEnd,8);
			*compLen += 8;

			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "gzip crc32=%lu\n", (unsigned long)this->_z_crc));
			HLOG(ZQ::common::Log::L_DEBUG,  CLOGFMT(HttpClient,"httpDeflate() gzip crc=%lu"), (unsigned long)this->_z_crc);
		}
#endif
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpDeflate() ends"));
	}
	
	return HTTP_OK;
}

/******************************************************************************/
int HttpClient::httpInflate(char* content, size_t len)
{	
	size_t ret = 0;

	////////////////////////test////////////
//	this->_mode &= HTTP_ENC_ZLIB;
//	this->_zlib_in = HTTP_ZLIB_GZIP;
//	this->_zlib_out = HTTP_ZLIB_NONE;
//	this->_d_stream->next_in = Z_NULL;
//	this->_d_stream->avail_in = 0;
//	this->_d_stream->next_out = (Byte*)this->_z_buf;
//	this->_d_stream->avail_out = _z_buflen;
	/////////////////////////////////////////////

	//inflate init
    if (this->_zlib_in != HTTP_ZLIB_NONE)
    {
#ifdef WITH_GZIP
		int32 c;
		if (this->_zlib_in != HTTP_ZLIB_DEFLATE)
		{ 
			c = ((unsigned char)content[0]);
			//gzip head
			if (c == 0x1F)
			{ 
				if (getGzipHeader(content+1))
					return this->_error;
				if (inflateInit2(this->_d_stream, -MAX_WBITS) != Z_OK)
					return this->_error = HTTP_ZLIB_ERROR;
				this->_z_crc = crc32(0L, NULL, 0);
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "gzip initialized\n"));
				HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpInflate() inflate gzip initialize"));
			}
			else
			{ 
				if (inflateInit(this->_d_stream) != Z_OK)
					return this->_error = HTTP_ZLIB_ERROR;
				this->_zlib_in = HTTP_ZLIB_DEFLATE;
			}
		}
		else
#endif
		if (inflateInit(this->_d_stream) != Z_OK)
			return this->_error = HTTP_ZLIB_ERROR;
		this->_zlib_state = HTTP_ZLIB_INFLATE;
		DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Inflate initialized\n"));
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpInflate() initialized"));
		this->_mode |= HTTP_ENC_ZLIB;
		if (!this->_z_buf)
			this->_z_buf = (char*)HTTP_MALLOC( _z_buflen);
#ifdef WITH_GZIP
		this->_d_stream->next_in = (Byte*)content + 10;
#else
		this->_d_stream->next_in = (Byte*)content;
#endif
		this->_d_stream->avail_in = len;
		this->_d_stream->next_out = (Byte*)this->_z_buf;
		this->_d_stream->avail_out = _z_buflen;
	}

	if (this->_mode & HTTP_ENC_ZLIB)
	{ 
		if (this->_d_stream->next_out == Z_NULL)
			return EOF;
		if (this->_d_stream->avail_in || !this->_d_stream->avail_out)
		{ 
			register int r;
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Inflating\n"));
			this->_d_stream->next_out = (Byte*)this->_z_buf;
			this->_d_stream->avail_out = _z_buflen;
			r = inflate(this->_d_stream, Z_NO_FLUSH);
			if (r == Z_OK || r == Z_STREAM_END)
			{ 
				this->_bufidx = 0;
				ret = this->_buflen = _z_buflen - this->_d_stream->avail_out;
				if (this->_zlib_in == HTTP_ZLIB_GZIP)
					this->_z_crc = crc32(this->_z_crc, (Byte*)this->_z_buf, (unsigned int)ret);
				if (r == Z_STREAM_END)
				{ 
					DBGLOG(TEST, HTTP_MESSAGE(fdebug, "httpInflate() inflated %lu->%lu bytes\n", this->_d_stream->total_in, this->_d_stream->total_out));
					HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpInflate() inflated %lu -> %lu bytes"),this->_d_stream->total_in, this->_d_stream->total_out);
					this->_d_stream->next_out = Z_NULL;
				}
				if (ret)
				{ 
					DBGLOG(RECV, HTTP_MESSAGE(fdebug, "\n---- decompressed ----\n"));
					DBGMSG(RECV, this->_z_buf, ret);
					DBGLOG(RECV, HTTP_MESSAGE(fdebug, "\n----\n"));
				}
				else
					return HTTP_ZLIB_ERROR;
			}
			else if (r != Z_BUF_ERROR)
			{ 
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "inflate error: %s\n", this->_d_stream->msg?this->_d_stream->msg:""));
				HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"httpInflate() error msg: %s"), this->_d_stream->msg?this->_d_stream->msg:"");
				this->_d_stream->next_out = Z_NULL;
				this->_error = HTTP_ZLIB_ERROR;
				return EOF;
			}
		}
	}

	if (this->_mode & HTTP_ENC_ZLIB)
	{ 
		// Make sure end of compressed content is reached 

		this->_mode &= ~HTTP_ENC_ZLIB;
		this->_bufidx = (char*)this->_d_stream->next_in - content;				
		
		this->_zlib_state = HTTP_ZLIB_NONE;
		if (inflateEnd(this->_d_stream) != Z_OK)
			return this->_error = HTTP_ZLIB_ERROR;
		DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpInflate() end ok\n"));		
#ifdef WITH_GZIP
		char gEnd[9] = {0};
		memcpy(gEnd,content+this->_bufidx,8);
		if (this->_zlib_in == HTTP_ZLIB_GZIP)
		{
			int32 c;
			short i;
			DBGLOG(TEST,HTTP_MESSAGE(fdebug, "httpInflate() gzip crc check\n"));
			for (i = 0; i < 8; i++)
			{ 

				if ((int)(c = gEnd[i]) == EOF)
					return this->_error = HTTP_EOF;
			}
			if (this->_z_crc != ((uLong)(unsigned char)gEnd[0] | ((uLong)(unsigned char)gEnd[1] << 8) | ((uLong)(unsigned char)gEnd[2] << 16) | ((uLong)(unsigned char)gEnd[3] << 24)))
			{ 
				DBGLOG(TEST,HTTP_MESSAGE(fdebug, "gzip error: crc check failed, message corrupted? (crc32=%lu)\n", (unsigned long)this->_z_crc));
				HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"httpInflate() gzip error crc check failed,message corrupted(rcr = %lu)"),(unsigned long)this->_z_crc);
				return this->_error = HTTP_ZLIB_ERROR;
			}
			if (this->_d_stream->total_out != ((uLong)(unsigned char)gEnd[4] | ((uLong)(unsigned char)gEnd[5] << 8) | ((uLong)(unsigned char)gEnd[6] << 16) | ((uLong)(unsigned char)gEnd[7] << 24)))
			{ 
				DBGLOG(TEST,HTTP_MESSAGE(fdebug, "gzip error: incorrect message _length\n"));
				HLOG(ZQ::common::Log::L_ERROR, CLOGFMT(HttpClient,"httpInflate() gzip error, incorrect message _length"));
				return this->_error = HTTP_ZLIB_ERROR;
			}
		}
		this->_zlib_in = HTTP_ZLIB_NONE;
#endif
		HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"httpInflate() inflate end"));
		_strContent = std::string(this->_z_buf,ret);
	}	

	return HTTP_OK;
}

#endif

/******************************************************************************/
#ifdef WITH_GZIP
int HttpClient::getGzipHeader(char* gziphdr)
{ 
	int i;
	int32 c = 0, f = 0;
	int nidx = 0;
	DBGLOG(TEST, HTTP_MESSAGE(fdebug, "Get gzip header\n"));
	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"getGzipHeader()"));
	for (i = 0; i < 9; i++)
	{
		if((int)(c = (unsigned char)gziphdr[nidx++]) == EOF)
			return this->_error = HTTP_EOF;
		if (i == 2)
			f = c;
	}
	if (f & 0x04) /* FEXTRA */
	{ 
		for (i = (unsigned char)gziphdr[nidx++] | ((unsigned char)gziphdr[nidx++] << 8); i; i--)
		{ 
			if ((int)gziphdr[nidx++] == EOF)
				return this->_error = HTTP_EOF;
		}
	}
	if (f & 0x08) /* FNAME */
	{ 
		do
			c = (unsigned char)gziphdr[nidx++];
		while (c && (int)c != EOF);
	}
	if ((int)c != EOF && (f & 0x10)) /* FCOMMENT */
	{ 
		do
			c = (unsigned char)gziphdr[nidx++];
		while (c && (int)c != EOF);
	}
	if ((int)c != EOF && (f & 0x01)) /* FHCRC */
	{ 
		if ((int)(c = (unsigned)gziphdr[nidx++]) != EOF)
			c = (unsigned char)gziphdr[nidx++];
	}
	if ((int)c == EOF)
		return this->_error = HTTP_EOF;
	return HTTP_OK;
}
#endif

/******************************************************************************/
int HttpClient::recvChunk()
{
	_needrecv = 1;
//	HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"Http receive chunk packet"));
	if ((this->_mode & HTTP_IO) == HTTP_IO_CHUNK) // read HTTP chunked transfer 
	{ 
//		for (;;)
//		{
			int32 c;
			char *t, tmp[8];
			t = tmp;
			while (!isxdigit((int)(c = getChunkChar())))
			{ 
				if ((int)c == EOF)
					return this->_ahead = EOF;
			}

			do
				*t++ = (char)c;
			while (isxdigit((int)(c = getChunkChar())) && t - tmp < 7);

			while ((int)c != EOF && c != '\n')
				c = getChunkChar();
			
			if ((int)c == EOF)
				return this->_ahead = EOF;
			*t = '\0';
			DBGLOG(TEST, HTTP_MESSAGE(fdebug, "chunk size = %s (hex)\n", tmp));
			this->_chunksize = strtoul(tmp, &t, 16);
			this->_totalChunkSize += (int64)this->_chunksize;
//			HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"recvChunk() chunk size = [%s](hex) or [%d] (dec)"), tmp, this->_chunksize);
			if (!this->_chunksize)
			{ 
				DBGLOG(TEST, HTTP_MESSAGE(fdebug, "End of chunked message\n"));
				HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"recvChunk() total chunk size = [%lld]"), this->_totalChunkSize);
				HLOG(ZQ::common::Log::L_DEBUG, CLOGFMT(HttpClient,"recvChunk() end of chunk message"));
				while ((int)c != EOF && c != '\n')
					c = getChunkChar();
				_needrecv = 0;
				this->_totalChunkSize = 0;
				this->_ahead = EOF;
				return HTTP_OK;
			}
			//this chunk is received
			if(this->_chunksize + this->_bufidx < this->_count)
			{
				_strContent += std::string(this->_buf+this->_bufidx,this->_chunksize);
				this->_bufidx += this->_chunksize;
				this->_chunksize = 0;				
				
			}
			else
			{
				do
				{
					size_t len = (this->_chunksize + this->_bufidx > this->_count) ? this->_count - this->_bufidx : this->_chunksize;
					_strContent += std::string(this->_buf+this->_bufidx,len);
					this->_chunksize -= len;
					
					this->_bufidx += len;
					if(this->_chunksize <= 0)
						break;
					c = getChunkChar();
					if((int)c == EOF && this->_count == 0)
						return EOF;
					this->_bufidx = 0;
					
				}while(this->_chunksize > 0);				
			}
			//move to next chunk
			while (!isxdigit((int)(c = getChunkChar())))
			{ 
				if ((int)c == EOF)
					return this->_error = EOF;
			}
			_bufidx--;
//		}
	}
	return HTTP_OK;
}

#ifdef ZQ_OS_MSWIN
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
WSA_init::WSA_init()
{
	//-initialize OS socket resources!
	WSADATA w;
	if (WSAStartup(MAKEWORD(2, 2), &w))
	{
		abort();
	}
};

WSA_init::~WSA_init() 
{ 
	WSACleanup(); 
} 
#endif

}}//namespace
