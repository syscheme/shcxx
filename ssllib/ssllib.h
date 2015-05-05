#ifndef _SLL_LIBRARY_H_
#define _SLL_LIBRARY_H_

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

#ifdef WIN32
#include <winsock2.h>
#endif

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>

using namespace std;

#ifndef _NO_NAMESPACE
namespace ZQ {
#endif

//////////////////////////////////////////////////////////////////////////
// Error Code

#define SSL_ERR_OK			0
#define SSL_ERR_FALIED		-1
#define SSL_ERR_UNKNOWN		-2

//////////////////////////////////////////////////////////////////////////

#define MAX_RANDOMNESS_SIZE		(1024 * 1024)

//////////////////////////////////////////////////////////////////////////
// Global procedure
int init_ssl_library();	
int load_randomess_file(const char* pathname);

// void _stremp(char* str);
#define _stremp(_str)		_str[0] = 0
typedef u_long SerialNumber;

// for debugging
#define SSLDBG_LEVEL_MIN				SSLDBG_LEVEL_DISABLE
#define SSLDBG_LEVEL_DISABLE			0
#define SSLDBG_LEVEL_FAULT				1
#define SSLDBG_LEVEL_ERROR				2
#define SSLDBG_LEVEL_WARNING			3
#define SSLDBG_LEVEL_NOTICE				4
#define SSLDBG_LEVEL_DEBUG				5
#define SSLDBG_LEVEL_MAX				SSLDBG_LEVEL_DEBUG

int ssldbg_setLevel(int level);
int ssldbg_getLevel();

//////////////////////////////////////////////////////////////////////////

class SSLException {
public:
	SSLException(int code);
	virtual ~SSLException();

	int getCode();
	string getDescription();
	virtual void report(ostream& ostrm);

private:
	int		_code;
};

//////////////////////////////////////////////////////////////////////////

#ifndef SOCKADDR_IN6_DEFINED
	#define SOCKADDR_IN6_DEFINED

	struct in6_addr {
		union {
			u_char	Byte[16];
			u_short	Word[8];
			u_long	DWord[4];
			u_int64	QWord[2];
		} u;
	};

	struct sockaddr_in6 {
			short   sin6_family;
			u_short sin6_port;
			u_long  sin6_flowinfo;
			struct  in6_addr sin6_addr;
			u_long  sin6_scope_id;
	};
	
	struct sockaddr_in6_old {
			short   sin6_family;        
			u_short sin6_port;          
			u_long  sin6_flowinfo;      
			struct  in6_addr sin6_addr;  
	};
#endif // #ifndef SOCKADDR_IN6_DEFINED

class InetAddr {
public:	
	InetAddr(const char* host, u_short port, short af);
	InetAddr(sockaddr_in& addr);
	InetAddr(sockaddr_in6& addr);

	inline sockaddr* getSockAddr() { return &_addr; }
	inline int getAddrLen() 
	{
		if ( _addr.sa_family == AF_INET)
			return sizeof (_addr4);
		else if (_addr.sa_family == AF_INET6)
			return sizeof (_addr6);
		else {
			assert(false);
			return AF_UNKNOWN1;
		}
	}

	inline operator sockaddr*() {
		return &_addr;
	}

	inline short getAddrFamily() { return _addr.sa_family; }

	bool setAddr(const char* host, u_short port, short af);

protected:
	union {
		sockaddr			_addr;
		sockaddr_in			_addr4;
		sockaddr_in6		_addr6;
		// sockaddr_in6_old	_addr6_old;
	};
};

//////////////////////////////////////////////////////////////////////////

typedef enum _SSLVersion{
	VER_SSLv2, 
	VER_SSLv3, 
	VER_SSLv2v3, 
	VER_TLSv1
}SSLVersion;

class SSLContext {
	friend class SSLSocket;
	friend int ssl_ctx_pwd_cb(char *buf,int num,int rwflag,void *userdata);
	
public:
	SSLContext(SSLVersion ver = VER_TLSv1);
	int create(bool isServer = true);

	int loadCertificate(const char* pathname)
		throw(SSLException);

	int loadCertificateChain(const char* pathname)
		throw(SSLException);

	int loadPrivateKeyFile(const char* pathname, const char* password)
		throw(SSLException);

	int loadRootCertificates(const char* pathname)
		throw(SSLException);

	int loadClientCAs(const char* pathname);

	int loadDHParams(const char* path)
		throw(SSLException);

	int generateTemporaryRSA(int len = 512);

	int setCipherList(char* ciphers);

	bool enableClientVerify(bool b);
	bool isClientVerifyEnabled();

protected:
	SSL_CTX*	_ctx;
	SSLVersion	_ver;
	
private:
	char		_pwd[64];
};

class DistinguishedName {
public:
	virtual int getTextCount() const = 0;
	virtual int getText(char* txtId, char* txt, int len) const = 0;
};

class Certificate {
public:
	virtual SerialNumber getSerial() const = 0;
	virtual DistinguishedName* getSubjectName() const = 0;
	virtual DistinguishedName* getIssuerName() const = 0;
	virtual bool operator==(Certificate& cert) const = 0;
};

class X509DistName: public DistinguishedName {
	friend class X509Cert;
protected:
	X509DistName(X509_NAME* name);
	
public:	
	virtual int getTextCount() const;
	virtual int getText(char* txtId, char* txt, int len) const;
	
protected:
	X509_NAME*	_name;
};

class X509Cert: public Certificate {
	friend class X509DistName;
	friend class SSLSocket;
protected:
	X509Cert(X509* cert);
public:
	virtual SerialNumber getSerial() const;
	virtual DistinguishedName* getSubjectName() const;
	virtual DistinguishedName* getIssuerName() const;
	virtual bool operator==(Certificate& cert) const;

protected:
	X509*	_cert;
};

typedef vector<Certificate* > CertChain;
struct PollSSLSocket;

class SSLSocket {
	friend class X509Cert;
	friend int SSLSocket_poll(PollSSLSocket* pollssocks, size_t ssockcount, 
		const timeval* timeo);

public:
	SSLSocket(SOCKET s = INVALID_SOCKET, SSLContext* ctx = NULL);
	virtual ~SSLSocket();
	
	int attach(SOCKET s);
	SOCKET detach();
	
	// wrapper of Berkeley socket API
	int socket(int af = AF_INET, int type = SOCK_STREAM, 
		int proto = IPPROTO_TCP);
	int connect(const sockaddr* addr, int len, bool secure = true);
	int bind(const sockaddr* addr, int len);
	int send(const void* buf, int len);
	int recv(void* buf, int len);
	int peek(void* buf, int len);
	int listen(int backlog = SOMAXCONN);
	SSLSocket* accept(sockaddr* addr = NULL, int* len = NULL, 
		bool secure = true);
	int close();
	int setsockopt(int level, int optname, const char* optval, 
		int optlen);
	int getsockopt(int level, int optname, char* optval, int* optlen);
	int getpeername(sockaddr* addr, int* len);

	// helper methods
	int waitForIncome(const timeval* timeout);
	int waitForOutgo(const timeval* timeout);
	
	// SSL API
	int s_connect();
	int s_connect(SOCKET s);
	int s_accept();
	int s_accept(SOCKET s);
	int s_shutdown();
	int s_ctrl(int cmd, long larg, void* parg);
	
	bool s_setConetext(SSLContext* ctx);
	inline SSLContext* s_getConetext()	{ return _ctx; }

	bool s_setResumeSession(bool resume);
	bool s_getResumeSession()			{ return _resumeSession; }
	
	bool setClientAuth(bool auth);
	bool s_getClientAuth()				{ return _clientAuth; }
	
	
	// propertys
	inline bool isSecure() { return _secure; }
	inline SOCKET getSocket() { return _sock; }
	
protected:
	virtual bool checkCertificate(const CertChain& certs);
	int buildSSL(SOCKET s);
	int buildCertChain(CertChain& certs);
	void releaseCertChain(CertChain& certs);
protected:
	bool			_secure;
	bool			_resumeSession;
	bool			_clientAuth;
	SOCKET			_sock;
	char			_testss[16];
	SSLContext*		_ctx;
	SSL*			_ssl;
	BIO*			_bio;
	SSL_SESSION*	_session;
};

class SSLServerSocket : public SSLSocket {

public:
	SSLServerSocket(SSLContext ctx);
	SSLServerSocket(SSLContext& ctx, SOCKET s);
	SSLServerSocket(SSLContext& ctx, const char* host, u_short port);
	~SSLServerSocket();

	int loadDHParams(const char* path)
		throw(SSLException);

	int generateTemporaryRSA(int len = 512);

	void enableClientAuth(bool auth);
	bool isClientAuthEnabled();	
	
};

typedef SSLSocket SSLClientSocket;
#define	INFTIM					-1
#define SSL_POLLREAD			0x0001
#define SSL_POLLWRITE			0x0002
#define SSL_POLLEXCEPTION		0X0004

#define INIT_POLLSSOCK(p, s, e) \
	do { \
		(p)->ssock = s;	\
		(p)->events = e; \
		(p)->revents = 0; \
	} while(0)

struct PollSSLSocket {
	SSLSocket*		ssock;
	short			events;
	short			revents;
};

int SSLSocket_poll(PollSSLSocket* pollssocks, size_t ssockcount, 
				   const timeval* timeo);

#ifndef _NO_NAMESPACE
}
#endif

#endif // #ifndef _SLL_LIBRARY_H_
