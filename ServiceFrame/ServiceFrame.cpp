#include "debug.h"
#include "ServiceFrame.h"

#ifndef _NO_LOG_LIB_SUPPORT
#include "Log.h"
#endif

#include "Locks.h"

#define SF_RECV_TIMEOUT			10
static uint32 sf_max_polltime = 200;

#define MS2TV(tv, ms)					\
	do { \
		(tv).tv_sec = (ms) / 1000; \
		(tv).tv_usec = ((ms) % 1000) * 1000; \
	} while(0)
	
#ifndef _NO_NAMESPACE
namespace ZQ {
#endif // #ifndef _NO_NAMESPACE

#ifndef _NO_LOG_LIB_SUPPORT
using namespace ZQ::common;
#endif

#ifdef _DEBUG
#define new			debug_new
#endif

//////////////////////////////////////////////////////////////////////////
// global
//

// trace supprot route.
#ifndef _NOTRACE
#define SVCDBG_PREFIX				"[SrvFrm] "
int svcdbg_print(int level, char* format, va_list vlist);

#ifdef _NO_LOG_LIB_SUPPORT
class _svcdbg_init{
public:
	_svcdbg_init()
	{
		InitializeCriticalSection(&svcdbg_cs);
	}

	~_svcdbg_init()
	{
		DeleteCriticalSection(&svcdbg_cs);
	}

	static CRITICAL_SECTION	svcdbg_cs;
}static _dbg_init;

CRITICAL_SECTION _svcdbg_init::svcdbg_cs;
static ostream&	svcdbg_outStrm		= cout;

void svcdbg_setDebugOutStream(ostream& ostrm)
{
	svcdbg_outStrm = ostrm;
}

#endif

static int		svcdbg_level = SVCDBG_LEVEL_DEBUG;
static int		svcdbg_counts[SVCDBG_LEVEL_MAX + 1];

int svcdbg_setLevel(int level)
{
	if (level >= SVCDBG_LEVEL_MIN && level <= SVCDBG_LEVEL_MAX)
		svcdbg_level = level;
	else {
		svcdbg_printError("Invalid level value");
		return -1;
	}
	return svcdbg_level;
}

int svcdbg_getLevel()
{
	return svcdbg_level;
}

void svcdbg_resetCounts()
{
	for(int i = SVCDBG_LEVEL_MIN; i <= SVCDBG_LEVEL_MAX; i ++)
		svcdbg_counts[i] = 0;
}

int svcdbg_getCount(int level)
{
	return svcdbg_counts[level];
}

int svcdbg_print(int level, const char* format, va_list vlist)
{
	int result;
	char buf[1025];

	if (level < SVCDBG_LEVEL_MIN || level > SVCDBG_LEVEL_MAX) {
		svcdbg_printError("SvcDebug::_print(): Invalid Debugging level");
		return 0;
	}
	
	svcdbg_counts[level] ++;
	if (svcdbg_level < level)
		return 0;
#ifdef _NO_LOG_LIB_SUPPORT

	result = vsprintf(buf, format, vlist);
	static char* levelname[] = {
		"", 
		"Fault:   ", 
		"Error:   ",
		"Warning: ",
		"Notice	  ", 
		"Debug:   ",
	};

	EnterCriticalSection(&_dbg_init.svcdbg_cs);
	if (result)
		svcdbg_outStrm << SVCDBG_PREFIX << GetTickCount() << setw(12) << 
			levelname[level] << buf << endl;
	LeaveCriticalSection(&_dbg_init.svcdbg_cs);
#else
	strcpy(buf, SVCDBG_PREFIX);
	result = vsprintf(&buf[sizeof(SVCDBG_PREFIX) - 1], 
		format, vlist);
	int iLens =strlen(buf);
	if(buf[iLens-1] == '\n')
		buf[iLens-1] = '\0';

	Log::loglevel_t logLevel;
	switch (level) 
	{
	case SVCDBG_LEVEL_FAULT:
		logLevel = Log::L_CRIT;
		break;
	case SVCDBG_LEVEL_ERROR:
		logLevel = Log::L_ERROR;
		break;
	case SVCDBG_LEVEL_WARNING:
		logLevel = Log::L_WARNING;
		break;
	case SVCDBG_LEVEL_NOTICE:
		logLevel = Log::L_NOTICE;
		break;
	case SVCDBG_LEVEL_DEBUG:
		logLevel = Log::L_DEBUG;
		break;
	default:
		logLevel = Log::L_WARNING;
		break;
	}
	if ((&glog) != NULL)
		glog(logLevel, buf);
#endif
	
	return result;
}

int svcdbg_dumpInternalError()
{
	return 0;
}

void svcdbg_printFault(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	svcdbg_print(SVCDBG_LEVEL_FAULT, format, vlist);
}

void svcdbg_printError(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	svcdbg_print(SVCDBG_LEVEL_ERROR, format, vlist);
}

void svcdbg_printWarning(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	svcdbg_print(SVCDBG_LEVEL_WARNING, format, vlist);
}

void svcdbg_printDebug(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	svcdbg_print(SVCDBG_LEVEL_DEBUG, format, vlist);
}

void svcdbg_printNotice(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	svcdbg_print(SVCDBG_LEVEL_NOTICE, format, vlist);
}

#endif // #ifdef _NOTRACE

//////////////////////////////////////////////////////////////////////////

uint32 sf_idle_timeout = SF_IDLE_TIMEOUT;

//////////////////////////////////////////////////////////////////////////

//
// 基于 SSLSocket 的 IConn 的实现, 用于服务器在多通道时
// 支持主连接之外的连接
// 方法注解注解请参看接口定义(ServiceFrame.h)
//
class IConnectionImpl : public IConn 
{
public:
	IConnectionImpl(SSLSocket& sock);
	virtual ~IConnectionImpl();

	virtual int recv(OUT void* buf, IN int size);
	virtual int recvTimeout(OUT void* buf, IN int size, IN int timeo);
	virtual int send(IN const void* buf, IN int size);
	virtual int sendTimeout(IN const void* buf, IN int size, IN int timeo);
	virtual int close();
	virtual int upgradeSecurity(int timeout);
	virtual int degradeSecurity();
	virtual bool isSecure();
	virtual bool isActive();
	virtual void release();
	virtual int getConnId();
	virtual int recvFrom(OUT char* buf , IN int size , ConnID* fromConID)
	{
		//connection oriented do not need this
		assert(false);
		return -1;
	}
	virtual int	sendTo(IN char* buf , IN int size ,ConnID* targetConnID)
	{
		//connection oriented do not need this
		assert(false);
		return -1;
	}
protected:
	SSLSocket&	_ssock;
};

static unsigned __int64 gConnectionIDXXX =0;
//
// 基于 SSLSocket 的 IMainConn 的实现, 用于服务器实现主连接
//
#define MAINCONN_CANNOT_SEND			1
#define MAINCONN_CANNOT_RECV			2

class IMainConnImpl: public IMainConn 
{
	friend class ServiceFrmBase;
public:
	IMainConnImpl(ServiceFrmBase& frm, SSLSocket& sock);
	virtual ~IMainConnImpl();

	virtual void onConnected();
	virtual int recv(OUT void* buf, IN int size);
	virtual int recvTimeout(OUT void* buf, IN int size, IN int timeo);
	virtual int send(IN const void* buf, IN int size);
	virtual int sendTimeout(IN const void* buf, IN int size, IN int timeo);
	virtual int close();

	virtual int recvFrom(OUT char* buf , IN int size , ConnID* fromConID);

	virtual int	sendTo(IN char* buf , IN int size ,ConnID* targetConnID);

	
	virtual int upgradeSecurity(int timeout);
	virtual int degradeSecurity();
	virtual bool isSecure();
	virtual bool isActive();
	virtual void release();
	virtual int getConnId();

	virtual int getPeerInfo(OUT PeerInfo* info);
	virtual int getLocalInfo(OUT LocalInfo* info );
	virtual IConn* createConn(IN const ConnID* addr, 
		IN int mode, bool inheritSec, IN int timeout);
	virtual void destroyConn(IN IConn* conn);

	virtual IDialogue* getDialogue();
	bool setDialogue(IDialogue* session);
	
	SSLSocket& getSSLSocket()
	{
		return _ssock;
	}

	DWORD64 getIdleTime();
	void updateIdleTime();

	bool modifyAttribute(uint32 added, uint32 removed)
	{
		_connAttr |= added;
		_connAttr &= ~removed;
		return true;
	}

	uint32 getAttribute()
	{
		return _connAttr;
	}

	unsigned __int64 getConnectionIdentity()
	{
		return _connectionIdentity;
	}
	virtual int getConnType( ) 
	{
		return CONN_TYPE_SOCKET_TCP;
	}
#if !defined(_SPEED_VERSION)
	void dump()
	{
		printDebug("IMainConnImpl dump: _session(Dialog) = %x, _ssock = %x", 
			_session, _ssock.getSocket());
	}
#endif

protected:
	void destroyAllConn();
	bool getCurrentTime(DWORD64* t);
protected:
	ServiceFrmBase&	_srvFrm;
	SSLSocket&		_ssock;
	IDialogue*		_session;
	ServiceFrmBase*	_service;
	PeerInfo		_peerInfo;
	LocalInfo		_localInfo;
	typedef vector<IConnectionImpl* > VecConn;
	VecConn			_vecConn;
	uint32			_connAttr;
	DWORD64			_lastActiveTime;	
	unsigned __int64 _connectionIdentity;
};

//////////////////////////////////////////////////////////////////////////

//
// class IConnectionImpl
// IConn 接口实现类
//

IConnectionImpl::IConnectionImpl(SSLSocket& sock) :
	_ssock(sock)
{

}

IConnectionImpl::~IConnectionImpl()
{
	close();
}

int IConnectionImpl::recv(OUT void* buf, IN int size)
{
	return _ssock.recv(buf, size);
}

int IConnectionImpl::recvTimeout(OUT void* buf, IN int size, IN int timeo)
{
	int result;
	timeval t;
	MS2TV(t, timeo);
	result = _ssock.waitForIncome(&t);
	if (result <= 0) // error occurred or timeout
		return result;
	return _ssock.recv(buf, size);
}

int IConnectionImpl::send(IN const void* buf, IN int size)
{
	return _ssock.send(buf, size);
}

int IConnectionImpl::sendTimeout(IN const void* buf, IN int size, IN int timeo)
{
	int result;
	timeval t;
	MS2TV(t, timeo);
	result = _ssock.waitForOutgo(&t);
	if (result <= 0) // error occurred or timeout
		return result;
	return _ssock.send(buf, size);
}

int IConnectionImpl::close()
{
	int result = _ssock.close();
	_delete(&_ssock);
	return result;
}

int IConnectionImpl::upgradeSecurity(int timeout)
{
	return _ssock.s_accept();
}

int IConnectionImpl::degradeSecurity()
{
	return _ssock.s_shutdown();
}

bool IConnectionImpl::isSecure()
{
	return _ssock.isSecure();
}

bool IConnectionImpl::isActive()
{
	return _ssock.getSocket() != INVALID_SOCKET;
}

void IConnectionImpl::release()
{
	_delete(this);
}

int IConnectionImpl::getConnId()
{
	return (int )_ssock.getSocket();
}
//
// class IMainConnImpl
// IMainConn 接口实现类
//

IMainConnImpl::IMainConnImpl(ServiceFrmBase& frm, SSLSocket& sock) :
	_srvFrm(frm), _ssock(sock)
{
	_session = NULL;
	_connAttr = 0;
	getCurrentTime(&_lastActiveTime);
	frm.addMainConn(this);


	gConnectionIDXXX ++;
	if(gConnectionIDXXX > 0xFFFFFFFFFFFFFFFF-2)
		gConnectionIDXXX=0;
	_connectionIdentity=gConnectionIDXXX;
}

IMainConnImpl::~IMainConnImpl()
{
	destroyAllConn();
	_srvFrm.delMainConn(this);
	delete &_ssock;
}

void IMainConnImpl::onConnected()
{
	time_t t;
	SOCKET s;
	time(&t);
	_peerInfo.ct = *localtime(&t);
	s = _ssock.getSocket();
	SF_ASSERT(s != INVALID_SOCKET);
	_peerInfo.addr.type = CONN_TYPE_SOCKET_TCP;
	_peerInfo.addr.addrlen = sizeof(_peerInfo.addr.caddr);
	::getpeername(s, (sockaddr*)&_peerInfo.addr.caddr, &_peerInfo.addr.addrlen);

	_localInfo.addr.type = CONN_TYPE_SOCKET_TCP;
	_localInfo.addr.addrlen = sizeof(_localInfo.addr.caddr);
	::getsockname(s,  (sockaddr*)&_localInfo.addr.caddr, &_localInfo.addr.addrlen);
}

int IMainConnImpl::recv(OUT void* buf, IN int size)
{
	if (_connAttr & MAINCONN_CANNOT_RECV) {
		assert(false);
		printDebug("Can't read data at here.");
		return SOCKET_ERROR;
	}

	int result = _ssock.recv(buf, size);
	if (result > 0)
		getCurrentTime(&_lastActiveTime);
	return result;
}

int IMainConnImpl::recvTimeout(OUT void* buf, IN int size, IN int timeo)
{
	int result;
	timeval t;
	
	if (_connAttr & MAINCONN_CANNOT_RECV) {
		printDebug("Can't read data at here.");
		return SOCKET_ERROR;
	}

	MS2TV(t, timeo);
	result = _ssock.waitForIncome(&t);
	if (result <= 0) // error occurred or timeout
		return result;

	getCurrentTime(&_lastActiveTime);
	return _ssock.recv(buf, size);
}

int IMainConnImpl::send(IN const void* buf, IN int size)
{
	if (_connAttr & MAINCONN_CANNOT_SEND) {
		printDebug("Can't write data at here.");
		return SOCKET_ERROR;
	}

	int result = _ssock.send(buf, size);
	if (result > 0)
		getCurrentTime(&_lastActiveTime);

	return result;
}

int IMainConnImpl::sendTimeout(IN const void* buf, IN int size, IN int timeo)
{
	int result;
	timeval t;

	if (_connAttr & MAINCONN_CANNOT_SEND) {
		printDebug("Can't write data at here.");
		return SOCKET_ERROR;
	}

	MS2TV(t, timeo);
	result = _ssock.waitForOutgo(&t);
	if (result <= 0) // error occurred or timeout
		return result;
	getCurrentTime(&_lastActiveTime);
	return _ssock.send(buf, size);
}
int IMainConnImpl::sendTo(IN char* buf , IN int size ,ConnID* targetConnID)
{
	assert(FALSE);
	return -1;
}
int IMainConnImpl::recvFrom(OUT char* buf , IN int size , ConnID* fromConID)
{
	assert(FALSE);
	return -1;
}

int IMainConnImpl::close()
{
	return _ssock.close();
}

int IMainConnImpl::upgradeSecurity(int timeout)
{
	return _ssock.s_accept();
}

int IMainConnImpl::degradeSecurity()
{
	return _ssock.s_shutdown();
}

bool IMainConnImpl::isSecure()
{
	return _ssock.isSecure();
}

bool IMainConnImpl::isActive()
{
	return _ssock.getSocket() != INVALID_SOCKET;
}

void IMainConnImpl::release()
{
	_delete(this);
}

int IMainConnImpl::getConnId()
{
	return (int )_ssock.getSocket();
}

int IMainConnImpl::getPeerInfo(OUT PeerInfo* info)
{
	memcpy(info, &_peerInfo, sizeof(_peerInfo));
	return sizeof(_peerInfo);
}
int IMainConnImpl::getLocalInfo(OUT LocalInfo* info ) 
{
	memcpy(info, &_localInfo, sizeof(_localInfo));
	return sizeof(_localInfo);
}

IConn* IMainConnImpl::createConn(IN const ConnID* addr, 
								IN int mode, 
								bool inheritSec, 
								IN int timeout)
{
	SSLSocket sock(INVALID_SOCKET, _ssock.s_getConetext());
	SSLSocket * newsock;
	IConnectionImpl* conn;
	bool secure = false;
	if (inheritSec)
		secure = _ssock.isSecure();
	if (addr->type != CONN_TYPE_SOCKET_TCP)
		return NULL;

	if (mode == CONN_MODE_PASSIVE)
	{
		sock.socket();

		if (sock.bind((sockaddr*)&addr->caddr, addr->addrlen) != 0)
		{
			sock.close();
			return NULL;
		}
		
		sock.listen();
		newsock = sock.accept(NULL, NULL, secure);
		if (newsock == NULL)
			return NULL;
		conn = new IConnectionImpl(*newsock);
		if (conn == NULL) 
		{
			_delete(newsock);
			return NULL;
		}

		_vecConn.push_back(conn);
		return conn;
	}
	else if (mode == CONN_MODE_ACTIVE) 
	{
		newsock = new SSLSocket(INVALID_SOCKET, _ssock.s_getConetext());
		newsock->socket();
		if (newsock->connect((sockaddr*)&addr->caddr, addr->addrlen, secure) != 0)
			return NULL;
		
		conn = new IConnectionImpl(*newsock);
		if (conn == NULL) 
		{
			_delete(newsock);
			return NULL;
		}

		_vecConn.push_back(conn);
		return conn;
	}

	return NULL;
}

void IMainConnImpl::destroyConn(IN IConn* conn)
{
	VecConn::iterator itor;
	for (itor = _vecConn.begin(); itor < _vecConn.end(); itor ++) {
		if (*itor == conn) {
			_vecConn.erase(itor);
			break;
		}
	}

	_delete(conn);	
}

IDialogue* IMainConnImpl::getDialogue()
{
	return _session;
}

bool IMainConnImpl::setDialogue(IDialogue* dlg)
{
	if (isActive() && dlg == NULL) {
		SF_ASSERT(false);
		printError("IMainConnImpl::setDialogue() faild. "
			"dlg == NULL, but conn is active.");
		return false;
	}

	_session = dlg;
	return true;
}

void IMainConnImpl::destroyAllConn()
{
	VecConn::iterator itor = _vecConn.begin(), last=_vecConn.end();
	while (itor < _vecConn.end()) {
		last = itor ++;
		_delete(*last);
		_vecConn.erase(itor);
	}

	if (_vecConn.end()!=last)
		_delete(*last);

}

bool IMainConnImpl::getCurrentTime(DWORD64* t)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	return SystemTimeToFileTime(&st, (FILETIME* )t);
}

DWORD64 IMainConnImpl::getIdleTime()
{
	DWORD64 cur;
	getCurrentTime(&cur);
	return (cur - _lastActiveTime) / 10000;
}

void IMainConnImpl::updateIdleTime()
{
	getCurrentTime(&_lastActiveTime);
}

/*------------------------------------------------------------------------
IMainConnUDPImpl
------------------------------------------------------------------------*/
class IMainConnUDPImpl : public IMainConn
{
public:

	IMainConnUDPImpl(SOCKET s)		
	{	
		_sock = s ;		
		gConnectionIDXXX ++;
		if(gConnectionIDXXX > 0xFFFFFFFFFFFFFFFF-2)
			gConnectionIDXXX=0;
		_connectionIdentity=gConnectionIDXXX;	

	}
	virtual int recv(OUT void* buf, IN int size);

	virtual int recvTimeout(OUT void* buf, IN int size, IN int timeo);

	virtual int send(IN const void* buf, IN int size) ;

	virtual int sendTimeout(IN const void* buf, IN int size, IN int timeo) ;

	virtual int recvFrom(OUT char* buf , IN int size , ConnID* fromConID);

	virtual int	sendTo(IN char* buf , IN int size ,ConnID* targetConnID) ;

	virtual int close() ;

	virtual int upgradeSecurity(int timeout) ;

	virtual int degradeSecurity() ;

	virtual bool isSecure() ;

	virtual bool isActive() ;
	
	virtual void release() ;
	
	virtual int getConnId() ;

	virtual int getPeerInfo(OUT PeerInfo* info) ;

	virtual int getLocalInfo(OUT LocalInfo* info ) ;

	virtual void onConnected() ;
		
	virtual IConn* createConn(IN const ConnID* addr, IN int mode, bool inheritSec, IN int timeout) ;
	
	virtual void destroyConn(IN IConn* conn) ;
	
	virtual IDialogue* getDialogue() ;
	
	virtual unsigned __int64 getConnectionIdentity();

	bool		setDialogue(IDialogue* dlg);

	virtual int getConnType( ) 
	{
		return CONN_TYPE_SOCKET_UDP;
	}
private:
	IDialogue*				_associatedDialog;
	SOCKET					_sock;
	unsigned __int64		_connectionIdentity;
	PeerInfo				_peerInfo;
	LocalInfo				_localInfo;
};


bool IMainConnUDPImpl::setDialogue(IDialogue* dlg)
{
	_associatedDialog = dlg;
	return true;
}
int IMainConnUDPImpl::recv(OUT void* buf, IN int size)
{//DO NOT CALL THIS FUNCTION
	assert(false);
	return -1;
}

int IMainConnUDPImpl::recvTimeout(OUT void* buf, IN int size, IN int timeo)
{//DO NOT CALL THIS FUNCTION
	assert(false);
	return -1;
}

int IMainConnUDPImpl::recvFrom(OUT char* buf , IN int size , ConnID* fromConID)
{
	assert(false);
	return -1;
}

int IMainConnUDPImpl::send(IN const void* buf, IN int size)
{
	//redirect to sendto
	return ::sendto(_sock,(const char*)buf,size , 0 , ((sockaddr*)&_peerInfo.addr.caddr) , _peerInfo.addr.addrlen );
}

int IMainConnUDPImpl::sendTimeout(IN const void* buf, IN int size, IN int timeo) 
{
#pragma message(__MSGLOC__"Can I redirect sendTimeout to sendto ?")
	return ::sendto(_sock,(const char*)buf,size , 0 , ((sockaddr*)&_peerInfo.addr.caddr) , _peerInfo.addr.addrlen );
}

int	IMainConnUDPImpl::sendTo(IN char* buf , IN int size ,ConnID* targetConnID) 
{
	return ::sendto(_sock,(const char*)buf,size , 0 , ((sockaddr*)&_peerInfo.addr.caddr) , _peerInfo.addr.addrlen );
}

int IMainConnUDPImpl::close() 
{	
	return closesocket (_sock);	
}

int IMainConnUDPImpl::upgradeSecurity(int timeout) 
{//DO NOT CALL THIS FUNCTION
	assert(false);
	return -1;
}

int IMainConnUDPImpl::degradeSecurity() 
{//DO NOT CALL THIS FUNCTION
	assert(false);
	return -1;
}

bool IMainConnUDPImpl::isSecure() 
{//of course it's not secure
	return false;
}

bool IMainConnUDPImpl::isActive()
{//UDP is always active
	return true;
}

void IMainConnUDPImpl::release()
{
	//delete the connection instance
	//NOTE:its's a dummy connection	
}

int IMainConnUDPImpl::getConnId()
{
	return (int)_sock;
}

int IMainConnUDPImpl::getPeerInfo(OUT PeerInfo* info)
{
	memcpy(info,&_peerInfo,sizeof(_peerInfo));
	return 1;
}

int IMainConnUDPImpl::getLocalInfo(OUT LocalInfo* info ) 
{
	memcpy(info,&_localInfo,sizeof(_localInfo));
	return 1;
}

void IMainConnUDPImpl::onConnected()
{//DO NOT CALL THIS FUNCTION
	//Call this function is useless in UDP state
	//assert(false);
}

IConn* IMainConnUDPImpl::createConn(IN const ConnID* addr, IN int mode, bool inheritSec, IN int timeout) 
{//DO NOT CALL THIS FUNCTION
	
	//Call this function is useless in UDP state
	return NULL;
}

void IMainConnUDPImpl::destroyConn(IN IConn* conn) 
{//DO NOT CALL THIS FUNCTION
	//assert(false);
}

IDialogue* IMainConnUDPImpl::getDialogue() 
{
	return _associatedDialog;
}

unsigned __int64 IMainConnUDPImpl::getConnectionIdentity()
{
	return _connectionIdentity;
}


//////////////////////////////////////////////////////////////////////////
ServiceConfig::ServiceConfig()
{
	_cfg_debugLevel = SVCDBG_LEVEL_DEBUG;
	_cfg_isSecure = false;
	_cfg_threadCount = 50;
	_cfg_threadPriority = 0;
	_cfg_recvBufSize = 8192;
	_cfg_minWorkingSet = 20*1024*1024;
	_cfg_maxConn = 2000;
	strcpy(_cfg_publicKeyFile, "server.pem");
	strcpy(_cfg_privateKeyFile, "server.pem");
	strcpy(_cfg_privateKeyFilePwd, "xiao");
	strcpy(_cfg_dhParamFile, "dh1024.pem");
	strcpy(_cfg_randFile, "rand.pem");

}

ServiceConfig::~ServiceConfig()
{

}

const ServiceConfig::_ConfigEntry* ServiceConfig::getBaseConfigMap()
{
	return NULL;
}

const ServiceConfig::_ConfigEntry* ServiceConfig::getConfigMap()
{
	static _ConfigEntry cfgMap[] = 
	{
		{"publicKeyFile", _ConfigEntry::STRING_VALUE, 
			&_cfg_publicKeyFile, sizeof(_cfg_publicKeyFile)}, 
		{"privateKeyFile", _ConfigEntry::STRING_VALUE, 
			&_cfg_privateKeyFile, sizeof(_cfg_privateKeyFile)}, 
		{"privateKeyFilePwd", _ConfigEntry::STRING_VALUE, 
			&_cfg_privateKeyFilePwd, sizeof(_cfg_privateKeyFilePwd)}, 
		{"dhParamFile", _ConfigEntry::STRING_VALUE, 
			&_cfg_dhParamFile, sizeof(_cfg_dhParamFile)}, 
		{"randFile", _ConfigEntry::STRING_VALUE, 
			&_cfg_randFile, sizeof(_cfg_randFile)}, 
		{"isSecure", _ConfigEntry::USHORT_VALUE, 
				&_cfg_isSecure, sizeof(_cfg_isSecure)}, 
		{"threadCount", _ConfigEntry::USHORT_VALUE, 
				&_cfg_threadCount, sizeof(_cfg_threadCount)}, 
		{"maxConnection", _ConfigEntry::ULONG_VALUE, 
				&_cfg_maxConn, sizeof(_cfg_maxConn)},
		{"debugLevel", _ConfigEntry::LONG_VALUE, 
			&_cfg_debugLevel, sizeof(_cfg_debugLevel)},	
		{"idleTimeout", _ConfigEntry::LONG_VALUE, 
			&sf_idle_timeout, sizeof(sf_idle_timeout)},
		{"maxPollTime", _ConfigEntry::ULONG_VALUE, 
			&sf_max_polltime, sizeof(sf_max_polltime)},
		{"recvBufSize", _ConfigEntry::ULONG_VALUE, 
		&_cfg_recvBufSize, sizeof(_cfg_recvBufSize)},
		{"threadPriority", _ConfigEntry::ULONG_VALUE, 
		&_cfg_threadPriority, sizeof(_cfg_threadPriority)},
		{"minWorkingSet", _ConfigEntry::ULONG_VALUE, 
		&_cfg_minWorkingSet, sizeof(_cfg_minWorkingSet)},
		{NULL, _ConfigEntry::INVALID_TYPE, NULL, 0}, 
	};

	return cfgMap;
}

bool ServiceConfig::load(const char* cfgfile,std::vector<std::string>& pathVec)
{
	/* read configuration from XML file */
	//using XMLPreferenceEx
	//ComInitializer init;
	XMLPreferenceDocumentEx root;
	XMLPreferenceEx* pref = NULL, * child = NULL;
	
	try {
		if(!root.open(cfgfile)) {
			printError("ServiceFrmBase::initByConfigureFile() failed");
			return false;
		}

		pref = root.getRootPreference();
		if (pref == NULL)
			return false;
		if(pathVec.size() <= 0)
		{
			child = pref->firstChild("Service");		
			pref->free();
		}
		else
		{
			while (pathVec.size() > 0)
			{
				child=pref->firstChild(pathVec[0].c_str());
				if(!child)
					return false;
				pathVec.erase(pathVec.begin());
				pref->free();
				pref=child;
			}
		}
		if (!child)
			return false;
		
		pref = child;
		const _ConfigEntry* cfgEntry;
		cfgEntry = getBaseConfigMap();
		if (cfgEntry != NULL)
			processEntry(pref, cfgEntry);

		cfgEntry = getConfigMap();
		if (cfgEntry != NULL)
			processEntry(pref, cfgEntry);

		pref->free();

	}  catch(Exception ) {
		if (pref)
			pref->free();
		return false;
	}

	if (sf_max_polltime < MAX_CONN_PER_REQ)
		sf_max_polltime = MAX_CONN_PER_REQ;

	return true;
}

bool ServiceConfig::processEntry(XMLPreferenceEx* pref, 
								 const _ConfigEntry* cfgEntry)
{
	XMLPreferenceEx* child = NULL;
	char buf[512];
	char* fmt;
	try {
		while(cfgEntry->type != _ConfigEntry::INVALID_TYPE) {
			child = pref->firstChild(cfgEntry->key);
			if (child == NULL)
				goto L_Next;
			if (child->get("value", buf, "", sizeof(buf) - 1) == "") {
				child->free();
				goto L_Next;
			}
			switch(cfgEntry->type) {
			case _ConfigEntry::STRING_VALUE:
				fmt = "%s";
				break;
			case _ConfigEntry::SHORT_VALUE:
				fmt = "%hd";
				break;
			case _ConfigEntry::USHORT_VALUE:
				fmt = "%hu";
				break;
			case _ConfigEntry::LONG_VALUE:
				fmt = "%ld";
				break;
			case _ConfigEntry::ULONG_VALUE:
				fmt = "%lu";
				break;
			default:
				fmt = "%s";
				assert(false);
				break;
			}
			if(cfgEntry->type==_ConfigEntry::STRING_VALUE)
				strncpy((char*)cfgEntry->value,buf,cfgEntry->maxValue);
			else
				sscanf(buf, fmt, cfgEntry->value);
			child->free();
L_Next:
			cfgEntry ++;
		}

	} catch(Exception ) {
		if (child)
			child->free();
		return false;
	}

	return true;

}
/*------------------------------------------------------------------------
class ServerUDPSocket //: public NativeThread
{
public:	
ServerUDPSocket(ServiceFrmBase* pFrmBase , int appType = APP_TYPE_LSCP)
~ServerUDPSocket(){;}
public:
bool	InitializeSocket(const char* nodeName , const char* servName,int socketFamily , bool bSecure);
//int		run(){return 1;}
protected:
IMainConnImpl*				accept();

  protected:
  SSLContext					_sslCtx;
  SSLSocket					_ssock;
  addrinfo*					resAddrinfo;
  ServiceFrmBase*				_pFrmBase;
  bool						_secure;
  int							_appType;
};
-------------------------------------------------------------------------*/
ServerUDPSocket::ServerUDPSocket (ServiceFrmBase* pFrmBase , int appType /* = APP_TYPE_LSCP */)
{
	_pFrmBase = pFrmBase;
	_appType = appType;
	resAddrinfo = NULL;
}

ServerUDPSocket::~ServerUDPSocket ()
{
	if (_ssock != INVALID_SOCKET) 
	{
		closesocket (_ssock);
		_ssock = INVALID_SOCKET;
	}
	if (resAddrinfo) 
	{
		freeaddrinfo(resAddrinfo);
		resAddrinfo = NULL;
	}
}
void ServerUDPSocket::close()
{
	if (_ssock != INVALID_SOCKET) 
	{
		closesocket (_ssock);
		_ssock = INVALID_SOCKET;
	}
}
bool ServerUDPSocket::InitializeSocket ( const char* nodeName , 
										const char* servName,
										int socketFamily , 
										bool bSecure )
{
	_secure = bSecure;
	addrinfo hint;
	
	ZeroMemory(&hint,sizeof(hint));
	
	hint.ai_family = socketFamily;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = IPPROTO_UDP;
	hint.ai_flags = AI_NUMERICHOST;
	
	int rc = getaddrinfo(nodeName,servName,&hint,&resAddrinfo);
	if ( rc != 0 ) 
	{
		printWarning("getAddrInfo failed with nodeName[%s] servName[%s] error[%u]",nodeName,servName,WSAGetLastError());
		return false;
	}
	
	_ssock = WSASocket(resAddrinfo->ai_family,resAddrinfo->ai_socktype,resAddrinfo->ai_protocol,NULL,0,WSA_FLAG_OVERLAPPED);
	
	
	if ( _ssock == INVALID_SOCKET) 
	{
		printError("Create socket erro [%u]",WSAGetLastError());
		return false;
	}	
	
	if (::bind(_ssock, resAddrinfo->ai_addr , resAddrinfo->ai_addrlen) != 0) 
	{
		printError("_ssock.bind() failed with nodeName[%s] servName[%s] error[%u]",nodeName,servName,WSAGetLastError());
		return false;
	}
	
	//No listen because UDP	
	//////////////////////////////////////////////////////////////////////////
	IMainConnImpl*	conn =(IMainConnImpl*) CreateMainConn();
		
	if (conn != NULL)
	{
		SOCKET s = (SOCKET )conn->getConnId();
		SvcCompletionPortKey* key = new SvcCompletionPortKey;
		memset(key, 0, sizeof(*key));
		//set the app type and connection type
		key->connType = CONN_TYPE_SOCKET_UDP;
		key->appType = _appType;
		key->perrInfo.addr.addrlen = sizeof(key->perrInfo.addr.caddr);
		key->serverSocket = _ssock;
		key->mainConn = conn;			
		if (!_pFrmBase->processConn(conn,key))
		{
			// may it is overload				
			printError("ServerUDPSocket  processConn() failed.");
		}
	} 
	else 
	{
		printError("ServerUDPSocket Can't create Main Conn");
		return false;
	}

	return true;
}
IMainConn* ServerUDPSocket::CreateMainConn ()
{
	return (IMainConn*) new IMainConnUDPImpl(_ssock);	
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//ServerTCPSocket
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
ServerTCPSocket::ServerTCPSocket(ServiceFrmBase* pFrmBase,int appType)
{
	_pFrmBase = pFrmBase;
	resAddrinfo = NULL;
	_appType = appType;
}
ServerTCPSocket::~ServerTCPSocket()
{
	if(resAddrinfo)
	{
		freeaddrinfo(resAddrinfo);
		resAddrinfo = NULL;
	}
}
void ServerTCPSocket::close()
{
	_ssock.close();
}
bool ServerTCPSocket::InitializeSocket(const char* nodeName , const char* servName,int socketFamily , bool bSecure)
{
	_secure = bSecure;
	addrinfo hint;
	
	ZeroMemory(&hint,sizeof(hint));
	
	hint.ai_family = socketFamily;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	hint.ai_flags = AI_NUMERICHOST;
	
	int rc = getaddrinfo(nodeName,servName,&hint,&resAddrinfo);
	if ( rc != 0 ) 
	{
		printWarning("getAddrInfo failed with nodeName[%s] servName[%s] error[%u]",nodeName,servName,WSAGetLastError());
		return false;
	}
	SOCKET s;
	
	s = WSASocket(resAddrinfo->ai_family,resAddrinfo->ai_socktype,resAddrinfo->ai_protocol,NULL,0,WSA_FLAG_OVERLAPPED);
	
	
	if ( s == INVALID_SOCKET) 
	{
		printWarning("Create socket erro [%u]",WSAGetLastError());
		return false;
	}
	
	_ssock.attach(s);

	if (_ssock.bind( resAddrinfo->ai_addr , resAddrinfo->ai_addrlen) != 0) 
	{
		printError("_ssock.bind() failed with nodeName[%s] servName[%s] error[%u]",nodeName,servName,WSAGetLastError());
		return false;
	}
	
	if (_ssock.listen() != 0)
	{
		printError("_ssock.listen() failed error[%u]",WSAGetLastError());
		return false;
	}

	return true;
}
int ServerTCPSocket::run()
{
	IMainConnImpl* conn;
	while (true) 
	{
		conn = accept();
		rwlock_lock_read( _pFrmBase->_connListLock, INFINITE);
		size_t connCount =_pFrmBase->_connList.size();
		rwlock_unlock_read(_pFrmBase->_connListLock);
		
		
		if (connCount > _pFrmBase->_maxConn) 
		{			
			conn->close();
			_delete(conn);
			printWarning("the count of connections overed limit with MaxConnection is %d",_pFrmBase->_maxConn);
			continue;
		}
		
		if (conn != NULL)
		{
			SOCKET s = (SOCKET )conn->getConnId();
			SvcCompletionPortKey* key = new SvcCompletionPortKey;
			memset(key, 0, sizeof(*key));
			//set the app type and connection type
			key->connType = CONN_TYPE_SOCKET_TCP;
			key->appType = _appType;
			key->mainConn = conn;
			key->serverSocket = _ssock.getSocket ();
			if (!_pFrmBase->processConn(conn,key))
			{
				// may it is overload				
				printError("ServiceFrmBase::run(): processConn() failed.");
			}
		} 
		else 
		{
			printError("ServiceFrmBase::run(): accept failed.");
			return -1;
		}
	}		
	_ssock.close();
	return 1;
}
IMainConnImpl* ServerTCPSocket::accept()
{
	SSLSocket* newsock;
	
	rwlock_lock_read(_pFrmBase->_connListLock, INFINITE);
	size_t connCount = _pFrmBase->_connList.size();	
	rwlock_unlock_read(_pFrmBase->_connListLock);
	
	newsock = _ssock.accept(NULL, NULL, _secure);
	if (newsock == NULL)
	{
		printDebug("_ssock.accept() failed.\n");
		//assert(false);
		return NULL;
	}
	
	printDebug("_maxCount = %d, connCount = %d\n", _pFrmBase->_maxConn, connCount);
	
	
	
	IMainConnImpl* conn = _pFrmBase->createMainConn(newsock);
	if (conn == NULL) 
	{
		_delete(newsock);
		printFault("ServiceFrmBase::accept(): new IMainConnImpl failed.");
		SF_ASSERT(false);
		return NULL;
	}
	
	return conn;
}

//
// ServiceFrmBase
//

ServiceFrmBase::ServiceFrmBase()
	//_sslCtx(VER_SSLv2v3)
{
	_dlgCtor = NULL;
	_secure = FALSE;
	_maxConn = 850;
	_acceptConn = 1;
	_threadCount = 50;
	_running = false;
	_idleTimeout = SF_IDLE_TIMEOUT;
	_serviceThread = true;
	_connListLock = create_rwlock();	
	_rtspPort =554;
	_lscpPort = 5542;
}

ServiceFrmBase::~ServiceFrmBase()
{
	destroy_rwlock(_connListLock);
}

bool ServiceFrmBase::initByConfiguration(const ServiceConfig* cfg)
{
	if (cfg == NULL) {
		assert(false);
		return false;
	}
	
	svcdbg_setLevel(cfg->_cfg_debugLevel);
	ssldbg_setLevel(cfg->_cfg_debugLevel);
	_secure = cfg->_cfg_isSecure;
	_threadCount = cfg->_cfg_threadCount;
	_maxConn = cfg->_cfg_maxConn;
	_recvBufSize = cfg->_cfg_recvBufSize;
	_threadPriority = cfg->_cfg_threadPriority;
	_minWorkingSet = cfg->_cfg_minWorkingSet;

	if (_minWorkingSet<20*1024*1024)
	{
		_minWorkingSet = 20*1024*1024;
	}

	if (_threadCount == 0) {
		_threadCount = 50;
	}

	if (_threadCount > 1000) {
		_threadCount = 1000;
	}

	if (_maxConn > 1000000) {
		_maxConn = 1000000;
	}	

	if (_recvBufSize > 64*1024)
	{
		_recvBufSize = 64*1024;
	}
	if (_recvBufSize < 1024)
	{
		_recvBufSize = 1024;
	}
	if(_threadPriority<THREAD_PRIORITY_NORMAL)
		_threadPriority=THREAD_PRIORITY_NORMAL;
	if(_threadPriority>THREAD_PRIORITY_TIME_CRITICAL)
		_threadPriority=THREAD_PRIORITY_TIME_CRITICAL;
	if (_secure) 
	{
//		// load_randomess_file(cfg->_cfg_randFile);
//		_sslCtx.loadCertificate(cfg->_cfg_publicKeyFile);
//		_sslCtx.loadPrivateKeyFile(cfg->_cfg_privateKeyFile, cfg->_cfg_privateKeyFilePwd);
//		// _sslCtx.loadRootCertificates("roots.pem");
//		_sslCtx.loadDHParams(cfg->_cfg_dhParamFile);
	}

	return true;
}

bool ServiceFrmBase::init(const ServiceConfig* cfg /* = NULL */)
{
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 0), &wsad);
	
	if (_secure) 
	{
		init_ssl_library();
		//_sslCtx.create();
	}

	if (cfg == NULL || initByConfiguration(cfg) == false)
	{
		// use default value
//		if (_secure) 
//		{
//			// load_randomess_file("rand.pem");
//			_sslCtx.loadCertificate("server.pem");
//			_sslCtx.loadPrivateKeyFile("server.pem", "xiao");
//			// _sslCtx.loadRootCertificates("roots.pem");
//			_sslCtx.loadDHParams("dh1024.pem");
//		}
	}

	if (_secure)
	{
//		_ssock.s_setConetext(&_sslCtx);
	}

	printDebug("_secure = %d, _threadCount = %d, _maxCount = %d\n", _secure, _threadCount, _maxConn);

	return true;
}

void ServiceFrmBase::uninit()
{

}

bool ServiceFrmBase::begin(unsigned short rtspPort , unsigned short lscpPort)
{
	if (_running) 
	{
		printFault("ServiceFrmBase::begin(): service is running.");
		SF_ASSERT(false);
		return false;
	}

//	if (connID->type != CONN_TYPE_SOCKET_TCP) 
//	{
//		printError("ServiceFrmBase::begin(): Must be TCP service.");
//		SF_ASSERT(false);
//		return false;
//	}
	
#pragma message(__MSGLOC__"TODO:初始化socket")	


	if (_serviceThread)
	{
		return start();
	}
	else 
	{
		run();
		return true;
	}
}

bool ServiceFrmBase::end()
{
	// _threadMgr->cancelAll();
	this->terminate(-1);	
	_running = false;
	return true;
}

IMainConn* ServiceFrmBase::getMainConn(int index)
{
	return *(_connList.begin() + index);
}

int ServiceFrmBase::getMainConnCount()
{
	return _connList.size();
}

bool ServiceFrmBase::setDialogueCreator(IDialogueCreator* dlgCtor)
{
	if (_dlgCtor)
		return false;

	_dlgCtor = dlgCtor;
	return true;
}

IDialogueCreator* ServiceFrmBase::getDialogueCreator()
{
	return _dlgCtor;
}

IMainConnImpl* ServiceFrmBase::createMainConn(SSLSocket* sock)
{
	assert(sock);
	return new IMainConnImpl(*this, *sock);
}

bool ServiceFrmBase::addMainConn(IMainConnImpl* conn)
{
	if (!conn)
		return false;

#ifdef _DEBUG
	/// 调试状态检查是否该连接已经在列表中,通常这代表有个编程错误发生了
	vector<IMainConn* >::iterator itor = _connList.begin();
	for(; itor != _connList.end(); itor ++) {
		if (*itor == conn) {
			printFault("the connect in the list alread conn = %p", conn);
			return false;
		}
	}
#endif

	size_t connCount;
	rwlock_lock_write(_connListLock, INFINITE);
	_connList.push_back(conn);
	connCount = _connList.size();

#ifdef _BSD
	if (connCount >= _maxConn) {
		_acceptConn = 0;
		_ssock.setsockopt(SOL_SOCKET, SO_ACCEPTCONN, 
			(const char* )&_acceptConn, 
			sizeof(_acceptConn));
	}
#endif
	
	rwlock_unlock_write(_connListLock);
	printDebug("ServiceFrmBase::addMainConn()\tconn = %p, "
		"Connection count = %d", conn, connCount);
	return true;
}

bool ServiceFrmBase::delMainConn(IMainConnImpl* conn)
{
	if (conn == NULL)
		return false;

	size_t connCount;

	bool deleted = false;
	rwlock_lock_write(_connListLock, INFINITE);
	vector<IMainConn* >::iterator itor = _connList.begin();
	for(; itor != _connList.end(); itor ++) {
		if (*itor == conn) {
			_connList.erase(itor);
			connCount = _connList.size();
#ifdef _BSD
			if (_acceptConn && connCount < _maxConn) {
				_acceptConn = 1;
				_ssock.setsockopt(SOL_SOCKET, SO_ACCEPTCONN, 
					(const char* )&_acceptConn, 
					sizeof(_acceptConn));
			}
#endif
			deleted = true;
			break;
		}
	}

	rwlock_unlock_write(_connListLock);

	if (deleted) {
		printDebug("ServiceFrmBase::delMainConn()\tconn = %p, "
			"Connection count = %d", conn, connCount);
	} else {
		
		printWarning("delete main conn but not found. conn = %p", conn);
	}

	return deleted;
}

int ServiceFrmBase::run()
{
	return 0;
}


// 动态改变安全属性实现困难
bool ServiceFrmBase::setSecure(bool secure)
{
	return _secure = secure;
}


bool ServiceFrmBase::isSecure()
{
	return _secure;
}

bool ServiceFrmBase::getServiceThread()
{
	return _serviceThread;
}

bool ServiceFrmBase::setServiceThread(bool b)
{
	if (_running)
		return false;
	_serviceThread = b;
	return true;
}

int ServiceFrmBase::getIdleTimeout()
{
	return _idleTimeout;
}

bool ServiceFrmBase::setIdleTimeout(int timeo)
{
	if (_running)
		return false;
	_idleTimeout = timeo;
	return true;
}

uint32 ServiceFrmBase::getMaxConnection()
{
	return _maxConn;
}

bool ServiceFrmBase::setMaxConnection(uint32 maxConn)
{
	if (maxConn >= 1000000) {
		// this is a error? or you must upgrade the performance of frame
		printError("maxConnection is invalid.");
		assert(false);
		return false;
	}

	_maxConn = maxConn;
	return true;
}

uint32 ServiceFrmBase::getThreadCount()
{
	return _threadCount;
}

bool ServiceFrmBase::setThreadCount(uint16 count)
{
	/*
	if (count > 200) {
		// too many thread
		assert(false);
		return false;
	}
	_threadCount = count;
	return true;
	*/

	// 不实现动态改变线程数
	assert(false);
	return false;
}

#ifdef _WIN32_SPECIAL_VERSION
//////////////////////////////////////////////////////////////////////////
// special implementation that base on Win32 IO completion port

class ServiceFrmWin32Special;

class ServiceThreadGroup 
{
public:
	ServiceThreadGroup(ServiceFrmWin32Special& frm, uint32 threadCount, uint32 threadPriority=THREAD_PRIORITY_NORMAL, uint32 recvBufSize=MAX_RECV_BUFF);
	virtual ~ServiceThreadGroup();
	bool start();
	bool stop();
	bool requestThread(IMainConn* conn,SvcCompletionPortKey* newKey);

protected:
	bool createThreads(uint32 threadCount);
	void destroyThreads();
	virtual DWORD threadProc();
	static DWORD __stdcall _threadProc(PVOID param);

protected:
	ServiceFrmWin32Special&	_serviceFrm;

	HANDLE		_completionPort;
	uint32		_threadCount;
	PHANDLE		_threadHandles;
	uint32		_threadPriority;
	uint32		_recvBufSize;	
};




ServiceThreadGroup::ServiceThreadGroup(ServiceFrmWin32Special& frm, 
									   uint32 threadCount, uint32 threadPriority, uint32 recvBufSize):
	_serviceFrm(frm), _threadCount(threadCount)
{
	_completionPort = NULL;
	_threadHandles = NULL;
	_threadPriority = threadPriority;
	_recvBufSize = recvBufSize;

	if (_recvBufSize > 64*1024)
	{
		_recvBufSize = 64*1024;
	}
	if (_recvBufSize < 1024)
	{
		_recvBufSize = 1024;
	}
	
	if(_threadPriority<THREAD_PRIORITY_NORMAL)
		_threadPriority=THREAD_PRIORITY_NORMAL;
	if(_threadPriority>THREAD_PRIORITY_TIME_CRITICAL)
		_threadPriority=THREAD_PRIORITY_TIME_CRITICAL;
}

ServiceThreadGroup::~ServiceThreadGroup()
{

}

bool ServiceThreadGroup::start()
{
	_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 
		NULL, 0, _threadCount);
	if (!_completionPort) 
	{
		printFault("ServiceThreadGroup::start()\t"
			"CreateIoCompletionPort() failed.\n");
		return false;
	}
	
	if (!createThreads(_threadCount))
		return false;

	return true;
}

bool ServiceThreadGroup::stop()
{
	return false;
}

bool ServiceThreadGroup::requestThread(IMainConn* conn,SvcCompletionPortKey* key)
{
	//associate SOCKET handle with completion port
	SOCKET s= (SOCKET)key->mainConn->getConnId();
	if (!CreateIoCompletionPort((HANDLE )s, _completionPort, 
		(ULONG_PTR )key, 0))
	{
		printFault("ServiceThreadGroup::requestThread()\t"
						"CreateIoCompletionPort() failed.\n");
		return false;
	}
	

	DWORD readBytes;
	WSABUF wsaBuf;
	DWORD wsaFlag = 0;
	IDialogue* dlg;
	
	wsaBuf.buf = (char *)key->buf;
	wsaBuf.len = sizeof(key->buf);
	memset(&key->overlapped, 0, sizeof(key->overlapped));

	int iRet = 0;
	if( key->connType == CONN_TYPE_SOCKET_TCP )
	{
		iRet = WSARecv(s, &wsaBuf, 1, &readBytes, &wsaFlag, &key->overlapped, NULL) ;
		
	}
	else if ( key->connType == CONN_TYPE_SOCKET_UDP ) 
	{
		iRet = WSARecvFrom( s, &wsaBuf , 1 , &readBytes , &wsaFlag , 
							(sockaddr*)(&key->perrInfo.addr.caddr),
							&key->perrInfo.addr.addrlen,
							&key->overlapped, NULL);	
	}
	else
	{
		assert(FALSE);
	}
	
	if (iRet == SOCKET_ERROR  )
	{
		int errCode = WSAGetLastError();
		if ( errCode != ERROR_IO_PENDING) 
		{			
			dlg = conn->getDialogue();
			conn->close();
			if ( key->connType == CONN_TYPE_SOCKET_TCP ) 
			{
				((IMainConnImpl*)conn)->setDialogue(NULL);				
			}
			else if ( key->connType == CONN_TYPE_SOCKET_UDP ) 
			{
				((IMainConnUDPImpl*)conn)->setDialogue(NULL);
			}
			else
			{
				printError("Unkown ConnType[%d]",key->connType);
			}
			{

			}
			
			dlg->onConnectionDestroyed(conn);
			_serviceFrm.getDialogueCreator()->releaseDialogue(dlg);
			_delete(key);
			printWarning("ServiceThreadGroup::requestThread():\t"
				"WSARecv() failed.");
			return false;
		}
	}
	
	return true;
}

bool ServiceThreadGroup::createThreads(uint32 threadCount)
{
	_threadHandles = new HANDLE[threadCount];
	memset(_threadHandles, 0, sizeof(HANDLE) * threadCount);

	if (!_threadHandles) {
		printFault("ServiceThreadGroup::createThreads()\t"
			"malloc() failed.\n");
		return false;
	}

	DWORD dwThreadId;
	for (int i = 0; i < threadCount; i ++) {
		_threadHandles[i] = CreateThread(NULL, 0, &_threadProc, 
			this, 0, &dwThreadId);
		if (!_threadHandles[i]) {
			printFault("ServiceThreadGroup::createThreads()\t"
				"CreateThread() failed.\n");
			destroyThreads();
			return false;
			_delete(_threadHandles);
			_threadHandles = NULL;
		}

		if (_threadPriority!=THREAD_PRIORITY_NORMAL)
		{
			if (SetThreadPriority(_threadHandles[i], _threadPriority))
			{
//				printf("ServiceFrame::SetThreadPriority(%04x) SUCCEEDED, current priority(%d)\n", GetCurrentThreadId(), GetThreadPriority(_threadHandles[i]));
			}
			else
			{
				printf("ServiceFrame::Failed to SetThreadPriority(%04x), current priority(%d)\n", GetCurrentThreadId(), GetThreadPriority(_threadHandles[i]));
			}
		}
	}

	return true;
}

void ServiceThreadGroup::destroyThreads()
{
	// _delete(_threadHandles);
}

// wait for completion port and process request.
DWORD ServiceThreadGroup::threadProc()
{
	DWORD byteCount;
	
	LPOVERLAPPED overlapped;
	SvcCompletionPortKey* key;
	DWORD readBytes;
	WSABUF wsaBuf;
	DWORD wsaFlag = 0;
	BOOL rc;
	IDialogue* dlg;
	IMainConn* mainConn;
	BOOL bTcp =TRUE;
	while(true) 
	{
		rc = GetQueuedCompletionStatus(_completionPort, &byteCount, 
											(PULONG_PTR )&key, &overlapped, INFINITE);

		if (overlapped == NULL)
		{
			printFault("occurred a error on the completion prot.");
			break;
		}
		SOCKET s;
		if ( key->connType == CONN_TYPE_SOCKET_UDP ) 
		{
			bTcp =FALSE;
			if (!rc || byteCount == 0)
			{
				mainConn = key->mainConn;
				dlg = mainConn->getDialogue();
				mainConn->close();				
//				dlg->onConnectionDestroyed(mainConn);
//				_serviceFrm.getDialogueCreator()->releaseDialogue(dlg);
				_delete(key);
				printDebug("ServiceThreadGroup::threadProc():\t", "GetQueuedCompletionStatus() failed.");
				continue;
			}			
			mainConn = key->mainConn;
			s = (SOCKET )mainConn->getConnId();
			
			dlg = mainConn->getDialogue();
			try
			{
				dlg->onDataGram(mainConn,key->perrInfo,key->buf,byteCount);
			}
			catch (...) 
			{
				printError("ServiceReq::run():\tonRequest() occurred a exception.");
			}
		}
		else
		{
			bTcp = TRUE;
			mainConn = (IMainConn*)key->mainConn;
			IMainConnImpl* mainConn1;
			mainConn1 = (IMainConnImpl*)key->mainConn;
			if (!rc || byteCount == 0)
			{
				dlg = mainConn1->getDialogue();
				mainConn1->close();
				mainConn1->setDialogue(NULL);
				dlg->onConnectionDestroyed(mainConn1);
				_serviceFrm.getDialogueCreator()->releaseDialogue(dlg);
				_delete(key);
				printDebug("ServiceThreadGroup::threadProc():\t", 
					"GetQueuedCompletionStatus() failed.");
				continue;
			}			
			s = (SOCKET )mainConn1->getConnId();
			
			dlg = mainConn1->getDialogue();
			mainConn1->modifyAttribute(MAINCONN_CANNOT_RECV, 0);
			try
			{
				dlg->onRequest(mainConn1, key->buf, byteCount);
			}
			catch(...)
			{
				printError("ServiceReq::run():\tonRequest() occurred a exception.");
			}

		}		

		wsaBuf.buf = (char *)key->buf;
		wsaBuf.len = sizeof(key->buf);
		memset(&key->overlapped, 0, sizeof(key->overlapped));
		if (!WSARecv(s, &wsaBuf, 1, &readBytes, &wsaFlag,  
								&key->overlapped, NULL) == 0)
		{

			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				if(bTcp && mainConn )
				{
					dlg = mainConn->getDialogue();
					mainConn->close();
					//mainConn->setDialogue(NULL);					
					dlg->onConnectionDestroyed(mainConn);
					_serviceFrm.getDialogueCreator()->releaseDialogue(dlg);
				}
				_delete(key);
				printDebug("ServiceThreadGroup::threadProc():\t" 	
					"WSARecv() failed.");
			}
		}
	}

	return 0;
}

DWORD __stdcall ServiceThreadGroup::_threadProc(PVOID param)
{
	ServiceThreadGroup* me = (ServiceThreadGroup* )param;
	return me->threadProc();
}

//////////////////////////////////////////////////////////////////////////
class _WriteDataEvent 
{
public:
	_WriteDataEvent()
	{
		_eventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	~_WriteDataEvent()
	{
		CloseHandle(_eventHandle);
	}

	HANDLE		_eventHandle;
};

static _WriteDataEvent writeDataEvent;

class IMainConnImpl2: public IMainConnImpl 
{
public:
	IMainConnImpl2(ServiceFrmBase& frm, SSLSocket& sock):
	  IMainConnImpl(frm, sock)
	{

	}

	virtual int send(IN const void* buf, IN int size)
	{
		DWORD writtenBytes;
		SOCKET s = (SOCKET )_ssock.getSocket();
		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = 
			(HANDLE )((DWORD )writeDataEvent._eventHandle | 0x1);
		WSABUF wsabuf;
		wsabuf.buf = (char* )buf;
		wsabuf.len = size;
		WSASend(s, &wsabuf, 1, &writtenBytes, 0, &overlapped, NULL);
		DWORD sendBytes;
		if (!GetOverlappedResult((HANDLE)s, &overlapped, &sendBytes, TRUE))
			return SOCKET_ERROR;

		return sendBytes;
	}

	virtual int sendTimeout(IN const void* buf, IN int size, IN int timeo)
	{
		DWORD writtenBytes;
		SOCKET s = (SOCKET )_ssock.getSocket();
		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = 
			(HANDLE )((DWORD )writeDataEvent._eventHandle | 0x1);
		WSABUF wsabuf;
		wsabuf.buf = (char* )buf;
		wsabuf.len = size;
		WSASend(s, &wsabuf, 1, &writtenBytes, 0, &overlapped, NULL);
		DWORD sendBytes;
		if (WaitForSingleObject((HANDLE)s, timeo) != WAIT_OBJECT_0) {
			return SOCKET_ERROR;
		}

		if (!GetOverlappedResult((HANDLE)s, &overlapped, &sendBytes, FALSE))
			return SOCKET_ERROR;

		return sendBytes;
	}
};


//////////////////////////////////////////////////////////////////////////
// class ServiceFrmWin32Special

ServiceFrmWin32Special::ServiceFrmWin32Special()
:_socketIpv4(this,APP_TYPE_RTSP),_socketIpv6(this,APP_TYPE_RTSP),
_sockLscUdpIpv4(this,APP_TYPE_LSCP),_sockLscUdpIpv6(this,APP_TYPE_LSCP),
_sockLscIpv6(this,APP_TYPE_LSCP),_sockLscIpv4(this,APP_TYPE_LSCP)
{
    _serviceGroup = NULL;
}

ServiceFrmWin32Special::~ServiceFrmWin32Special()
{

}

bool ServiceFrmWin32Special::init(const ServiceConfig* cfg)
{
	if (cfg->_cfg_isSecure) 
	{
		printFault("ServiceFrmWin32Special::init():\t"
			"couldn't support secure at current version.\n");
		return false;
	}
	
	if (!ServiceFrmBase::init(cfg))
		return false;

	if (_serviceGroup == NULL) 
	{
		_serviceGroup = createThreadGroup(_threadCount, _threadPriority, _recvBufSize);
		if (_serviceGroup == NULL) 
		{
			printFault("ServiceFrmWin32Special::init():\t"
				"createThreadGroup() failed.\n");
			return false;
		}
	}

	SetProcessWorkingSetSize(GetCurrentProcess(), _minWorkingSet, 0x28000000);
	_serviceGroup->start();

	return true;
}

void ServiceFrmWin32Special::uninit()
{
	_socketIpv6.close();
	_socketIpv4.close();
	
	_sockLscIpv6.close();
	_sockLscIpv4.close();
	
	_sockLscUdpIpv6.close();
	_sockLscUdpIpv4.close();

	if (_serviceGroup) 
	{
		_serviceGroup->stop();
		_delete(_serviceGroup);
		_serviceGroup = NULL;
	}

	ServiceFrmBase::uninit();
}

bool	ServiceFrmWin32Special::bindRtsp(unsigned short port)
{
	BOOL	bIPv4OK			= TRUE;
	BOOL	bIPv6Ok			= TRUE;
	char	szPort[32];
	sprintf(szPort,"%u",port);
	if(!_socketIpv6.InitializeSocket("::",szPort,AF_INET6,false))
	{
		printWarning("ServiceFrmWin32Special::bindRtsp(): initialize ipv6 socket failed with port [%s]",szPort);
		bIPv6Ok = FALSE;
	}
	
	if(!_socketIpv4.InitializeSocket("0",szPort,AF_INET,false))
	{
		printWarning("ServiceFrmWin32Special::bindRtsp(): initialize ipv4 socket failed with port [%s]",szPort);
		bIPv4OK = FALSE;
	}
	
	if (!bIPv4OK && ! bIPv6Ok) 
	{
		printError("ServiceFrmWin32Special::bindRtsp() intialize socket failed");
		return false;
	}
	if(bIPv4OK)
		_socketIpv4.start();
	if(bIPv6Ok)
		_socketIpv6.start();
	return true;
}

bool	ServiceFrmWin32Special::bindLscp(unsigned short port)
{
	BOOL	bLscIPv6OK		= TRUE;
	BOOL	bLscIPv4OK		= TRUE;
	BOOL	bLscUDPV6Ok		= TRUE;
	BOOL	bLscUDPV4OK		= TRUE;
	
	char	szPort[32];
	///initialize LSCP
	sprintf(szPort,"%u",port);
	
	if (!_sockLscIpv4.InitializeSocket ("0",szPort,AF_INET,false))
	{
		printWarning("ServiceFrmWin32Special::begin(): initialize ipv4 TCP socket for LSCP failed with port [%s]",szPort);
		bLscIPv4OK = FALSE;
	}
	if (!_sockLscIpv6.InitializeSocket ("::",szPort,AF_INET6,false)) 
	{
		printWarning("ServiceFrmWin32Special::begin(): initialize ipv6 TCP socket for LSCP failed with port [%s]",szPort);
		bLscIPv6OK = FALSE;
	}
	if (!_sockLscUdpIpv4.InitializeSocket ("0",szPort,AF_INET,false)) 
	{
		printWarning("ServiceFrmWin32Special::begin(): initialize ipv6 UDP socket for LSCP failed with port [%s]",szPort);
		bLscUDPV4OK = FALSE;
	}
	if (!_sockLscUdpIpv6.InitializeSocket ("::",szPort,AF_INET6,false)) 
	{
		printWarning("ServiceFrmWin32Special::begin(): initialize ipv6 UDP socket for LSCP failed with port [%s]",szPort);
		bLscUDPV6Ok = FALSE;
	}
	
	if ( !( bLscIPv4OK||
		bLscIPv6OK ||
		bLscUDPV4OK ||
		bLscUDPV6Ok)) 
	{
		printError("ServiceFrmWin32Special::begin() intialize socket failed");
		return false;		
	}
	
	if (bLscIPv4OK)
		_sockLscIpv4.start();
	if (bLscIPv6OK) 	
		_sockLscIpv6.start ();	
	
	return true;

}

bool ServiceFrmWin32Special::begin()
{
	if (_running)
	{
		printFault("ServiceFrmWin32Special::begin(): service is running.");
		SF_ASSERT(false);
		return false;
	}
	_running = true;
	return true;
}

ServiceThreadGroup* ServiceFrmWin32Special::createThreadGroup(uint32 threadCount, uint32 threadPriority, uint32 recvBufSize)
{
	return new ServiceThreadGroup(*this, threadCount, threadPriority, recvBufSize);
}

bool ServiceFrmWin32Special::processConn(IMainConn* conn,SvcCompletionPortKey* key)
{
	IDialogue* dlg;
	dlg = _dlgCtor->createDialogue(key->appType);
	if (dlg == NULL) 
	{
		printFault("ServiceFrmWin32Special::processConn(): "
			"_dlgCtor->createDialogue() failed.");
		_delete(conn);
		return false;
	}
	if (key->connType == CONN_TYPE_SOCKET_TCP) 
	{
		IMainConnImpl * connImpl = (IMainConnImpl*)conn;
		connImpl->setDialogue(dlg);
		connImpl->onConnected();
		dlg->onConnected(connImpl);
	}
	else if( key->connType == CONN_TYPE_SOCKET_UDP )
	{
		IMainConnUDPImpl * connImpl = (IMainConnUDPImpl*)conn;
		connImpl->setDialogue(dlg);
		connImpl->onConnected();
		dlg->onConnected(connImpl);
	}
	
	return _serviceGroup->requestThread(conn,key);

}

IMainConnImpl* ServiceFrmWin32Special::createMainConn(SSLSocket* sock)
{
	assert(sock);
	return new IMainConnImpl(*this, *sock);
}

// special implementation that base on Win32 IO completion port
//////////////////////////////////////////////////////////////////////////
#else // #ifdef _WIN32_SPECIAL_VERSION
//////////////////////////////////////////////////////////////////////////
// Generic implementation

typedef NativeThreadPool ThreadPool;

/* ++
class:
	ServiceReq
Desc:
	线程池请求对象, 代表一个客户请求的执行绪.
-- */

class ServiceReq : public ThreadRequest {
	friend class ThreadMgr;
	friend class ServiceFrmBase;
public:
	ServiceReq(ThreadMgr& threadMgr, ServiceFrmBase& mgr);
	virtual ~ServiceReq();


	/// get count of connection in the this request object.
	int getConnCount();

	/// test overload
	bool isOverload();

protected:
	/// method of super class
	virtual int run();
	virtual bool init(void);
	virtual void final(int retcode = 0, bool bCancelled = false);

	/// add a connection into current req
	/// call by ServiceFrmBase
	int addConn(IMainConnImpl& conn);

	/// clear invalide connection
	int cleanError();

	/// cleck the timeout interval of all connection
	int checkTimeout();

	// int removeConn(IMainConnImpl* conn);	
	
protected:
	PollSSLSocket	_pollSSocks[MAX_CONN_PER_REQ];
	IMainConnImpl*	_mainConns[MAX_CONN_PER_REQ];
	size_t			_pollSSockCount;
	
	ServiceFrmBase&		_service;
	ThreadMgr&		_threadMgr;
	bool			_isOver;
	bool			_lastTimeo;

	//////////////////////////////////////////////////////////////////////////
	
#if !defined(_SPEED_VERSION)
public:
	long			_reqID;
	long			_processTime;

	/// dump this object
	void dump()
	{
		printDebug("ServiceReq dump: _pollSSockCount = %d, _isOver = %d", 
			_pollSSockCount, _isOver);
	}

protected:
#endif // #if !defined(_SPEED_VERSION)

};

/// the thread manager, it's base on thread pool
class ThreadMgr: public ThreadPool {
	friend class ServiceReq;
public:
	ThreadMgr(ServiceFrmBase& service, uint16 threadCount);

	/// insert a request into the thread pool
	bool requestThread( IMainConn* conn , SvcCompletionPortKey* newKey );

	/// get the count of thread in thread pool
	int getThreadCount();

	/*
	/// cancel a request.
	bool cancel(ServiceReq* req);
  
	*/
	/// cancel all request in the thread manager
	// void cancelAll();

protected:
	/// 请求开始处理时通知
	virtual void onRequestBegin(ServiceReq* req);

	/// 请求处理完成是通知
	virtual void onRequestEnd(ServiceReq* req);

	/// 获取一个请求对象, 用于负载客户端的连接
	ServiceReq* obtainRequest(bool& isNew);

	/// 获取当前正在处理的请求数
	size_t getThreadRequestCount();

	/// 将一个线程请求加入到池中
	void addReq(ServiceReq* req);

	/// 将一个请求从线程池中删除
	void delReq(ServiceReq* req);
protected:
	ServiceFrmBase&		_service;

	typedef std::vector<ServiceReq* > SrvReqVec;
	SrvReqVec		_reqs;
	Mutex			_reqsMutex;
	int				_nextReq;
};

//
// class ServiceReq
//

ServiceReq::ServiceReq(ThreadMgr& threadMgr, ServiceFrmBase& mgr) :
	ThreadRequest(threadMgr), _threadMgr(threadMgr), _service(mgr)
{

#if !defined(_SPEED_VERSION)
	static volatile long _sID =0;
	_reqID = InterlockedIncrement(&_sID);
	printDebug("ServiceReq::ServiceReq():\tReqID = %d", _reqID);
	_processTime = GetTickCount();
#endif

	threadMgr.addReq(this);
	_isOver = false;
	_pollSSockCount = 0;
	_lastTimeo = true;
}

ServiceReq::~ServiceReq()
{
	_threadMgr.delReq(this);
}

int ServiceReq::run()
{
	byte buf[MAX_RECV_BUFF];
	int size;

	timeval t;

#if !defined(_SPEED_VERSION)
	if (_pollSSockCount == 0) {
		_isOver = true;
		assert(false);
		return 0;
	}
#endif
	
	// calculate the dynamic value of time out
	if (_lastTimeo)
		MS2TV(t, sf_max_polltime / MAX_CONN_PER_REQ * _pollSSockCount);
	else
		MS2TV(t, 0);

	int result = SSLSocket_poll(_pollSSocks, _pollSSockCount, &t);
	
	// error
	if (result < 0) {
		cleanError();
		if (_pollSSockCount == 0)
			_isOver = true;
		return result;
	}
	
	if (result > 0) {
		_lastTimeo = false;
		// cleanError();
		for (size_t i = 0; i < _pollSSockCount; i ++) {
			PollSSLSocket* pollSSock = &_pollSSocks[i];
			assert(pollSSock->revents != SSL_POLLEXCEPTION);
			
			if (pollSSock->revents == SSL_POLLREAD) {

				IMainConnImpl* mainConn = _mainConns[i];
				memset(buf, 0 , sizeof(buf));

				// poll 函数确定已有数据
				size = mainConn->recv(buf, sizeof(buf));
				// size = mainConn->recvTimeout(buf, sizeof(buf), 
				//	SF_RECV_TIMEOUT);

				if (size < 0) {

					// mark it, and delete it in checkError
					pollSSock->revents |= SSL_POLLEXCEPTION;
					// assert(false);

				} else if (size > 0) {

					IDialogue* dlg = mainConn->getDialogue();
					if (dlg) {

						// 阻止在处理时, 服务器内向客户发送信息.
						mainConn->modifyAttribute(MAINCONN_CANNOT_RECV, 0);

						try {

							dlg->onRequest(mainConn, buf, size);
						} catch(...) {

							printDebug(
								"ServiceReq::run():\tonRequest() "
								"occurred a exception.");
						}

						mainConn->modifyAttribute(0, MAINCONN_CANNOT_RECV);
					}

					if (!mainConn->isActive()) {

						// mark it, and delete it in checkError
						pollSSock->revents |= SSL_POLLEXCEPTION;
					}

				} else { // size = 0, connection is closed

					pollSSock->revents |= SSL_POLLEXCEPTION;
					mainConn->close();
				}
			}			
		}

	} else { // result == 0
		_lastTimeo = true;
	}

	// 最后检查一遍错误与超时
	cleanError();
	checkTimeout();

	// have no connection
	if (_pollSSockCount == 0)
		_isOver = true;

	return result;
}

bool ServiceReq::init(void)
{
	_threadMgr.onRequestBegin(this);

#if !defined(_SPEED_VERSION)
	printDebug("ServiceReq::init():\tReqID = %d, processTime = %d", 
		_reqID, GetTickCount() - _processTime);
	_processTime = GetTickCount();
#endif

	return true;
}

void ServiceReq::final(int retcode /* = 0 */, 
					   bool bCancelled /* = false */)
{
#if !defined(_SPEED_VERSION)
	printDebug("ServiceReq::final():\tReqID = %d, processTime = %d" , 
		_reqID, GetTickCount() - _processTime);
	_processTime = GetTickCount();
#endif
	
	/// 一次请求流程已经完成, 通知 thread manager.
	_threadMgr.onRequestEnd(this);
	if (_isOver) {
		printDebug("ServiceReq::final():\treq is over(this = %p)", this);
		_delete(this);
	} else {
#ifdef	_DEBUG
		if (IsBadReadPtr(this, 1))
			DebugBreak();
#endif

		_threadMgr.pushRequest(*this);
	}

}

int ServiceReq::addConn(IMainConnImpl& conn)
{
	ZQ::common::MutexGuard(*this);
	assert(_pollSSockCount < MAX_CONN_PER_REQ);
	if (_pollSSockCount == MAX_CONN_PER_REQ - 1)
		return 0;
	
	_mainConns[_pollSSockCount] = &conn;
	INIT_POLLSSOCK(&_pollSSocks[_pollSSockCount], 
		&conn.getSSLSocket(), SSL_POLLREAD);

	_pollSSockCount ++;
	
	return _pollSSockCount;
}

int ServiceReq::getConnCount()
{
	return _pollSSockCount;
}

bool ServiceReq::isOverload()
{
	return _pollSSockCount == MAX_CONN_PER_REQ - 1;
}

int ServiceReq::cleanError()
{
	common::MutexGuard(*this);

	/// 扫描当前连接列表, 删除已经出错的连接
	IDialogueCreator* dlgctor = _service.getDialogueCreator();

	for (size_t i = 0; i < _pollSSockCount; i ++) {
		if (_pollSSocks[i].revents & SSL_POLLEXCEPTION) {

			IMainConnImpl* mainConn = _mainConns[i];
			assert(mainConn);
			mainConn->close();

			IDialogue* dlg = mainConn->getDialogue();
			if (dlg) {

				mainConn->setDialogue(NULL);
				
				// onConnectionDestroyed 完成后 mainConn 可能已经释放
				dlg->onConnectionDestroyed(mainConn);

				// releaseDialogue 后 dlg 不可用
				dlgctor->releaseDialogue(dlg);
			}
			
			if (i < _pollSSockCount - 1) {
				int lastCount = (_pollSSockCount - i - 1);
				memmove(&_mainConns[i], &_mainConns[i + 1], 
					sizeof(_mainConns[i]) * lastCount);

				memmove(&_pollSSocks[i], &_pollSSocks[i + 1], 
					sizeof(_pollSSocks[i]) * lastCount);
				i --;
			}

			_pollSSockCount --;
		}
	}

	return _pollSSockCount;
}

int ServiceReq::checkTimeout()
{
	common::MutexGuard(*this);
	IDialogueCreator* dlgctor = _service.getDialogueCreator();
	IMainConnImpl* mainConn;

	for (size_t i = 0; i < _pollSSockCount; i ++) {

		mainConn = _mainConns[i];

		if (mainConn->getIdleTime() >= sf_idle_timeout) {

			IDialogue* dlg = mainConn->getDialogue();
			if (dlg) {

				dlg->onIdlenessTimeout(mainConn);
				if (!mainConn->isActive()) {
					mainConn->setDialogue(NULL);
					
					// onConnectionDestroyed 完成后 mainConn 可能已经释放
					dlg->onConnectionDestroyed(mainConn);

					// releaseDialogue 后 dlg 不可用
					dlgctor->releaseDialogue(dlg);					
				} else {
					// 没有关闭, 更新 idle time
					mainConn->updateIdleTime();
					continue;
				}

			}
			
			if (i < _pollSSockCount - 1) {
				int lastCount = (_pollSSockCount - i - 1);
				memmove(&_mainConns[i], &_mainConns[i + 1], 
					sizeof(_mainConns[i]) * lastCount);

				memmove(&_pollSSocks[i], &_pollSSocks[i + 1], 
					sizeof(_pollSSocks[i]) * lastCount);
				i --;
			}
			
			_pollSSockCount --;

		}
	} // end for
	
	return 0;
}


//
// class ThreadMgr
//

ThreadMgr::ThreadMgr(ServiceFrmBase& service, uint16 threadCount):
	_service(service), ThreadPool(threadCount)
{
	_nextReq = 0;
}

ServiceReq* ThreadMgr::obtainRequest(bool& isNew)
{
	common::MutexGuard guard(_reqsMutex);

	// 在请求数小于线程数时总是新建请求
	if ((size_t )size() < _reqs.size()) {
		if ((size_t )_nextReq >= _reqs.size())
			_nextReq = 0;

		SrvReqVec::iterator itor;
		itor = _reqs.begin() + _nextReq;
		if (!(*itor)->isOverload()) {
			isNew = false;
			_nextReq ++;
			return *itor;
		}

		_nextReq = 0;
		for (itor = _reqs.begin(); itor != _reqs.end(); itor ++) {
			if (!(*itor)->isOverload()) {
				isNew = false;
				_nextReq ++;
				return *itor;
			}
			_nextReq ++;
		}
	}
	
	ServiceReq* req = new ServiceReq(*this, _service);
	printDebug("ThreadMgr::obtainRequest(): new req = %p, req count = %d", 
		req, _reqs.size());
	isNew = req != NULL;
	return req;
}

void ThreadMgr::addReq(ServiceReq* req)
{
	common::MutexGuard guard(_reqsMutex);
	_reqs.push_back(req);
}

void ThreadMgr::delReq(ServiceReq* req)
{
	common::MutexGuard guard(_reqsMutex);
	SrvReqVec::iterator itor;
	for (itor = _reqs.begin(); itor != _reqs.end(); itor ++) {
		if (*itor == req) {
			_reqs.erase(itor);
			break;
		}
	}
}

size_t ThreadMgr::getThreadRequestCount()
{
	common::MutexGuard guard(_reqsMutex);
	return _reqs.size();
}

bool ThreadMgr::requestThread(IMainConn* conn,SvcCompletionPortKey* newKey)
{
	MutexGuard guard(*this);
	bool isNew;
	ServiceReq* req;
	while (true) 
	{
		req = obtainRequest(isNew);
		if (req == NULL)
			return false;

		if (req->addConn(*conn))
			break;
	}
	
	if (isNew) 
	{
#ifdef	_DEBUG
		if (IsBadReadPtr(this, 1))
			DebugBreak();

		if (IsBadReadPtr(req, 1))
			DebugBreak();
#endif
		// req->start();
		pushRequest(*req);
	}

	return true;
}

int ThreadMgr::getThreadCount()
{
	return this->size();
}

void ThreadMgr::onRequestBegin(ServiceReq* req)
{
#if !defined(_SPEED_VERSION)
	printDebug("ThreadMgr::onRequestBegin(): Req = %d", req->_reqID);
#endif
}

void ThreadMgr::onRequestEnd(ServiceReq* req)
{

#if !defined(_SPEED_VERSION)
	printDebug("ThreadMgr::onRequestBegin(): Req = %d", req->_reqID);
#endif
}

/*
bool ThreadMgr::cancel(ServiceReq* req)
{
	SF_ASSERT(false);
	printDebug("ThreadMgr::cancel(): no implementation.");
	return false;
}
*/

/*
void ThreadMgr::cancelAll()
{
	printDebug("ThreadMgr::cancelAll().");
	_requests.empty();
}
*/

//////////////////////////////////////////////////////////////////////////
// class ServiceFrmGeneric

ServiceFrmGeneric::ServiceFrmGeneric()
{
	_threadMgr = NULL;
}

ServiceFrmGeneric::~ServiceFrmGeneric()
{
	if (_threadMgr)
		_delete(_threadMgr);
}

bool ServiceFrmGeneric::init(const ServiceConfig* cfg)
{
	if (!ServiceFrmBase::init(cfg))
		return false;

	if (_threadMgr == NULL)
		_threadMgr = createThreadMgr();
	if (_threadMgr == NULL) {
		printFault("createThreadMgr() failed.\n");
		return false;
	}

	return true;
}

void ServiceFrmGeneric::uninit()
{
	if (_threadMgr) {
		_delete(_threadMgr);
		_threadMgr = NULL;
	}

	ServiceFrmBase::uninit();
}

ThreadMgr* ServiceFrmGeneric::createThreadMgr()
{
	return new ThreadMgr(*this, _threadCount);
}

bool ServiceFrmGeneric::processConn(IMainConnImpl* conn)
{
	IDialogue* dlg;

	dlg = _dlgCtor->createDialogue();
	if (dlg == NULL) {
		printFault("ServiceFrmGeneric::processConn(): "
			"_dlgCtor->createDialogue() failed.");
		return false;
	}
	conn->setDialogue(dlg);
	conn->onConnected();
	dlg->onConnected(conn);
	return _threadMgr->requestThread(*conn);
}

#endif // #ifdef _WIN32_SPECIAL_VERSION

#ifndef _NO_NAMESPACE
}
#endif // #ifndef _NO_NAMESPACE
