#include "ssllib.h"

#ifndef _NO_LOG_LIB_SUPPORT
#include "Log.h"
#endif

#pragma comment(lib, "ws2_32.lib")

#ifndef _NO_NAMESPACE
namespace ZQ {
#endif

#ifndef _NO_LOG_LIB_SUPPORT
using namespace ZQ::common;
#endif

#define ERR_BIO_BUFF_SIZE		0

//////////////////////////////////////////////////////////////////////////
// global procedure and variant

// for debugging
#ifndef _NOTRACE
#define SSLDBG_PREFIX					"[SSL] "

#define printFault						ssldbg_printFault
#define printError						ssldbg_printError
#define printWarning					ssldbg_printWarning
#define printNotice						ssldbg_printNotice
#define printDebug						ssldbg_printDebug

#ifdef _NO_LOG_LIB_SUPPORT
class _ssldbg_init{
public:
	_ssldbg_init()
	{
		InitializeCriticalSection(&ssldbg_cs);
	}

	~_ssldbg_init()
	{
		DeleteCriticalSection(&ssldbg_cs);
	}

	static CRITICAL_SECTION	ssldbg_cs;
}static _dbg_init;

CRITICAL_SECTION _ssldbg_init::ssldbg_cs;
ostream& ssldbg_ostrm	= cout;

static void ssldbg_setOutStrm(ostream& ostrm)
{
	ssldbg_ostrm = ostrm;
}

#endif

int ssldbg_dbgLevel		= SSLDBG_LEVEL_DEBUG;
int ssldbg_counts[SSLDBG_LEVEL_MAX + 1];

static void ssldbg_printError(char* format, ...);
static void ssldbg_printDebug(char* format, ...);

int ssldbg_setLevel(int level)
{
	if (level >= SSLDBG_LEVEL_MIN && level <= SSLDBG_LEVEL_MAX)
		ssldbg_dbgLevel = level;
	else {
		ssldbg_printError("Invalid level value");
		return -1;
	}
	return ssldbg_dbgLevel;
}

int ssldbg_getLevel()
{
	return ssldbg_dbgLevel;
}

static void ssldbg_resetCounts()
{
	for(int i = SSLDBG_LEVEL_MIN; i <= SSLDBG_LEVEL_MAX; i ++)
		ssldbg_counts[i] = 0;
}

static int ssldbg_getCount(int level)
{
	return ssldbg_counts[level];
}

static void ssldbg_print(int level, char* format, va_list vlist)
{
	int result;
	char buf[1025];

	if (level < SSLDBG_LEVEL_MIN || level > SSLDBG_LEVEL_MAX) {
		ssldbg_printError("ssldbg__print(): Invalid Debugging level");
		return;
	}
	
	ssldbg_counts[level] ++;
	if (ssldbg_dbgLevel < level)
		return;
	
#ifdef _NO_LOG_LIB_SUPPORT
	result = vsprintf(buf, format, vlist);
	static char* levelname[] = {
		"", 
		"Fault:   ", 
		"Error:   ",
		"Warning: ",
		"Notice:  ",
		"Debug:   ",
	};

	EnterCriticalSection(&_dbg_init.ssldbg_cs);
	if (result)
		ssldbg_ostrm << SSLDBG_PREFIX << GetTickCount() << setw(12) << 
			levelname[level] << buf << endl;
	LeaveCriticalSection(&_dbg_init.ssldbg_cs);
#else
	strcpy(buf, SSLDBG_PREFIX);
	result = vsprintf(&buf[sizeof(SSLDBG_PREFIX) - 1], 
		format, vlist);

	Log::loglevel_t logLevel;
	switch (level) {
	case SSLDBG_LEVEL_FAULT:
		logLevel = Log::L_CRIT;
		break;
	case SSLDBG_LEVEL_ERROR:
		logLevel = Log::L_ERROR;
		break;
	case SSLDBG_LEVEL_WARNING:
		logLevel = Log::L_WARNING;
		break;
	case SSLDBG_LEVEL_DEBUG:
		logLevel = Log::L_DEBUG;
		break;
	default:
		logLevel = Log::L_WARNING;
		break;
	}
	if ((&glog) != NULL)
		glog(logLevel, buf);
#endif
}

static int ssldbg_dumpInternalError()
{
	BIO* errin		= NULL;
	BIO* errout		= NULL;
	char buf[1025];

	if (!BIO_new_bio_pair(&errin, ERR_BIO_BUFF_SIZE, 
		&errout, ERR_BIO_BUFF_SIZE)) {
		return 0;
	}

	BIO_flush(errout);
	ERR_print_errors(errout);
	int len = BIO_read(errin, buf, sizeof(buf));
	
	if (len > 0) {
		buf[len] = 0;
		ssldbg_printDebug(buf);
	}

	BIO_free(errin);
	BIO_free(errout);

	return len;
}

static void ssldbg_printFault(char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	ssldbg_print(SSLDBG_LEVEL_FAULT, format, vlist);
}

static void ssldbg_printError(char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	ssldbg_print(SSLDBG_LEVEL_ERROR, format, vlist);
}

static void ssldbg_printWarning(char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	ssldbg_print(SSLDBG_LEVEL_WARNING, format, vlist);
}

static void ssldbg_printNotice(char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	ssldbg_print(SSLDBG_LEVEL_NOTICE, format, vlist);
}

static void ssldbg_printDebug(char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	ssldbg_print(SSLDBG_LEVEL_DEBUG, format, vlist);
}

#else // #ifndef _NOTRACE
#define ssldbg_setOutStrm(o)
#define ssldbg_setLevel(n)			(SSLDBG_LEVEL_MAX + 1)
#define ssldbg_getLevel()			(SSLDBG_LEVEL_MAX + 1)

#define ssldbg_resetCounts()
#define ssldbg_getCount(n)			0
#define ssldbg_print
#define ssldbg_dumpInternalError()	0

#define ssldbg_printFault
#define ssldbg_printError
#define ssldbg_printWarning
#define ssldbg_printDebug

#endif // #ifndef _NOTRACE

int init_ssl_library()
{
	static bool inited = false;
	if (inited)
		return 0;

	SSL_library_init();
	SSL_load_error_strings();
	// ssldbg_setLevel(SSLDBG_LEVEL_DISABLE);
	inited = true;
	return 0;
}

int load_randomess_file(const char* pathname)
{
	int result = RAND_load_file(pathname, MAX_RANDOMNESS_SIZE);
	if (result <= 0) {
		ssldbg_printFault("load_randomess_file(): RAND_load_file() failed");
		ssldbg_dumpInternalError();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////

SSLException::SSLException(int code)
{

}

SSLException::~SSLException()
{

}

int SSLException::getCode()
{
	return _code;
}

string SSLException::getDescription()
{
	return NULL;
}

void SSLException::report(ostream& ostrm)
{

}

//////////////////////////////////////////////////////////////////////////
InetAddr::InetAddr(const char* host, u_short port, short af)
{
	if (!setAddr(host, port, af)) {
		assert(false);
		ssldbg_printError("InetAddr::InetAddr(): setAddr() failed.");
	}
}

InetAddr::InetAddr(sockaddr_in& addr)
{
	_addr4 = addr;
}

InetAddr::InetAddr(sockaddr_in6& addr)
{
	_addr6 = addr;
}

bool InetAddr::setAddr(const char* host, u_short port, short af)
{
	if (af == AF_INET) {
		memset(&_addr4, 0, sizeof(sockaddr_in));
		_addr4.sin_family = AF_INET;
		_addr4.sin_addr.s_addr = inet_addr(host);
		if (_addr4.sin_addr.s_addr == INADDR_NONE) {
			hostent* ht = gethostbyname(host);
			if (ht != NULL, ht->h_addr_list[0] != NULL)
				memcpy(&_addr4.sin_addr.s_addr, ht->h_addr_list[0], ht->h_length);
			else 
				return false;
		}
		_addr4.sin_port = htons(port);
		return true;
	} /* else (af == AF_INET6) {
		return false;
	} */
	return false;
}

//////////////////////////////////////////////////////////////////////////

static int ssl_ctx_pwd_cb(char *buf,int num,int rwflag,void *userdata)
{
	SSLContext* ctx = (SSLContext* )userdata;
	const char* pwd = ctx->_pwd;
	if ((size_t)num < strlen(pwd))
		return 0;
	strcpy(buf, pwd);
	return strlen(pwd);
}

SSLContext::SSLContext(SSLVersion ver /* = VER_TLSv1 */)
{
	_ver = ver;
}
int SSLContext::create(bool isServer /* = true */)
{
	SSL_METHOD* method;
	if (isServer) {
		switch(_ver){
#ifndef OPENSSL_NO_SSL2
		case VER_SSLv2:
			method = (SSL_METHOD *)SSLv2_server_method();
			break;
#endif
		case VER_SSLv3:
			method = (SSL_METHOD *)SSLv3_server_method();
			break;
		case VER_SSLv2v3:
			method = (SSL_METHOD *)SSLv23_server_method();
			break;
		case VER_TLSv1:
			method = (SSL_METHOD *)TLSv1_server_method();
			break;

		default:
			method = (SSL_METHOD *)TLSv1_server_method();
			assert(false);
		}
	} else {
		switch(_ver){
#ifndef OPENSSL_NO_SSL2
		case VER_SSLv2:
			method = (SSL_METHOD *) SSLv2_client_method();
			break;
#endif
		case VER_SSLv3:
			method = (SSL_METHOD *)SSLv3_client_method();
			break;
		case VER_SSLv2v3:
			method = (SSL_METHOD *)SSLv23_client_method();
			break;
		case VER_TLSv1:
			method = (SSL_METHOD *)TLSv1_client_method();
			break;

		default:
			method = (SSL_METHOD *)TLSv1_server_method();
			assert(false);
		}
	}

	_ctx = SSL_CTX_new(method);
	SSL_CTX_set_default_passwd_cb(_ctx, ssl_ctx_pwd_cb);
	SSL_CTX_set_default_passwd_cb_userdata(_ctx, this);
	_ctx->quiet_shutdown = 1;
	_ctx->options |= SSL_OP_ALL;

	return 1;
}

int SSLContext::loadCertificateChain(const char* pathname)
	throw(SSLException)
{
	int result;
	assert(_ctx != NULL);
	result = SSL_CTX_use_certificate_chain_file(_ctx, pathname);
	if (result <= 0) {
		ssldbg_printFault("SSLContext::loadCertificateChain(): "
			"SSL_CTX_use_certificate_chain_file() failed.");
		ssldbg_dumpInternalError();
	}
	return result;
}

int SSLContext::loadCertificate(const char* pathname)
	throw(SSLException)
{
	int result;
	assert(_ctx != NULL);
	result = SSL_CTX_use_certificate_file(_ctx, pathname, SSL_FILETYPE_PEM);
	if (result <= 0) {
		ssldbg_printFault("SSLContext::loadCertificate(): "
			"SSL_CTX_use_certificate_file() failed.");
		ssldbg_dumpInternalError();
	}
	return result;
}


int SSLContext::loadPrivateKeyFile(const char* pathname, const char* password)
	throw(SSLException)
{
	int result;
	assert(_ctx != NULL);
	strcpy(_pwd, password);
	result = SSL_CTX_use_PrivateKey_file(_ctx, pathname, SSL_FILETYPE_PEM);
	if (result <= 0) {
		ssldbg_printFault("SSLContext::loadPrivateKeyFile(): "
			"SSL_CTX_use_PrivateKey_file() failed.");
		ssldbg_dumpInternalError();
	}
	return result;
}

int SSLContext::loadRootCertificates(const char* pathname)
	throw(SSLException)
{
	int result;
	assert(_ctx != NULL);
	result = SSL_CTX_load_verify_locations(_ctx, pathname, NULL);
	if (result <= 0) {
		ssldbg_printFault("SSLContext::loadRootCertificates(): "
			"SSL_CTX_load_verify_locations() failed.");
		ssldbg_dumpInternalError();
	}
	return result;
}

int SSLContext::loadClientCAs(const char* pathname)
{
	assert(_ctx != NULL);
	STACK_OF(X509_NAME)* names = SSL_load_client_CA_file(pathname);
	if (names) {
		SSL_CTX_set_client_CA_list(_ctx, names);
		return 1;
	}
	else
		return 0;
}

int SSLContext::loadDHParams(const char* path)
	throw(SSLException)
{
	DH* ret = NULL;
    BIO* bio;
	assert(_ctx != NULL);

    if ((bio = BIO_new_file(path, "r")) == NULL)
      return SSL_ERR_FALIED;

    ret=PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return SSL_CTX_set_tmp_dh(_ctx,ret);
}

int SSLContext::generateTemporaryRSA(int len /* = 512 */)
{
    RSA *rsa;
	int ret;
	assert(_ctx != NULL);
	
    rsa = RSA_generate_key(len, RSA_F4, NULL, NULL);
	ret = SSL_CTX_set_tmp_rsa(_ctx, rsa);
	if (!ret)
      return 0;
	RSA_free(rsa);
	return ret;
}

int SSLContext::setCipherList(char* ciphers)
{
	int result; 
	assert(_ctx != NULL);
	result = SSL_CTX_set_cipher_list(_ctx, ciphers);
	if (result <= 0) {
		ssldbg_printFault("SSLContext::setCipherList(): "
			"SSL_CTX_set_cipher_list() failed.");
		ssldbg_dumpInternalError();
	}

	return result;
}

bool SSLContext::enableClientVerify(bool b)
{
	int mode;
	assert(_ctx != NULL);
	mode = SSL_CTX_get_verify_mode(_ctx);
	if (b)
		mode |= SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
	else
		mode &= ~(SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE);
	SSL_CTX_set_verify(_ctx, mode, NULL);
	return true;
}

bool SSLContext::isClientVerifyEnabled()
{
	assert(_ctx != NULL);
	int mode = SSL_CTX_get_verify_mode(_ctx);
	return (mode & (SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE)) != 0;
}

//////////////////////////////////////////////////////////////////////////
X509DistName::X509DistName(X509_NAME* name)
{
	assert(name != NULL);
	_name = name;
}
	
int X509DistName::getTextCount() const
{
	assert(_name != NULL);
	return X509_NAME_entry_count(_name);
}

int X509DistName::getText(char* txtId, char* txt, int len) const
{
	assert(_name != NULL);

	_stremp(txt);
	if (X509_NAME_get_text_by_NID(_name, OBJ_txt2nid(txtId), txt, len) == -1)
		return 0;
	return strlen(txt);
}

//////////////////////////////////////////////////////////////////////////

X509Cert::X509Cert(X509* cert)
{
	assert(_cert  != NULL);
	_cert = cert;
}

SerialNumber X509Cert::getSerial() const
{
	assert(_cert  != NULL);
	ASN1_INTEGER* sn = X509_get_serialNumber(_cert);
	if (sn == NULL) {
		ssldbg_printFault("X509Cert::getSerial(): X509_get_serialNumber() failed");
		ssldbg_dumpInternalError();
		return -1;
	}
	return (SerialNumber )ASN1_INTEGER_get(sn);
}

DistinguishedName* X509Cert::getSubjectName() const
{
	assert(_cert  != NULL);
	X509_NAME* name = X509_get_subject_name(_cert);
	if (name == NULL)
		return NULL;
	return new X509DistName(name);
}

DistinguishedName* X509Cert::getIssuerName() const
{
	assert(_cert  != NULL);
	X509_NAME* name = X509_get_issuer_name(_cert);
	if (name == NULL)
		return NULL;
	return new X509DistName(name);
}

bool X509Cert::operator==(Certificate& cert) const
{
	assert(_cert != NULL);
	X509Cert& x509 = (X509Cert& )cert;
	return X509_cmp(_cert, x509._cert) == 0;
}

typedef vector<Certificate* > CertChain;

//////////////////////////////////////////////////////////////////////////

SSLSocket::SSLSocket(SOCKET s /* = INVALID_SOCKET */, 
					 SSLContext* ctx /* = NULL */)
{
	_secure = false;
	_resumeSession = true;
	_clientAuth = false;
	_sock = s;
	strcpy(_testss,"555555555555555");
	_ctx = ctx;
	_ssl = NULL;
	_bio = NULL;
	_session = NULL;
}

SSLSocket::~SSLSocket()
{
	if (_sock != INVALID_SOCKET)
		close();
}

int SSLSocket::attach(SOCKET s)
{
	if (s == INVALID_SOCKET) {
		assert(false);
		ssldbg_printFault("SSLSocket::socket(): invalid argument.");
		return SOCKET_ERROR;
	}

	if (_sock != INVALID_SOCKET) {
		assert(false);
		ssldbg_printFault("SSLSocket::socket(): invalid _sock.");
		return SOCKET_ERROR;
	}

	_sock = s;
	return (int )s;
}

SOCKET SSLSocket::detach()
{
	SOCKET s;
	if (_secure)
		return INVALID_SOCKET;

	s = _sock;
	_sock = INVALID_SOCKET;
	return s;
}

// wrapper of Berkeley socket API
int SSLSocket::socket(int af /* = AF_INET */, 
					  int type /* = SOCK_STREAM */, 
					  int proto /* = IPPROTO_TCP*/ )
{
	if (_sock != INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::socket(): socket has created.");
		assert(false);
		return INVALID_SOCKET;
	}

	_sock = ::socket(af, type, proto);
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::socket(): socket() failed.");
		return SOCKET_ERROR;
	}
	return (int )_sock;
}

int SSLSocket::connect(const sockaddr* addr, 
					   int len, 
					   bool secure /* = true */)
{
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::connect(): invalid _sock");
		return SOCKET_ERROR;
	}
	if (::connect(_sock, addr, len) == SOCKET_ERROR) {
		ssldbg_printFault("SSLSocket::connect(): ::connect() failed");
		return SOCKET_ERROR;
	}
	if (secure) {
		if (s_connect() <= 0) {
			closesocket(_sock);
			_sock = INVALID_SOCKET;
			ssldbg_printFault("SSLSocket::connect(): s_connect() failed");
			return SOCKET_ERROR;
		}
	}
	
	return 0;
}

int SSLSocket::bind(const sockaddr* addr, int len)
{
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::bind(): invalid _sock");
		return SOCKET_ERROR;
	}

	return ::bind(_sock, addr, len);
}

int SSLSocket::send(const void* buf, int len)
{
	int ret;
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::send(): invalid _sock");
		return SOCKET_ERROR;
	}

	if (_secure) {
		if (_ssl == NULL) {
			ssldbg_printFault("SSLSocket::send(): invalid _ssl");
			return SOCKET_ERROR;
		}
		
		ret = SSL_write(_ssl, buf, len);
		if (ret <= 0) {
			ssldbg_printFault("SSLSocket::send(): SSL_write() failed.");
			ssldbg_dumpInternalError();
			return SOCKET_ERROR;
		}

	} else {
		ret = ::send(_sock, (const char* )buf, len, 0);
	}

	return ret;
}

int SSLSocket::recv(void* buf, int len)
{
	int ret;

	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::recv(): invalid _sock");
		return SOCKET_ERROR;
	}

	if (_secure) {
		if (_ssl == NULL) {
			ssldbg_printFault("SSLSocket::recv(): invalid _ssl");
			return SOCKET_ERROR;
		}
		
		ret = SSL_read(_ssl, buf, len);
		if (ret <= 0) {
			ssldbg_printFault("SSLSocket::recv(): SSL_read() failed.");
			ssldbg_dumpInternalError();
			return SOCKET_ERROR;
		}

	} else {
		ret = ::recv(_sock, (char* )buf, len, 0);
	}

	return ret;
}

int SSLSocket::peek(void* buf, int len)
{
	int ret;
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::peek(): invalid _sock");
		return SOCKET_ERROR;
	}

	if (_secure) {
		if (_ssl == NULL) {
			ssldbg_printFault("SSLSocket::peek(): invalid _ssl");
			return SOCKET_ERROR;
		}

		ret = SSL_peek(_ssl, buf, len);
		if (ret <= 0) {
			ssldbg_printFault("SSLSocket::peek(): SSL_read() failed.");
			ssldbg_dumpInternalError();
			return SOCKET_ERROR;
		}

	} else {
		ret = ::recv(_sock, (char* )buf, len, MSG_PEEK);
	}

	return ret;
}

int SSLSocket::listen(int backlog /* = SOMAXCONN */)
{
	return ::listen(_sock, backlog);
}

SSLSocket* SSLSocket::accept(sockaddr* addr /* = NULL */, 
							 int* len /* = NULL */, 
							 bool secure /* = true */)
{
	SOCKET s;
	SSLSocket* ssock = NULL;

	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::accept(): invalid _sock");
		return NULL;
	}
	
	s = ::accept(_sock, addr, len);
	if (s == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::accept(): SSL_read() failed.");
		return NULL;
	}

	ssock = new SSLSocket(s, _ctx);
	if (secure) {
		if (ssock->s_accept() <= 0) {
			delete ssock;
			return NULL;
		}
	}

	return ssock;
}

int SSLSocket::close()
{
	int result;
	if (_sock == INVALID_SOCKET) {
		ssldbg_printWarning("SSLSocket::close(): invalid _sock");
		return SOCKET_ERROR;
	}

	if (_secure) {
		s_shutdown();
	}

	result = closesocket(_sock);
	_sock = INVALID_SOCKET;
	return result;
}

int SSLSocket::setsockopt(int level, int optname, 
						 const char* optval, 
						 int optlen)
{
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::setOption(): invalid _sock");
		return SOCKET_ERROR;
	}

	return ::setsockopt(_sock, level, optname, optval, optlen);
}

int SSLSocket::getsockopt(int level, int optname, char* optval, int* optlen)
{
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::setOption(): invalid _sock");
		return SOCKET_ERROR;
	}

	return ::getsockopt(_sock, level, optname, optval, optlen);
}

int SSLSocket::getpeername(sockaddr* addr, int* len)
{
	return ::getpeername(_sock, addr, len);
}

// Helper functions
int SSLSocket::waitForIncome(const timeval* timeout)
{
	int result;
	char test;
	int errcode;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(_sock, &readfds);
	result = select(_sock + 1, &readfds, NULL, NULL, timeout);
	if (result <= 0 || !_secure)
		return result;
	
	while(true) {
		result = SSL_peek(_ssl, &test, sizeof(test));
		errcode = SSL_get_error(_ssl, result);
		switch(errcode) {
		case SSL_ERROR_NONE:
			return 1;
		case SSL_ERROR_WANT_READ:
			break;
		default:
			ssldbg_printError("SSLSocket::waitForIncome(): SSL_peek falied "
				"(%x)", errcode);
			ssldbg_dumpInternalError();
			return SOCKET_ERROR;
		}

		FD_ZERO(&readfds);
		FD_SET(_sock, &readfds);
		result = select(_sock + 1, &readfds, NULL, NULL, timeout);
	} 

	assert(false);
	return result;
}

int SSLSocket::waitForOutgo(const timeval* timeout)
{
	int result;
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(_sock, &writefds);
	result = select(_sock + 1, NULL, &writefds, NULL, timeout);
	return result;
}

// SSL API
int SSLSocket::buildSSL(SOCKET s)
{
	assert(_bio == NULL);
	assert(_ssl == NULL);
	_bio = BIO_new_socket(s, BIO_NOCLOSE);
	if (_bio == NULL) {
		ssldbg_printFault("SSLSocket::buildSSL(): BIO_new_socket() failed.");
		ssldbg_dumpInternalError();
		return 0;
	}
	_ssl = SSL_new(_ctx->_ctx);
	if (_ssl == NULL) {
		ssldbg_printFault("SSLSocket::buildSSL(): SSL_new() failed.");
		ssldbg_dumpInternalError();
		return 0;
	}
	
	SSL_set_bio(_ssl, _bio, _bio);
	return 1;
}

int SSLSocket::buildCertChain(CertChain& certs)
{
	STACK_OF(X509)* x509s;
	X509* x;
	X509Cert* cert;
	int i;

	certs.clear();
	x509s = SSL_get_peer_cert_chain(_ssl);
	if (x509s == NULL) {
		ssldbg_printError("SSLSocket::buildCertChain(): SSL_get_peer_cert_chain() failed.");
		ssldbg_dumpInternalError();
	}
	for (i = 0; i < sk_X509_num(x509s); i++) {
		x = sk_X509_value(x509s, i);
		cert = new X509Cert(x);
		certs.push_back(cert);
	}

	return i;
}

void SSLSocket::releaseCertChain(CertChain& certs)
{
	CertChain::iterator iter;
	for(iter = certs.begin(); iter != certs.end(); iter ++)
		delete *iter;
	certs.empty();
}

int SSLSocket::s_connect()
{
	int result;
	CertChain certs;
	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::s_connect(): invalid _sock");
		return 0;
	}

	if (_ctx == NULL) {
		ssldbg_printFault("SSLSocket::s_connect(): invalid _ctx");
		return 0;
	}

	if (_secure) {
		ssldbg_printFault("SSLSocket::s_connect(): already be "
			"upgrade to secured connection.");
		return 0;
	}

	if (buildSSL(_sock) <= 0) {
		ssldbg_printFault("SSLSocket::s_connect(): buildSSL() failed.");
		return 0;
	}

	if (_session != NULL) {
		SSL_set_session(_ssl, _session);
	}

	result = SSL_connect(_ssl);
	if (result <= 0) {
		ssldbg_printFault("SSLSocket::s_connect(): SSL_connect() failed.");
		ssldbg_dumpInternalError();
		return result;
	}

	// precheck cert chain;
	result = SSL_get_verify_result(_ssl);
	if (result != X509_V_OK) {
		ssldbg_printWarning("SSLSocket::s_connect(): SSL_get_verify_result() failed.");
		ssldbg_dumpInternalError();
		return result;
	}

	buildCertChain(certs);
	result = checkCertificate(certs);
	releaseCertChain(certs);
	if (!result) {
		if (SSL_shutdown(_ssl) <= 0) {
			ssldbg_printWarning("SSLSocket::s_connect(): SSL_shutdown() failed.");
		}
		
		ssldbg_printError("SSLSocket::s_connect(): checkCertificate() failed.");
		return 0;
	}

	_secure = true;
	return result;
}

int SSLSocket::s_connect(SOCKET s)
{
	int ret;
	if (s != INVALID_SOCKET) {
		assert(false);
		return 0;
	}

	if (_secure) {
		ssldbg_printFault("SSLSocket::s_connect(): already "
			"upgrade to secure connection.");
		return 0;
	}

	_sock = s;
	ret = s_connect();
	if (ret <= 0)
		_sock = INVALID_SOCKET;
	return ret;
}

int SSLSocket::s_accept()
{
	int result;
	CertChain certs;

	if (_sock == INVALID_SOCKET) {
		ssldbg_printFault("SSLSocket::s_accept(): invalid _sock");
		return 0;
	}

	if (_ctx == NULL) {
		ssldbg_printFault("SSLSocket::s_accept(): invalid _ctx");
		return 0;
	}

	if (_secure) {
		ssldbg_printFault("SSLSocket::s_accept(): already "
			"upgrade to secure connection.");
		return 0;
	}

	if (buildSSL(_sock) <= 0) {
		ssldbg_printFault("SSLSocket::s_accept(): buildSSL() failed.");
		return 0;
	}

	result = SSL_accept(_ssl);
	if (result <= 0) {
		ssldbg_printFault("SSLSocket::s_accept(): SSL_accept() failed.");
		ssldbg_dumpInternalError();
		return result;
	}

	if (_clientAuth) {
		// precheck cert chain;
		result = SSL_get_verify_result(_ssl);
		if (result != X509_V_OK) {
			ssldbg_printWarning("SSLSocket::s_connect(): SSL_get_verify_result() failed.");
			ssldbg_dumpInternalError();
			return result;
		}

		buildCertChain(certs);
		result = checkCertificate(certs);
		releaseCertChain(certs);
		if(!result) {
			s_shutdown();
			ssldbg_printWarning("SSLSocket::s_connect(): checkCertificate() failed.");
			return 0;
		}
	}
	
	_secure = true;

	return result;

}

int SSLSocket::s_accept(SOCKET s)
{
	int ret;
	if (s == INVALID_SOCKET) {
		assert(false);
		return 0;
	}
	if (_secure) {
		ssldbg_printFault("SSLSocket::s_accept(): already "
			"upgrade to secure connection.");
		return 0;
	}
	
	_sock = s;
	ret = s_accept();
	if (ret <= 0)
		_sock = INVALID_SOCKET;

	return ret;
}

int SSLSocket::s_shutdown()
{
	int ret;
	if (!_secure) {
		ssldbg_printFault("SSLSocket::s_shutdown(): it haven't "
			"upgrade to secure connection.");
		return 0;
	}

	if (_resumeSession) {
		_session = SSL_get_session(_ssl);
		if (_session == NULL) {
			ssldbg_printWarning("SSLSocket::s_shutdown(): can't get session id.");
		}

	}
	assert(_ssl);
	ret = SSL_shutdown(_ssl);
	if (ret <= 0) {
		ssldbg_printWarning("SSLSocket::s_shutdown(): SSL_shutdown() failed.");
		ssldbg_dumpInternalError();
	}

	SSL_free(_ssl);
	_bio = NULL;
	_ssl = NULL;
	_secure = false;
	return ret;
}

int SSLSocket::s_ctrl(int cmd, long larg, void* parg)
{
	int ret;
	if (!_secure) {
		ssldbg_printFault("SSLSocket::s_shutdown(): it haven't "
			"upgrade to secure connection.");
		return 0;
	}

	assert(_ssl);
	ret = SSL_shutdown(_ssl);
	SSL_free(_ssl);
	return ret;
}

bool SSLSocket::s_setConetext(SSLContext* ctx)
{
	if (ctx == NULL) {
		assert(ctx);
		ssldbg_printFault("SSLSocket::s_setConetext(): invalid ctx");
		return false;
	}

	_ctx = ctx;
	return true;
}

bool SSLSocket::s_setResumeSession(bool resume)
{
	if (_secure) {
		ssldbg_printFault("SSLSocket::s_setResumeSession(): already "
			"upgrade to secure connection.");
		return false;
	}

	_resumeSession = resume;
	return true;
}

bool SSLSocket::setClientAuth(bool auth)
{
	if (_secure) {
		ssldbg_printFault("SSLSocket::s_setResumeSession(): already "
			"upgrade to secure connection.");
		return false;
	}
	
	_clientAuth = auth;
	return true;
}

bool SSLSocket::checkCertificate(const CertChain& certs)
{
#ifdef _DEBUG
	// example

	ssldbg_printDebug("SSLSocket::checkCertificate(): Depth: %d", 
		certs.size());

	char cn[512];
	const Certificate* cert = *(certs.end() - 1);
	DistinguishedName* name = cert->getSubjectName();
	name->getText("CN", cn, sizeof(cn));
	ssldbg_printDebug("checkCertificate(): CN = %s", cn);
	delete name;
	// return strcmp(cn, "SeaChange") == 0;
#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////
int SSLServerSocket::loadDHParams(const char* path)
	throw(SSLException)
{
	DH* ret = NULL;
    BIO* bio;

    if ((bio = BIO_new_file(path, "r")) == NULL)
      return SSL_ERR_FALIED;

    ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return SSL_set_tmp_dh(_ssl, ret);
}

int SSLServerSocket::generateTemporaryRSA(int len /* = 512 */)
{
    RSA *rsa;
	int ret;
    rsa = RSA_generate_key(len, RSA_F4, NULL, NULL);
	ret = SSL_set_tmp_rsa(_ssl, rsa);
	if (!ret)
      return 0;
	RSA_free(rsa);
	return ret;
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	return 1;
}

void SSLServerSocket::enableClientAuth(bool auth)
{
	int mode = SSL_get_verify_mode(_ssl);
	if (auth)
		mode |= SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
	else
		mode &= ~(SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE);

	SSL_set_verify(_ssl, mode, verify_callback);
}

bool SSLServerSocket::isClientAuthEnabled()
{
	long v = SSL_get_verify_mode(_ssl);
	return (v & (SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE)) != 0;
}

int SSLSocket_poll(PollSSLSocket* pollssocks, size_t ssockcount, 
				   const timeval* timeo)
{
	assert(pollssocks);
	if (ssockcount <= 0 ||ssockcount > FD_SETSIZE) {
		assert(false);
		return -1;
	}
	
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	// int rc = 0, wc = 0;

	PollSSLSocket* pollssock;
	SSLSocket* ssock;
	SOCKET fd;

	size_t i;
	for (i = 0; i < ssockcount; i ++) {
		pollssock = &pollssocks[i];
		pollssock->revents = 0;
		
		fd = pollssock->ssock->getSocket();
		if (pollssock->events & SSL_POLLREAD) {
			FD_SET(fd, &readfds);
			// rc ++;
		}

		if (pollssock->events & SSL_POLLWRITE) {
			FD_SET(fd, &writefds);
			// wc ++;
		}

		FD_SET(fd, &exceptfds);
	}

	int result = select(FD_SETSIZE, &readfds, &writefds, &exceptfds, 
		timeo);
	if (result <= 0)
		return result;

	int errcode;
	char test;
	// check for read
	size_t n = 0;
	for (i = 0; i < ssockcount; i ++) {
		pollssock = &pollssocks[i];
		ssock = pollssock->ssock;
		fd = ssock->getSocket();

		if (FD_ISSET(fd, &exceptfds)) {
			pollssock->revents = SSL_POLLEXCEPTION;
		} else {
			if (pollssock->events & SSL_POLLREAD) {
				if (FD_ISSET(fd, &readfds) ) {
					if (ssock->_secure) {
						result = SSL_peek(ssock->_ssl, &test, sizeof(test));
						errcode = SSL_get_error(ssock->_ssl, result);
						if (errcode == SSL_ERROR_NONE) {
							pollssock->revents |= SSL_POLLREAD;
							n ++;
						} else if (errcode != SSL_ERROR_WANT_READ) {
							pollssock->revents = SSL_POLLEXCEPTION;
							n = -1;
							break;
						}
					} else {
						pollssock->revents |= SSL_POLLREAD;
						n ++;
					}
				}
			}

			if (pollssock->events & SSL_POLLWRITE) {
				if (FD_ISSET(fd, &writefds) ) {
					pollssock->revents |= SSL_POLLWRITE;
					n ++;
				}
			}
		}
	}

	return n;
}

#ifndef _NO_NAMESPACE
}
#endif
