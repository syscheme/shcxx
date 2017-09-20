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
#include "TCPSocket.h"
#include "NativeThreadPool.h"
#include "TimeUtil.h"
#include "Pointer.h"

#include <map>
#include <vector>
#include <queue>

namespace ZQ{
namespace common{

class ZQ_COMMON_API RedisClient;
class ZQ_COMMON_API RedisCommand;

#define DEFAULT_CLIENT_TIMEOUT      (5000) // 5sec
#define DEFAULT_CONNECT_TIMEOUT     (3000) // 3sec
#define MAX_SEND_ERROR_COUNT        (10)
#define MAX_PENDING_REQUESTS        (1000)

#define REDIS_RECV_BUF_SIZE         (128*1024)
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
class RedisSink : virtual public SharedObject
{
	friend class RedisCommand;

public:
	typedef Pointer < RedisSink > Ptr;
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

//	RedisCommand& _req;
//	Data _data;
};

// -----------------------------
// class RedisCommand
// -----------------------------
class RedisCommand : public RedisSink
{
public:
	typedef Pointer < RedisCommand > Ptr;
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

	Event::Ptr _pEvent; // for sync call
};

// -----------------------------
// class RedisClient
// -----------------------------
// represent a tcp connection to the RTSP server, may be shared by multiple RTSPSessions
class RedisClient : public TCPClient, virtual public SharedObject
{
	friend class RedisCommand;
	friend class RequestErrCmd;
	friend class ReplyDispatcher;

public:
	typedef Pointer < RedisClient > Ptr;

	RedisClient(Log& log, NativeThreadPool& thrdpool, const std::string& server="localhost", tpport_t serverPort=REDIS_DEFAULT_PORT, InetHostAddress& bindAddress=InetHostAddress(), Log::loglevel_t verbosityLevel =Log::L_WARNING, tpport_t bindPort=0);
	virtual ~RedisClient();

	static void setVerboseFlags(uint16 flags =0) { _verboseFlags =flags; }
	void setClientTimeout(int32 connectTimeout =DEFAULT_CONNECT_TIMEOUT, int32 messageTimeout =DEFAULT_CLIENT_TIMEOUT);
	void disconnect();

	static int encode(std::string& output, const void* source, size_t len=-1);
	static int decode(const char* source, void* target, size_t maxlen);

protected:

	// overridding of TCPSocket
	virtual void OnConnected();
	virtual void OnError();
	virtual void OnDataArrived();
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
	NativeThreadPool&  _thrdpool;
	LogWrapper		  _log;

	std::string       _lastErr;

	// about the connection
	InetHostAddress  _serverAddress;
	tpport_t         _serverPort;
	int32            _connectTimeout;
	int32            _messageTimeout;

	uint32			 _cTcpStreams;
	AtomicInt        _sendErrorCount;

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
	static uint16 _verboseFlags;
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
