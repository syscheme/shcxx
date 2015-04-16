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
// Desc  : Define a simple http client-side
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/HttpClient.h $
// 
// 3     1/26/15 11:33a Zhiqiang.niu
// add http proxy supported
// 
// 2     5/27/14 12:01p Hongquan.zhang
// add a new method for fetching body content
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 25    10-10-12 16:26 Haoyuan.lu
// 
// 24    10-09-08 15:24 Hui.shao
// added file-desc
// ===========================================================================

#ifndef	__ZQ_COMMON_HttpClient_H__
#define	__ZQ_COMMON_HttpClient_H__

#include "ZQ_common_conf.h"
#include "Log.h"
#include "FileLog.h"

#include <map>
#include <string>

#ifndef _THREAD_SAFE
# define _THREAD_SAFE
#endif

#ifdef WITH_LEANER
# ifndef WITH_LEAN
#  define WITH_LEAN
# endif
#endif

#if !defined ZQ_OS_MSWIN
# if defined(FREEBSD) || defined(__FreeBSD__)
# include <xlocale.h>
# elif defined(__GNU__)
# include <xlocale.h>
# else
// Default asumptions on supported functions 
#  define HAVE_GETHOSTBYNAME_R
# endif
#endif

#if defined(__cplusplus) && !defined(WITH_LEAN)
# include <iostream>
#endif

// Suggestion when HTTP_FD_EXCEEDED error occurs:
//   Some systems allow increasing FD_SETSIZE before including sys/types.h:
//#define FD_SETSIZE (2048)

#ifndef WITH_NOIO
#   include <errno.h>
#   include <sys/types.h>
#	include <time.h>
#endif

#ifndef ZQ_OS_MSWIN
#	include <sys/socket.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>          // TCP_NODELAY
#	include <arpa/inet.h>
#endif

#ifdef ZQ_OS_MSWIN
# define HTTP_WINSOCKINT int
#else
# define HTTP_WINSOCKINT size_t
#endif

#ifdef ZQ_OS_MSWIN
#  include <io.h>
#  include <fcntl.h>
# ifdef WITH_IPV6
#  include <winsock2.h> // Visual Studio 2005 users: you must install the Platform SDK (R2)
#  include <ws2tcpip.h>
#  include <wspiapi.h>
#  define HTTP_GAI_STRERROR gai_strerrorA
# else
//#  include <winsock.h> // Visual Studio 2005 users: you must install the Platform SDK (R2)
# include <winsock2.h>   //Alternative: use winsock2 (not available with eVC)
# endif
#else
#   include <netdb.h>
#   include <netinet/in.h>
#   include <unistd.h>
#   include <fcntl.h>
#endif

#ifdef WITH_OPENSSL
# define OPENSSL_NO_KRB5
# include <openssl/bio.h>
# include <openssl/err.h>
# include <openssl/rand.h>
# include <openssl/ssl.h>
# include <openssl/x509v3.h>
# ifndef ALLOW_OLD_VERSIONS
#  if (OPENSSL_VERSION_NUMBER < 0x00905100L)
#   error "Must use OpenSSL 0.9.6 or later"
#  endif
# endif
#endif

#ifdef WITH_GZIP
# ifndef WITH_ZLIB
#  define WITH_ZLIB
# endif
#endif

#ifdef WITH_ZLIB
# include <zlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Portability: define HTTP_SOCKLEN_T

#if defined(FREEBSD) || defined(__FreeBSD__)
# define HTTP_SOCKLEN_T socklen_t
#elif defined(ZQ_OS_MSWIN)
# define HTTP_SOCKLEN_T int
#else
# define HTTP_SOCKLEN_T socklen_t
#endif

#ifndef HTTP_SOCKET
# ifdef ZQ_OS_MSWIN
#  define HTTP_SOCKET SOCKET
#  define http_closesocket(n) closesocket(n)
# else
#  define HTTP_SOCKET int
#  define http_closesocket(n) close(n)
# endif
#endif

#define HTTP_INVALID_SOCKET ((HTTP_SOCKET)-1)
#define valid_socket(n) ((n) != HTTP_INVALID_SOCKET)

#ifndef HTTP_GAI_STRERROR
# define HTTP_GAI_STRERROR gai_strerror
#endif

#ifndef FD_SETSIZE
# define FD_SETSIZE (1024)
#endif

#ifdef ZQ_OS_MSWIN
# define HTTP_EINTR WSAEINTR
# define HTTP_EAGAIN WSAEWOULDBLOCK
# define HTTP_EWOULDBLOCK WSAEWOULDBLOCK
# define HTTP_EINPROGRESS WSAEINPROGRESS
# define HTTP_EADDRINUSE WSAEADDRINUSE
#else
# define HTTP_EINTR EINTR
# define HTTP_EAGAIN EAGAIN
# define HTTP_EADDRINUSE EADDRINUSE
# define HTTP_EWOULDBLOCK EWOULDBLOCK
# define HTTP_EINPROGRESS EINPROGRESS
#endif

#ifdef ZQ_OS_MSWIN
#  define http_errno GetLastError()
#  define socket_errno(s) WSAGetLastError()
#else
# ifndef WITH_NOIO
#  define http_errno errno
#   ifndef socket_errno
#       define socket_errno(s) errno
#   endif
# else
#  define http_errno 0
#   ifndef socket_errno
#       define socket_errno(s) 0
#   endif
# endif
#endif

#ifndef HTTP_BUFLEN
# ifndef WITH_LEAN
#  define HTTP_BUFLEN  (65536) // buffer length for socket packets, also used by gethostbyname_r and UDP messages, so don't make this too small 
# else
#  define HTTP_BUFLEN  (2048)
# endif
#endif
#ifndef HTTP_BLKLEN
# ifndef WITH_LEAN
#  define HTTP_BLKLEN   (256) // size of blocks to collect long strings and XML attributes */
# else
#  define HTTP_BLKLEN    (32)
# endif
#endif
#ifndef HTTP_TAGLEN
# ifndef WITH_LEAN
#  define HTTP_TAGLEN  (1024) // maximum length of XML element tag/attribute name or host/path name + 1 
# else
#  define HTTP_TAGLEN    (64)
# endif
#endif
#ifndef HTTP_HDRLEN
# ifndef WITH_LEAN
#  define HTTP_HDRLEN  (8192) // maximum length of HTTP header line (must be >4096 to read cookies) 
# else
#  define HTTP_HDRLEN  (1024)
# endif
#endif

#ifndef HTTP_MAXLOGS
# define HTTP_MAXLOGS	  (3) // max number of debug logs per struct http environment
# define HTTP_INDEX_RECV  (0)
# define HTTP_INDEX_SENT  (1)
# define HTTP_INDEX_TEST  (2)
#endif

#define HTTP_SSL_DEFAULT			(HTTP_SSL_REQUIRE_SERVER_AUTHENTICATION | HTTP_SSLv3_TLSv1)

// state 
#define HTTP_INIT	1
#define HTTP_COPY	2
#define check_state(http) (!(http) || ((http)->_state != HTTP_INIT && (http)->_state != HTTP_COPY))

// DEBUG macros
#ifndef WITH_LEAN
# ifdef DEBUG
#  ifndef HTTP_DEBUG
#   define HTTP_DEBUG
#  endif
# endif
#endif

#ifndef HTTP_MALLOC			// use libc malloc
# define HTTP_MALLOC(size) malloc(size)
#endif

#ifndef HTTP_FREE			// use libc free 
# define HTTP_FREE(ptr) free(ptr)
#endif


#if defined(__cplusplus) && !defined(WITH_LEAN)
}
extern "C" {
#endif
namespace ZQ{
namespace common{

class ZQ_COMMON_API HttpClient;
class ZQ_COMMON_API WSA_init;
	
/******************************************************************************/
class HttpClient  
{
public:
	HttpClient(ZQ::common::Log* pLog = NULL, char* pLocalIP = NULL, int nPort = 0);
	//Destructor frees  data
	virtual ~HttpClient();

	//Init HttpClient,if want persistent connection set iomode = HTTP_IO_KEEPALIVE
	void  init(int iomode = HTTP_IO_DEFAULT);	
	//Uninit HttpClient(close socket, destroy and free memory)
	void uninit();

	void setLog(Log* pLog);
	//set http client bind ip and port
	void setLocalHost(std::string& strLocalIP, int nPort = 0);
	//Connect server
	int  httpConnect(const char* endpoint, int command = HTTP_POST);	
	
	//Send HTTP content to the server
	int  httpSendContent(const char* content, size_t contentLen);
	//End send HTTP data
	int  httpEndSend();

	//Receive HTTP data
	int  httpBeginRecv();
	int	 httpContinueRecv();
	int  httpEndRecv();		

    //Add proxy
    void setProxy(const char* host, const int port);

public:
	//Get HTTP response start line
	char* getMsg();
	//Whether HTTP receive is EOF,true is EOF
	bool isEOF();

	//Get HTTP response header area
	std::map<std::string,std::string> getHeader();
	//Set HTTP request header area,if key is NULL that is clear header,else update or add header
	void setHeader(char* key,char* val);
	//Set range header,the range from nBegin to nEnd,if nBegin or nEnd less zero,it is not specify the begin or end point
	bool setRangeHeader(int nBegin, int nEnd = -1);
	//Get the total count of range request,unit is byte
	bool getRangeHeader(int& nTotal);
	
	//Return Http status code example(200 ,404,500)
	int getStatusCode();
	//Set keep alive or not keep alive,true is keep alive, false is not
	void setKeepAlive(bool bKeep = true);

	//Get response content
	bool getContent(char* buf, size_t bufLen);
	size_t getBodyData( char* buf, size_t bufLen );
	void getContent(std::string& strContent);
	//Get response content length
	size_t getContentLength();

	//Get error code
	int getErrorcode();
	//Get error string
	const char* getErrorstr();

	//Set connect timeout ,when parameter timeout > 0, gives socket connect timeout in seconds, < 0 in usec, = 0 is block mode
	void setConnectTimeout(int timeout);
	//Set send timeout ,when parameter timeout > 0, gives socket connect timeout in seconds, < 0 in usec
	void setSendTimeout(int timeout);
	//Set receive timeout ,when parameter timeout > 0, gives socket connect timeout in seconds, < 0 in usec
	void setRecvTimeout(int timeout);

	// Set whether dump when send 
	void setDump(bool flag) { _bDump = flag; };
public:	
	//Code number and string
	struct http_code_map
	{ 
		long code;
		std::string errstr;
	};
	
private:
	int  httpCloseSock();
	int  httpDeflate(char* content, size_t len, size_t* compLen);
	int  httpInflate(char* content, size_t len);
	void setEndpoint(const char* endpoint);

	int  closesock();
	void done();

	int  connectCommand(int command, const char* endpoint);	

	int  beginSend();
	int  recvRaw();
	int  httpRecv(class HttpClient* );
	int  recvChunk();
	
	int  sendRaw(const char*, size_t);
	int  flushRaw(const char*, size_t);	
	int  flush();
	int32  httpGetChar();	

	char*  strdup(const char*);
	
	const char*  codeStr(struct http_code_map*, long);

private:
	void  begin();	
	void end();
	
	void*  httpMalloc(size_t);
	void httpDealloc( void* p = NULL);

	const char*  int2s(int);
	const char* long2s( long);	
	int isxdigit(int);
private:	
#ifdef HTTP_DEBUG
	void init_logs();
	void close_logfile( int);
	void close_logfiles();
	void set_logfile( int, const char*);
	void  set_recv_logfile( const char*);
	void  set_sent_logfile( const char*);
	void  set_test_logfile( const char*);
	void  open_logfile( int);
#endif
		
	int  tagCmp(const char*, const char*);
	int  setSenderError(const char*, const char*, int);	
	int  setError(const char*, const char*, int);
	int  setReceiverError(const char*, const char*, int);	


	int  putHttpHdr(int status, size_t ncount);
	int32 getChunkChar();
	int httpPost(const char*, const char*, int, const char*, size_t);
	int httpGet();	
	int httpPut();
	int httpDel();
	int httpHead();
	int httpSendHeader(const char*);
	int httpPostHeader(const char*, const char*);
	int httpParse();
	int httpParseHeader(const char*, const char*);
	int getLine( char*, int);

#ifndef WITH_LEAN
	void initErrorCode();
	char*  s2base64(const unsigned char*, char*, int);
	const char*  base642s( const char*, char*, size_t, int*);	 
#endif
	
	int  poll();
	int  fSend(const char*, size_t);
	size_t fRecv(char*, size_t);
	const char *tcpError();
#ifndef WITH_IPV6
	int tcpGetHost(const char *addr, struct in_addr *inaddr);
#endif
	HTTP_SOCKET tcpConnect(const char *endpoint, const char *host, int port);
	int tcpDisconnect();
	int tcpCloseSocket(HTTP_SOCKET);
	int tcpShutDownSocket(HTTP_SOCKET, int);
	const char* httpStrError();
	 
 private:

	const char* getHeaderAttribute(const char *line, const char *key);
	const char* decodeKey(char *buf, size_t len, const char *val);
	const char* decodeVal(char *buf, size_t len, const char *val);
	const char* decode(char *buf, size_t len, const char *val, const char *sep);

private:

#ifndef WITH_LEAN
	# define get0(http) (((http)->_bufidx>=(http)->_buflen && httpRecv(http)) ? EOF : (unsigned char)(http)->_buf[(http)->_bufidx])
	# define get1(http) (((http)->_bufidx>=(http)->_buflen && httpRecv(http)) ? EOF : (unsigned char)(http)->_buf[(http)->_bufidx++])
#else
	int32 get0(class HttpClient*);
	int32 get1(class HttpClient*);
#endif

#ifdef WITH_GZIP
	int getGzipHeader(char* gziphdr);
#endif

public:
	typedef enum ErrCode
	{
		HTTP_EOF		 = EOF,
		HTTP_ERR		 = EOF,
		HTTP_OK			 = 0,
		HTTP_TYPE,
		HTTP_NO_DATA,	
		HTTP_GET_METHOD,	
		HTTP_PUT_METHOD,	
		HTTP_DEL_METHOD,	
		HTTP_HEAD_METHOD,
		HTTP_METHOD,
		HTTP_EOM,
		HTTP_MOE,
		HTTP_HDR,
		HTTP_UDP_ERROR,
		HTTP_TCP_ERROR,
		HTTP_SSL_ERROR,
		HTTP_ZLIB_ERROR,
		HTTP_FD_EXCEEDED
	} ErrCode;

	enum HttpStatus
	{		
		HTTP_STOP	= 1000,	// No HTTP response
		HTTP_FORM,			//Form request/response 
		HTTP_HTML,			//Custom HTML response
		HTTP_FILE			// Custom file-based response
	};

	enum HttpMethod
	{
		HTTP_POST	= 2000,
		HTTP_GET
	};

	enum Zlib
	{
		HTTP_ZLIB_NONE		= 0x00,
		HTTP_ZLIB_DEFLATE	= 0x01,
		HTTP_ZLIB_INFLATE	= 0x02,
		HTTP_ZLIB_GZIP		= 0x02
	};

	// transport, connection, and content encoding modes
	enum IOMode
	{
		HTTP_IO				= 0x00000003,	// IO mask 
		HTTP_IO_FLUSH		= 0x00000000,	// flush output immediately, no buffering 
		HTTP_IO_BUFFER		= 0x00000001,	// buffer output in packets of size HTTP_BUFLEN 
		HTTP_IO_STORE		= 0x00000002,	// store entire output to determine length for transport 
		HTTP_IO_CHUNK		= 0x00000003,	// use HTTP chunked transfer AND buffer packets 
		HTTP_IO_UDP			= 0x00000004,	// TCP or UDP 
		HTTP_IO_LENGTH		= 0x00000008,	// calc message length (internal) 
		HTTP_IO_KEEPALIVE	= 0x00000010,	// keep connection alive 
		HTTP_IO_DEFAULT		= HTTP_IO_FLUSH
	};

	enum Enc
	{
		HTTP_ENC			= 0x00000FFF,	// IO and ENC mask
		HTTP_ENC_XML		= 0x00000040,
		HTTP_ENC_DIME		= 0x00000080,
		HTTP_ENC_MIME		= 0x00000100,
		HTTP_ENC_MTOM		= 0x00000200,
		HTTP_ENC_ZLIB		= 0x00000400,
		HTTP_ENC_SSL		= 0x00000800		
	};

	// SSL client/server authentication settings
	enum SSLSet
	{
		HTTP_SSL_REQUIRE_SERVER_AUTHENTICATION		= 0x01,	// client requires server to authenticate 
		HTTP_SSL_SKIP_HOST_CHECK					= 0x04	// client does not check the common name of the host in certificate 
	};

private:
	bool		_bContinue;	//if true should receive other data
	bool        _bDump; //if dump when send
	std::string _send_msg;
	
	short		_state;			// 0 = uninitialized, 1 = initialized, 2 = copy of another HttpClient class
	int32		_mode;
	int32		_imode;
	int32		_omode;
	const char	*_http_version;	// HTTP version used "1.0" or "1.1"
	const char	*_http_content;	// optional custom response content type 
	int			_recv_timeout;		// when > 0, gives socket recv timeout in seconds, < 0 in usec 
	int			_send_timeout;		// when > 0, gives socket send timeout in seconds, < 0 in usec 
	int			_connect_timeout;		// when > 0, gives socket connect() timeout in seconds, < 0 in usec 
	int			_accept_timeout;		// when > 0, gives socket accept() timeout in seconds, < 0 in usec
	int			_socket_flags;		// socket recv() and send() flags, e.g. set to MSG_NOSIGNAL to disable sigpipe 
	int			_connect_flags;		// connect() SOL_SOCKET sockopt flags, e.g. set to SO_DEBUG to debug socket 
	
	void		*_alist;			// memory allocation (malloc) list 
	char		*_userid;			// HTTP Basic authorization userid 
	char		*_passwd;			// HTTP Basic authorization passwd 

	HTTP_SOCKET _sock;
	
	std::string	_strLocalIP;
	int			_nLocalPort;

	int _sendfd;
	int _recvfd;

	int			_needrecv;//need to recv bytes
	size_t		_bufidx;	// index in buf[] 
	size_t		_buflen;	// length of buf[] content 
	int32		_ahead;	// parser lookahead 
	size_t		_count;		// message length counter 
	size_t		_length;	// message length as set by HTTP header 
	size_t      _totalLenth; // all bytes received to simple log
	
	char	_buf[HTTP_BUFLEN];// send and receive buffer
	char	_msgbuf[1024];	// in/output buffer for messages >=1024 bytes
	std::map<std::string,std::string> _headmap;
	std::string	_strContent;
	char	_tmpbuf[1024];	// in/output buffer for HTTP/MIME headers, simpleType values, attribute names, and DIME must be >=1024 bytes

	size_t	_chunksize;
	int64	_totalChunkSize;
	
	char	_endpoint[HTTP_TAGLEN];
	char	_path[HTTP_TAGLEN];
	char	_host[HTTP_TAGLEN];
	char	*_action;
	char	*_authrealm;		// HTTP authentication realm 
	int		_port;			// port number 
	short	_keep_alive;		// connection should be kept open 
	short	_tcp_keep_alive;		// enable SO_KEEPALIVE 
	unsigned int _tcp_keep_idle; 	// set TCP_KEEPIDLE
	unsigned int _tcp_keep_intvl; 	// set TCP_KEEPINTVL 
	unsigned int _tcp_keep_cnt; 	// set TCP_KEEPCNT
	const char *_proxy_http_version;// HTTP version of proxy "1.0" or "1.1"
	const char *_proxy_host;	// Proxy Server host name
	int		_proxy_port;		// Proxy Server port (default = 8080) 
	const char *_proxy_userid;	// Proxy Authorization user name 
	const char *_proxy_passwd;	// Proxy Authorization password 
	const char *_proxy_from;	// X-Forwarding-For header returned by proxy 
	int		_status;			// -1 when request, else error code to be returned by server 
	int		_error;	
	std::string _errorstr;//error string 
	int		_errmode;
	int		_errnum;
	
	ZQ::common::Log* _pHttpLog;
#ifndef WITH_LEAN
	//error code array
	struct http_code_map _error_codes[7];
#endif
	
#if !defined(WITH_LEAN) || defined(HTTP_DEBUG)
	const char *logfile[HTTP_MAXLOGS];
	FILE *fdebug[HTTP_MAXLOGS];
#endif

#ifdef WITH_IPV6
	struct sockaddr_storage _peer;	// IPv6: set by http_accept and by UDP recv 
#else
	struct sockaddr_in _peer;	// IPv4: set by httpConnect/http_accept and by UDP recv
#endif

	size_t _peerlen;
	
#ifdef WITH_OPENSSL
	int (*fsslauth)(class HttpClient*);
	int (*fsslverify)(int, X509_STORE_CTX*);
	BIO*			_bio;
	SSL*			_ssl;
	SSL_CTX*		_ctx;
	SSL_SESSION*	_session;
	
	unsigned short _ssl_flags;
	char	_session_host[HTTP_TAGLEN];
	int		_session_port;
#endif

#ifdef WITH_ZLIB
	z_stream*	_d_stream;		// decompression stream 
	uLong		_z_crc;			// internal gzip crc 

	short		_zlib_state;		// HTTP_ZLIB_NONE, HTTP_ZLIB_DEFLATE, or HTTP_ZLIB_INFLATE 
	short		_zlib_in;		// HTTP_ZLIB_NONE, HTTP_ZLIB_DEFLATE, or HTTP_ZLIB_GZIP 
	short		_zlib_out;		// HTTP_ZLIB_NONE, HTTP_ZLIB_DEFLATE, or HTTP_ZLIB_GZIP 
	char*		_z_buf;			// buffer 
	size_t		_z_buflen;
	unsigned short _z_level;	// compression level to be used (0=none, 1=fast to 9=best) 
#endif

};

//class for WSAStartup and WSACleanup
#ifdef ZQ_OS_MSWIN
class WSA_init
{
public:
	WSA_init();
	~WSA_init();
};
static WSA_init wsa;
#endif

}}//namespace
#ifdef __cplusplus
} // extern "C" 
#endif

#endif// __ZQ_COMMON_HttpClient_H__
