#ifndef _SERVICEFRAME_H_
#define _SERVICEFRAME_H_
#if _MSC_VER > 1000
#pragma once
#endif // #if _MSC_VER > 1000

#include "NativeThread.h"
#include "NativeThreadPool.h"
#include "XMLPreferenceEx.h"
#include "rwlock.h"
#include <vector>
#include <string>


#include <WS2tcpip.h>
	//#include <WinSock2.h>
#include <time.h>
// #include <assert.h>


#include <ssllib.h>
#include <list>

#ifndef _NO_NAMESPACE
namespace ZQ {
#endif // #ifndef _NO_NAMESPACE

using namespace common;
//////////////////////////////////////////////////////////////////////////
// implemented by SERVICE FRAME

#define MAX_RECV_BUFF				16384
#define SF_IDLE_TIMEOUT				300000	/* 5 minute */
// #define SF_IDLE_TIMEOUT				30000	/* 30 second */
#define	MAX_CONN_PER_REQ			FD_SETSIZE
#define CONN_TYPE_SOCKET_TCP		0
#define CONN_TYPE_SOCKET_UDP		1
#define CONN_TYPE_PIPE				2

struct ConnID 
{
	int				type;
	int				addrlen;
	union
	{
		sockaddr_storage	caddr;
		TCHAR				cname[MAX_PATH];
	};
};

struct SocketInfo 
{
	ConnID		addr;	// address of peer
	tm			ct;		// time of connection
};
typedef SocketInfo PeerInfo;
typedef SocketInfo LocalInfo;

struct IConn 
{
	virtual ~IConn(){}
	/// receive a chuck of data from this connection
	///@param[out] buf the buf to receive data
	///@param[in] maxlen the max length in byte that the buffer can receive data
	///@return number of bytes have been received
	virtual int recv(OUT void* buf, IN int size) = 0;

	/// receive a chuck of data from this connection with timeout
	///@param[out] buf		the buf to receive data
	///@param[in] maxlen	the max length in byte that the buffer can receive data
	///@param[in] timeo		timeout interval
	///@return number of bytes have been received
	virtual int recvTimeout(OUT void* buf, IN int size, IN int timeo) = 0;

	/// send a chuck of data thru this connection
	///@param[in] buf data to send
	///@param[in] maxlen the number of bytes to send
	///@return number of bytes have been sent
	virtual int send(IN const void* buf, IN int size) = 0;

	/// send a chuck of data thru this connection with time-out
	///@param[in] buf		data to send
	///@param[in] maxlen	the number of bytes to send
	///@param[in] timeo		timeout interval
	///@return number of bytes have been sent
	virtual int sendTimeout(IN const void* buf, IN int size, IN int timeo) = 0;


	//Add support to UDP
	///@return the total number of bytes received
	///@param buf buffer to hold the incoming data
	///@param size data size in byte
	///@param fromConID peer ConnID
	virtual int recvFrom(OUT char* buf , IN int size , ConnID* fromConID) = 0;

	///@return the total number of bytes sent
	///@param buf data to be sent
	///@param size buffer size in byte
	///@param targetConnID target connID
	virtual int	sendTo(IN char* buf , IN int size ,ConnID* targetConnID) = 0;

	/// close the connection.
	///@return 0 if no error occurs, otherwise SOCKET_ERROR
	virtual int close() = 0;

	/// upgrade the TCP connection to SSL connection
	///param[in] timeout	time-out interval
	virtual int upgradeSecurity(int timeout) = 0;

	/// degread the SSL connection to TCP connection
	virtual int degradeSecurity() = 0;
	
	/// check secure status
	virtual bool isSecure() = 0;

	/// check active flase
	virtual bool isActive() = 0;

	/// release this object
	virtual void release() = 0;

	virtual int getConnId() = 0;
};

#define CONN_MODE_PASSIVE		0
#define CONN_MODE_ACTIVE		1

struct IDialogue;
struct IMainConn: IConn 
{
	virtual ~IMainConn(){}
	/// get peer information
	///@patam[in] info		output the peer information
	virtual int getPeerInfo(OUT PeerInfo* info) = 0;

	///get local information
	virtual int getLocalInfo(OUT LocalInfo* info ) =0;

	/// when the connection becomes active status, this method will call
	virtual void onConnected() = 0;

	/// crate a additive connection for multi channel
	virtual IConn* createConn(IN const ConnID* addr, 
		IN int mode, bool inheritSec, IN int timeout) = 0;

	/// destroy a non-main conneciton
	virtual void destroyConn(IN IConn* conn) = 0;

	/// get dialogue object from main connection
	virtual IDialogue* getDialogue() = 0;
	
	virtual unsigned __int64 getConnectionIdentity() =0;
};

//////////////////////////////////////////////////////////////////////////
// implemented by protocol layer

/// 协议实现层使用该接口与 ServiceFrame 对接
struct IDialogue 
{
	virtual ~IDialogue(){};
	/// when service frame get a income connection, 
	/// it notice the protocol layer
	///@param[IN] conn		new main connection object
	virtual void onConnected(IN IMainConn* conn) = 0;

	/// when a request arrived, service notice the protocol layer
	///@param[in] conn		main connection object who is there a new request
	///@param[in] buf		request package data
	///@param[in] size		size of the request package
	virtual void onRequest(IN IMainConn* conn, IN const void* buf, IN int size) = 0;

	/// when a dialog was timeout
	/// service frame notice the protocal layer
	///@param[in] conn		main connection object
	virtual void onIdlenessTimeout(IN IMainConn* conn) = 0;

	/// when a main connection destroyed, service notice the protocol layer
	///@param[in] conn		main connection object
	virtual void onConnectionDestroyed(IN IMainConn* conn) = 0;

	///received datagram
	///@param conn main connection object,but only local information is available because 
	///it's not a connection oriented
	///@param peer peer information
	///@param buf incoming buffer
	///@param size incoming buffer size in byte
	virtual void onDataGram( IMainConn* conn  ,const PeerInfo& peer, void* buf ,int size ) = 0;
};

enum
{
	APP_TYPE_RTSP,
	APP_TYPE_LSCP
};



struct IDialogueCreator 
{
	virtual ~IDialogueCreator(){}

	/// crate a dialog object.
	///@return the a dialog object
	virtual IDialogue*		createDialogue( int type = APP_TYPE_RTSP ) = 0;


	
	
	/// release a dialog object
	///@param[in] dlg	this dialog object will release
	virtual void			releaseDialogue(IN IDialogue* dlg) = 0;
};


//////////////////////////////////////////////////////////////////////////
// some implementation
#define SVCDBG_LEVEL_MIN				SVCDBG_LEVEL_DISABLE
#define SVCDBG_LEVEL_DISABLE			0
#define SVCDBG_LEVEL_FAULT				1
#define SVCDBG_LEVEL_ERROR				2
#define SVCDBG_LEVEL_WARNING			3
#define SVCDBG_LEVEL_NOTICE				4
#define SVCDBG_LEVEL_DEBUG				5
#define SVCDBG_LEVEL_MAX				SVCDBG_LEVEL_DEBUG

#define printFault						svcdbg_printFault
#define printError						svcdbg_printError
#define printWarning					svcdbg_printWarning
#define printNotice						svcdbg_printNotice
#define printDebug						svcdbg_printDebug

#ifdef _DEBUG
#define	SF_ASSERT(b)					assert(b);
#else
#define	SF_ASSERT(b)
#endif

#ifndef _NOTRACE
void svcdbg_setDebugOstream(ostream& ostrm);
int svcdbg_dumpInternalError();

void svcdbg_printFault(const char* format, ...) PRINTFLIKE(1, 2);
void svcdbg_printError(const char* format, ...) PRINTFLIKE(1, 2);
void svcdbg_printWarning(const char* format, ...) PRINTFLIKE(1, 2);
void svcdbg_printNotice(const char* format, ...) PRINTFLIKE(1, 2);
void svcdbg_printDebug(const char* format, ...) PRINTFLIKE(1, 2);

int svcdbg_setLevel(int level);
int svcdbg_getLevel();
void svcdbg_resetCounts();
int svcdbg_getCount(int level);

#else // #ifndef _NOTRACE

#define svcdbg_setDebugOstream(ostrm)
#define svcdbg_dumpInternalError()			0

#define svcdbg_printFault
#define svcdbg_printError
#define svcdbg_printWarning
#define svcdbg_printNotice
#define svcdbg_printDebug

#define svcdbg_setLevel(n)					(SVCDBG_LEVEL_MAX + 1)
#define svcdbg_getLevel()					(SVCDBG_LEVEL_MAX + 1)
#define svcdbg_resetCounts()
#define svcdbg_getCount(n)					0

#endif // #ifndef _NOTRACE

/// 服务器的配置管理类
class ServiceConfig {
public:
	
	/// 配置信息的数据类型
	struct _ConfigEntry 
	{
		const char* key;
		enum 
		{
			INVALID_TYPE, 
			STRING_VALUE, 
			SHORT_VALUE, 
			USHORT_VALUE, 
			LONG_VALUE,
			ULONG_VALUE,
			DISPATCH_ENTRY, 
		} type;
		
		void* value;
		
		int	maxValue;
	};

	ServiceConfig();
	virtual ~ServiceConfig();

	/// 基类的配置表
	virtual const _ConfigEntry* getBaseConfigMap();

	/// 当前配置表
	virtual const _ConfigEntry* getConfigMap();

	/// 加载一个配置文件
	virtual bool load(const char* fileName,std::vector<std::string>& path=std::vector<std::string>());

protected:
	/// 处理配置文件
	bool processEntry(XMLPreferenceEx* pref, const _ConfigEntry* cfgEntry);
public:
	int32		_cfg_debugLevel;
	uint16		_cfg_isSecure;
	uint16		_cfg_threadCount;
	uint32		_cfg_maxConn;
	uint32		_cfg_recvBufSize;
	uint32		_cfg_threadPriority;
	uint32		_cfg_minWorkingSet;
	char		_cfg_publicKeyFile[MAX_PATH];
	char		_cfg_privateKeyFile[MAX_PATH];
	char		_cfg_privateKeyFilePwd[64];
	char		_cfg_dhParamFile[MAX_PATH];
	char		_cfg_randFile[MAX_PATH];
};

//typedef NativeThread Thread;
class IMainConnImpl;
class ServiceFrmBase;

class ServerTCPSocket : public NativeThread
{
public:	
	ServerTCPSocket(ServiceFrmBase* pFrmBase,int appType = APP_TYPE_RTSP);
	~ServerTCPSocket();
public:
	bool	InitializeSocket(const char* nodeName , const char* servName,int socketFamily , bool bSecure);
	int		run();
	void	close();
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


class ServerUDPSocket //: public NativeThread
{
public:	
	ServerUDPSocket(ServiceFrmBase* pFrmBase , int appType = APP_TYPE_LSCP);
	~ServerUDPSocket();
public:
	bool	InitializeSocket(const char* nodeName , const char* servName,int socketFamily , bool bSecure);
	void	close();
	//int		run(){return 1;}
protected:
	IMainConn*			CreateMainConn();
protected:
	SSLContext					_sslCtx;
	SOCKET						_ssock;	
	addrinfo*					resAddrinfo;
	ServiceFrmBase*				_pFrmBase;
	bool						_secure;
	int							_appType;
};

struct SvcCompletionPortKey
{
	int					appType;//APP_TYPE_RTSP or APP_TYPE_LSCP
	int					connType;//CONN_TYPE_TCP or	CONN_TYPE_UDP
	ConnID				connId;
	SocketInfo			perrInfo;	//Used for UDP
	OVERLAPPED			overlapped;
	IMainConn*			mainConn;
	unsigned char		buf[MAX_RECV_BUFF];
	SOCKET				serverSocket;
};

class ServiceFrmBase: protected NativeThread 
{
	friend class IMainConnImpl;
	friend class ServerTCPSocket;
public:
	ServiceFrmBase();
	virtual ~ServiceFrmBase();

	/// 初始化 service frame
	virtual bool init(const ServiceConfig* cfg);
	virtual void uninit();

	/// 开始服务
	virtual bool begin(unsigned short rtspPort , unsigned short lscpPort=5542);

	/// 结束服务
	virtual bool end();

	/// 得到总共的连接数
	int getMainConnCount();
	
	/// 得到指定的连接
	IMainConn* getMainConn(int index);

	/// dialogCreator 属性
	bool setDialogueCreator(IDialogueCreator* dlgCtor);
	IDialogueCreator* getDialogueCreator();

	/// 服务器的安全属性. 安全被开启时,使用 SSL 作为通信信道
	bool setSecure(bool secure);
	bool isSecure();

	/// 服务器是否以单独的线程开始接收服务
	bool getServiceThread();
	bool setServiceThread(bool b);

	/// 空闲时间检查的间隔
	int getIdleTimeout();
	bool setIdleTimeout(int timeo);
	
	/// 最大连接数
	uint32 getMaxConnection();
	bool setMaxConnection(uint32 maxConn);
	
	/// 线程池的线程数
	uint32 getThreadCount();
	bool setThreadCount(uint16 count);

public:
	// NativeThread methods
	virtual int run(); 

	virtual IMainConnImpl* createMainConn(SSLSocket* sock);

	/// 用于接收来自客户的连接
	IMainConnImpl* accept();

	/// 管理连接列表
	bool addMainConn(IMainConnImpl* conn);
	bool delMainConn(IMainConnImpl* conn);

	/// 处理连接
	virtual bool processConn(IMainConn* conn,SvcCompletionPortKey* newKey) = 0;

	bool initByConfiguration(const ServiceConfig* cfg);

protected:
	vector<IMainConn* >			_connList;
	_rwlock_t					_connListLock;
	uint32						_maxConn;
protected:
//	SSLContext					_sslCtx;
//	SSLSocket					_ssock;

	bool						_secure;	
	IDialogueCreator*			_dlgCtor;
	bool						_running;
	int							_idleTimeout;
	bool						_serviceThread;	
	uint32						_recvBufSize;
	uint32						_threadPriority;
	int32						_acceptConn;
	int32						_threadCount;
	uint32						_minWorkingSet;
	unsigned short				_rtspPort;
	unsigned short				_lscpPort;
};

#ifdef _WIN32_SPECIAL_VERSION

class ServiceThreadGroup;

class ServiceFrmWin32Special: public ServiceFrmBase 
{
friend class ServerTCPSocket;
friend class ServerUDPSocket;
public:
	ServiceFrmWin32Special();
	virtual ~ServiceFrmWin32Special();
	virtual bool init(const ServiceConfig* cfg);
	virtual void uninit();
	virtual bool begin();

	bool	bindRtsp(unsigned short port);
	bool	bindLscp(unsigned short port);

protected:
	virtual IMainConnImpl* createMainConn(SSLSocket* sock);
	virtual ServiceThreadGroup* createThreadGroup(uint32 threadCount, uint32 threadPriority=THREAD_PRIORITY_NORMAL, uint32 recvBufSize=MAX_RECV_BUFF);
	virtual bool processConn( IMainConn* conn , SvcCompletionPortKey* newKey );

protected:
	ServiceThreadGroup*	_serviceGroup;

	ServerTCPSocket		_socketIpv4;
	ServerTCPSocket		_socketIpv6;

	ServerTCPSocket		_sockLscIpv4;
	ServerTCPSocket		_sockLscIpv6;
	ServerUDPSocket		_sockLscUdpIpv6;
	ServerUDPSocket		_sockLscUdpIpv4;
};

typedef ServiceFrmWin32Special ServiceFrm;

#else // #ifdef _WIN32_SPECIAL_VERSION

class ThreadMgr;

class ServiceFrmGeneric: public ServiceFrmBase 
{
	friend class ServiceReq;
public:
	ServiceFrmGeneric();
	virtual ~ServiceFrmGeneric();
	virtual bool init(const ServiceConfig* cfg);
	virtual void uninit();

protected:

	/// 线程管理器的工厂方法
	virtual ThreadMgr* createThreadMgr();
	virtual bool processConn(IMainConn* conn , SvcCompletionPortKey* newKey);

protected:
	ThreadMgr*					_threadMgr;
};

typedef ServiceFrmGeneric ServiceFrm;

#endif // #ifdef _WIN32_SPECIAL_VERSION

#ifndef _NO_NAMESPACE
}
#endif // #ifndef _NO_NAMESPACE

#endif // #ifndef _SERVICEFRAME_H_
