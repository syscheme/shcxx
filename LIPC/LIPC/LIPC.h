// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai, PRC.,
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
// of the above copyright notice.  This  software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: LIPC.h,v 1.8 2017/06/09 09:32:35 zhixiang.zhu Exp $
// Branch: $Name:  $
// Author: zhuzhixiang
// Desc  : Define LIPC class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/LIPC/LIPC.h $
// ===========================================================================

#ifndef __ZQ_COMMON_LIPC_H__
#define __ZQ_COMMON_LIPC_H__

#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#  ifdef LIPC_EXPORTS
#    define ZQ_LIPC_API __EXPORT
#  else
#    define ZQ_LIPC_API __DLLRTL
#  endif
#else
#  define ZQ_LIPC_API
#endif // OS

#include "PipeConnection.h"
#include "eloop/eloop_net.h"

#include <vector>
#include <list>

namespace ZQ {
	namespace LIPC {
class ZQ_LIPC_API Service;
class ZQ_LIPC_API Client;
class ZQ_LIPC_API Request;
class ZQ_LIPC_API Response;
class ZQ_LIPC_API Message;
class ZQ_LIPC_API Handler;

class PipePassiveConn;
class Servant;

// ------------------------------------------------
// class Message
// ------------------------------------------------
class Message
{
public:
	typedef enum _LIPCError
	{
		LIPC_OK = 0,
		LIPC_PARSING_ERROR = -32700, /**< Invalid JSON. An error occurred on the server while parsing the JSON text. */
		LIPC_INVALID_REQUEST = -32600, /**< The received JSON not a valid JSON-RPC Request. */
		LIPC_METHOD_NOT_FOUND = -32601, /**< The requested remote-procedure does not exist / is not available. */
		LIPC_INVALID_PARAMS = -32602, /**< Invalid method parameters. */
		LIPC_INTERNAL_ERROR = -32603, /**< Internal JSON-RPC error. */

		//user define error
		LIPC_NOT_FD	= -33000,
		LIPC_INVALID_FD_TYPE = -33001,

	}LIPCError;

	static const char* errDesc(LIPCError code);

	typedef enum _FdType
	{
		LIPC_UDP,
		LIPC_TCP,
		LIPC_FILE,
		LIPC_NONE,
	}FdType;

	Message(Json::Value msg):_msg(msg){}

	void setErrorCode(LIPCError code);
	void setCSeq(int cseq);
	int getCSeq() const;
	void setFd(int fd);
	void setFdType(FdType type);

	int getFd() const;
	FdType getFdType() const;
	bool hasFd();
	bool empty();
	std::string toString();
	bool fromString(const std::string& str);

protected:
	Json::Value		_msg;
};

// ------------------------------------------------
// class Response
// ------------------------------------------------
class Response:public Message,public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<Response> Ptr;
	Response(Json::Value msg = Json::Value::null):Message(msg){}

	void setResult(const Json::Value& result);
	Json::Value getResult();
};

// ------------------------------------------------
// class Request
// ------------------------------------------------
class Request:public Message,public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<Request> Ptr;
	Request(Json::Value msg = Json::Value::null):Message(msg){}

	typedef void (*RpcCB)(const Response::Ptr& resp,void* data);

	typedef struct _RpcCBInfo{
		RpcCB cb;
		void* data;
		_RpcCBInfo(){cb = NULL;data = NULL;}
	}RpcCBInfo;

	void setMethodName(const std::string& methodName);
	void setCb(RpcCB cb,void* data);
	RpcCBInfo& getCb();
	void setParam(const Json::Value& param);
	Json::Value getParam() const;

private:
	RpcCBInfo			_cbInfo;
};

// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
class Handler
{
public:
	typedef std::map<int,Request::RpcCBInfo> seqToCBInfoMap;

public:
	Handler();
	virtual ~Handler();

	void addcb(int seqId,Request::RpcCB cb,void* data);

	void process(const std::string& msg,PipeConnection& conn);

	uint lastCSeq();

protected:
	virtual void execMethod(const std::string& methodName,const Request::Ptr& req,Response::Ptr& resp){resp->setErrorCode(Message::LIPC_METHOD_NOT_FOUND);}

private:
	Handler(const Handler& obj);
	Handler& operator=(const Handler& obj);

	void invoke(const Json::Value& msg,PipeConnection& conn);

	Json::FastWriter m_writer;
	Json::Reader m_reader;
	seqToCBInfoMap m_seqIds;

	ZQ::common::AtomicInt _lastCSeq;
};

// ------------------------------------------------
// class Service
// ------------------------------------------------
class Service : public Handler,public ZQ::eloop::Pipe
{
public:
	friend class PipePassiveConn;
public:
	typedef std::list< PipePassiveConn* > PipeClientList;

public:
	Service(ZQ::common::Log& log):_lipcLog(log){}
	int init(ZQ::eloop::Loop &loop, int ipc=1);
	PipeClientList& getPipeClientList(){return _ClientList;}

protected:
	ZQ::common::Log& _lipcLog;
	void addConn(PipePassiveConn* conn);
	void delConn(PipePassiveConn* conn);
	
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void onError( int error,const char* errorDescription )
	{	
		_lipcLog(ZQ::common::Log::L_ERROR, CLOGFMT(Service, "errCode = %d,errDesc:%s"),error,errorDescription);
	}

private:
	PipeClientList _ClientList;
	int				_ipc;
};


// ------------------------------------------------
// class Client
// ------------------------------------------------
class Client : public ZQ::LIPC::Handler
{
public:
	friend class Servant;
public:
	Client(ZQ::eloop::Loop &loop,ZQ::common::Log& log,int ipc=1):_loop(loop),_ipc(ipc),_lipcLog(log),_svt(NULL),_reconnect(false){}

	int  init();
	int  bind(const char *name);
	int connect(const char *name);
	ZQ::eloop::Loop& get_loop() const;

	int read_start();
	int read_stop();
	int sendRequest(Request::Ptr req);
	int shutdown();
	void close();

protected:
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status);
	virtual void OnWrote(int status){}
	virtual void OnShutdown(ZQ::eloop::Handle::ElpeError status){}
	virtual void OnClose();
	// supposed to receive a response of request just sent
	virtual void OnMessage(std::string& msg);
	virtual void onError( int error,const char* errorDescription )
	{	
		_lipcLog(ZQ::common::Log::L_ERROR, CLOGFMT(Client, "errCode = %d,errDesc:%s"),error,errorDescription);
	}

	ZQ::common::Log& _lipcLog;
private:
	std::string		_localPipeName;
	std::string		_peerPipeName;
	int			_ipc;
	Servant*	_svt;
	ZQ::eloop::Loop& _loop;
	bool		_reconnect;
};

}}//ZQ::LIPC
#endif
