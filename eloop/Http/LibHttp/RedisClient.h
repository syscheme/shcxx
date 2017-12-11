// ===========================================================================
// Copyright (c) 1997, 1998 by
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
// Ident : $Id: RedisClient.h,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define RedisClient class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/RedisClient.h $
// 
// 6     6/10/15 1:53p Hui.shao
// 
// 5     5/20/15 11:17a Hui.shao
// added DEL/KEYS
// 
// 4     1/22/15 10:47a Hui.shao
// 
// 3     1/21/15 5:57p Hui.shao
// tested sadd/srem/smembers
// 
// 2     1/19/15 3:45p Hui.shao
// tested messaging
// 
// 1     1/16/15 2:15p Hui.shao
// created
// ===========================================================================

#ifndef __RedisClient_H__
#define __RedisClient_H__

#include "Log.h"
#include "Pointer.h"
#include "eloop_net.h"
#include "NativeThreadPool.h"
#include "TimeUtil.h"
#include "Pointer.h"
#include "Evictor.h"

#include <map>
#include <vector>
#include <queue>

#ifdef ZQ_OS_MSWIN
#  include <winsock2.h>
#  define TIMEOUT_INF ~((timeout_t) 0)
typedef int socklen_t;
#  ifndef ssize_t
#    define ssize_t int
#  endif
#  ifndef timeout_t
typedef DWORD   timeout_t;
#  endif
#else
#ifndef SOCKET
#  define SOCKET int
#endif
#ifndef INVALID_SOCKET
#  define  INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#  define  SOCKET_ERROR	(-1)
#endif
#endif

namespace ZQ{
namespace eloop{

class ZQ_COMMON_API RedisClient;
class ZQ_COMMON_API RedisCommand;
class ZQ_COMMON_API RedisEvictor;

#define DEFAULT_CLIENT_TIMEOUT      (5000) // 5sec
#define DEFAULT_CONNECT_TIMEOUT     (3000) // 3sec
#define MAX_SEND_ERROR_COUNT        (10)
#define MAX_PENDING_REQUESTS        (1000)

#define REDIS_RECV_BUF_SIZE         (64*1024) // 64KB
#define REDIS_DEFAULT_PORT          (6379)

#ifndef FLAG
#  define FLAG(_BIT) (1 << _BIT)
#endif // FLAG
#define REDIS_VERBOSEFLG_SEND_HEX       FLAG(0)
#define REDIS_VERBOSEFLG_RECV_HEX       FLAG(1)
#define REDIS_VERBOSEFLG_TCPTHREADPOOL  FLAG(2)
#define REDIS_VERBOSEFLG_THREADPOOL     FLAG(3)

#define REDIS_IDENT_LOWEST_CHAR         ('!') // =0x21, the first ASCII char can be used in redis-key
#define REDIS_IDENT_HIGHEST_CHAR        ('~') // =0x7e, the last ASCII char can be used in redis-key

#ifdef ZQ_OS_MSWIN // should goes into Socket.h
#define errorno()  WSAGetLastError() // should goes into Socket.h
#else
#define errorno()  (errno)
#endif //

typedef std::map < std::string, std::string > PropertyMap;
typedef std::vector < std::string > StringList;

// -----------------------------
// class RedisSink
// -----------------------------
class RedisSink : virtual public ZQ::common::SharedObject
{
	friend class RedisCommand;

public:
    typedef ZQ::common::Pointer < RedisSink > Ptr;
	virtual ~RedisSink() {}

	typedef enum _RTSPResultCode
	{
		rdeOK                       = 200,
		rdeClientError              = 400,
		rdeNil                      = 404,
		rdeConnectError,
		rdeSendError,
		rdeRequestTimeout,
		rdeClientCanceled,

		rdeServerReturnedError      =500,
		rdeProtocolMissed,

	} Error;

	static const char* err2str(Error err);

	// typedef std::vector< uint8 > Bulk;
	typedef std::string Bulk;
	typedef std::vector< Bulk > Multibulk;

	typedef struct _Data
	{
		char type;
		int  integer;
		Multibulk bulks;
	} Data;

protected:
	virtual void OnRequestError(RedisClient& client, RedisCommand& cmd, Error errCode, const char* errDesc=NULL) =0;
	virtual void OnReply(RedisClient& client, RedisCommand& cmd, Data& data) =0;
};

// -----------------------------
// class RedisCommand
// -----------------------------
class RedisCommand : public RedisSink
{
    friend class RedisEvictor;
public:
	typedef ZQ::common::Pointer < RedisCommand > Ptr;
	virtual ~RedisCommand() {}

	std::string desc();
	bool wait(int timeout);

protected:
	friend class RedisClient;
	friend class RequestErrCmd;
	friend class ReplyDispatcher;

	RedisCommand(RedisClient& client, const std::string& cmdstr, char replyType, RedisSink::Ptr cbRedirect=NULL);

	virtual void OnRequestError(RedisClient& client, RedisCommand& cmd, Error errCode, const char* errDesc=NULL);
	virtual void OnReply(RedisClient& client, RedisCommand& cmd, Data& data);

	RedisClient& _client;
	std::string _command;
	char        _replyType;
	RedisSink::Ptr _cbRedirect;
	int64       _stampCreated, _stampSent, _stampReceived;

	typedef struct _ReplyContext
	{
		int  errCode;
		Data data;
		bool isBulkPayload;
		int  nBulksLeft;
	} ReplyContext;

	ReplyContext _replyCtx;

	ZQ::common::Event::Ptr _pEvent; // for sync call
};

// -----------------------------
// class RedisClient
// -----------------------------
// represent a tcp connection to the RTSP server, may be shared by multiple RTSPSessions
class RedisClient : public ZQ::eloop::TCP, virtual public ZQ::common::SharedObject
{
	friend class RedisCommand;
	friend class RequestErrCmd;
	friend class ReplyDispatcher;
    friend class RedisEvictor;
public:
	typedef ZQ::common::Pointer < RedisClient > Ptr;

    enum TCPStatus
    {
        unConnect = 0,
        Connecting,
        Connected,
    };

    RedisClient(ZQ::eloop::Loop& loop, ZQ::common::Log& log, ZQ::common::NativeThreadPool& thrdpool, const std::string& server="localhost", unsigned short serverPort=REDIS_DEFAULT_PORT, ZQ::common::Log::loglevel_t verbosityLevel =ZQ::common::Log::L_DEBUG);
    virtual ~RedisClient();

	uint16	getVerbosity() { return (ZQ::common::Log::loglevel_t)_log.getVerbosity() | (_verboseFlags<<8); }
	void    setVerbosity(uint16 verbose = (0 | ZQ::common::Log::L_ERROR)) { _log.setVerbosity(verbose & 0x0f); _verboseFlags =verbose>>8; }

	void setClientTimeout(int32 connectTimeout =DEFAULT_CONNECT_TIMEOUT, int32 messageTimeout =DEFAULT_CLIENT_TIMEOUT);
	void disconnect();

	RedisSink::Error lastSyncCallError() const { return _lastErr; }

	static int encode(std::string& output, const void* source, size_t len=-1);
	static int decode(const char* source, void* target, size_t maxlen);

protected:

	// overridding of TCPSocket
	virtual void OnConnected(ElpeError status);
	virtual void OnError();
	virtual void OnRead(ssize_t nread, const char *buf);
    virtual void OnWrote(int status);
	virtual void OnTimeout();

	// new  callbacks
	virtual int  OnRequestPrepare(RedisCommand::Ptr& pReq) { return 0; }
	virtual void OnRequestClean(RedisCommand& req) {}

public: // Redis commands

	// async sending commands
	RedisCommand::Ptr sendPING(RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendMONITOR(RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendINFO(RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendSLAVEOF(const char *host, int port, RedisSink::Ptr reply=NULL);

	RedisCommand::Ptr sendSET(const char *key, const uint8* val, int vlen =-1, RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendGET(const char *key, RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendGETSET(const char *key, const uint8* val, int vlen =-1, RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendDEL(const char *key, RedisSink::Ptr reply=NULL);

	RedisCommand::Ptr sendKEYS(const char *pattern, RedisSink::Ptr reply=NULL);

	RedisCommand::Ptr sendSADD(const char *key, const char *member, int vlen=-1, RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendSREM(const char *key, const char *member, int vlen=-1, RedisSink::Ptr reply=NULL);
	RedisCommand::Ptr sendSMEMBERS(const char *key, RedisSink::Ptr reply=NULL);

	// sync commands with result parsed
	RedisSink::Error PING();
	RedisSink::Error INFO(PropertyMap& props);
	RedisSink::Error SLAVEOF(const std::string& host, int port);

	RedisSink::Error SET(const std::string& key, const uint8 *value, int vlen =-1);
	RedisSink::Error GET(const std::string& key, uint8* value, uint& vlen);
	RedisSink::Error GETSET(const char *key, const char *val, int vlen =-1, RedisSink::Ptr reply=NULL);
	RedisSink::Error DEL(const std::string& key);

	RedisSink::Error KEYS(const std::string& pattern, StringList& keys);

	RedisSink::Error SADD(const std::string& key, const std::string& member);
	RedisSink::Error SREM(const std::string& key, const std::string& member);
	RedisSink::Error SMEMBERS(const std::string& key, StringList& members);

private:
	RedisCommand::Ptr sendSETAddRm(const char *cmd, const char *key, const char *member, int vlen, RedisSink::Ptr reply);
	RedisCommand::Ptr sendMultikeyBulkCmd(const char *cmd, std::vector<std::string>& keys, RedisSink::Ptr reply);

protected:
	ZQ::common::NativeThreadPool&  _thrdpool;
	ZQ::common::LogWrapper		  _log;

	RedisSink::Error  _lastErr;

	// about the connection
	std::string      _serverIp;
	unsigned short         _serverPort;
	int32            _connectTimeout;
	int32            _messageTimeout;
	TCPStatus        _tcpStatus;

	ZQ::common::AtomicInt        _sendErrorCount;

	char             _connDesc[128];
	// about the out-going and await queues
	typedef std::queue <RedisCommand::Ptr> CommandQueue;
	CommandQueue _commandQueueToSend, _commandQueueToReceive;
	ZQ::common::Mutex _lockCommandQueue;

	// send a request thru the connection
	//@return CSeq num in [1, MAX_CLIENT_CSEQ) if succeeded, or negative if failed
	RedisCommand::Ptr sendCommand(const std::string& command, char replyType, RedisSink::Ptr cbReply);
	RedisCommand::Ptr sendCommand(RedisCommand::Ptr pCmd);

	int _sendLine(const std::string& cmdline);
	static char* _nextLine(char* startOfLine, int maxByte, int minLen =0);

	char     _inCommingBuffer[REDIS_RECV_BUF_SIZE];
	int	     _inCommingByteSeen;

	/// increase last CSeq and then return the new CSeq in the range [1, MAX_CLIENT_CSEQ)
	uint lastCSeq();

private:
	void   _cancelCommands(); // private use, no mutex protection
	uint16 _verboseFlags;
};

// -----------------------------
// class RedisEvictor
// -----------------------------
class RedisEvictor : public ZQ::common::Evictor, public RedisSink
{
public:
    struct LocateReply
    {
        ZQ::common::Event::Ptr             _pEvent;
        ZQ::common::Evictor::Item::Ptr     _pItem;
        ZQ::common::Evictor::Ident         _ident;

        LocateReply(){}
    };

    RedisEvictor(ZQ::common::Log& log, RedisClient::Ptr client, const std::string& name, const ZQ::common::Evictor::Properties& props = Properties());
	virtual ~RedisEvictor();

	RedisClient::Ptr getClient() { return _client; }

protected: // overwrite of Evictor

	// save a batch of streamed object to the target object store
	int saveBatchToStore(ZQ::common::Evictor::StreamedList& batch);

	// load a specified object from the object store
	//@ return IOError, NotFound, OK
	Evictor::Error loadFromStore(const std::string& key, ZQ::common::Evictor::StreamedObject& data);

public: // the child class inherited from this evictor should implement the folloing method
    //don't check exist. now StreamEngine only use add.
    virtual ZQ::common::Evictor::Item::Ptr add(const ZQ::common::Evictor::Item::ObjectPtr& obj, const ZQ::common::Evictor::Ident& ident);
    
    virtual ZQ::common::Evictor::Item::ObjectPtr locate(const ZQ::common::Evictor::Ident& ident);

protected:
    virtual ZQ::common::Evictor::Item::Ptr pin(const ZQ::common::Evictor::Ident& ident, ZQ::common::Evictor::Item::Ptr item = NULL);

	// marshal a servant object into a byte stream for saving to the object store
	virtual bool marshal(const std::string& category, const ZQ::common::Evictor::Item::Data& data, ZQ::common::Evictor::ByteStream& streamedData) { return false; }

	// unmarshal a servant object from the byte stream read from the object store
	virtual bool unmarshal(const std::string& category, ZQ::common::Evictor::Item::Data& data, const ZQ::common::Evictor::ByteStream& streamedData) { return false; }

protected:
    virtual void OnRequestError(RedisClient& client, RedisCommand& cmd, RedisSink::Error errCode, const char* errDesc=NULL);
    virtual void OnReply(RedisClient& client, RedisCommand& cmd, Data& data);
protected:
	RedisClient::Ptr                _client;
	size_t                          _maxValueLen;
    uint8*                          _recvBuf;       //$ only single string;

    std::queue<LocateReply>         _lcQueue;
    ZQ::common::Mutex               _lockLocateQueue;
};

}}//endof namespace

// brief usage:
//    RedisClient client(logger, threadpool, bindAddress, "rtsp://192.168.11.22");
//    RTSPSession::Ptr sess = new RTSPSession(logger, threadpool, "udp://@223.12.12.12:1234", "/aaa.mpg");
//    client.sendSETUP(*sess);
//    ... wait for the overwritten of RTSPSession::OnResponse_SETUP()
//    client.sendPLAY(*sess);
//    ... wait
//    client.sendTEARDOWN(*sess);
//    sess->destroy(); //must call to destroy the session

#endif // __RedisClient_H__
