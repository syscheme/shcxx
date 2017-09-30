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
// Ident : $Id: LIPC.h, v 1.8 2017/06/09 09:32:35 zhixiang.zhu Exp $
// Branch: $Name:  $
// Author: zhuzhixiang
// Desc  : Define LIPC class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/LIPC/LIPC.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_LIPC_H__
#define __ZQ_COMMON_ELOOP_LIPC_H__

#include "UnixSocket.h"

#include <vector>
#include <list>

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API LIPCService;
class ZQ_ELOOP_API LIPCClient;
class ZQ_ELOOP_API LIPCRequest;
class ZQ_ELOOP_API LIPCResponse;
class ZQ_ELOOP_API LIPCMessage;
class ZQ_ELOOP_API LIPCProcess;

// ------------------------------------------------
// class LIPCMessage
// ------------------------------------------------
class LIPCMessage : virtual public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<LIPCMessage> Ptr;

	typedef enum
	{
		LIPC_OK = 0, 
		LIPC_PARSING_ERROR = -32700, /**< Invalid JSON. An error occurred on the server while parsing the JSON text. */
		LIPC_INVALID_REQUEST = -32600, /**< The received JSON not a valid JSON-RPC LIPCRequest. */
		LIPC_METHOD_NOT_FOUND = -32601, /**< The requested remote-procedure does not exist / is not available. */
		LIPC_INVALID_PARAMS = -32602, /**< Invalid method parameters. */
		LIPC_INTERNAL_ERROR = -32603, /**< Internal JSON-RPC error. */

		//user define error
		LIPC_NOT_FD	= -33000, 
		LIPC_INVALID_FD_TYPE = -33001, 

	} Error;

	static const char* errDesc(Error code);

	typedef int fd_t;
	typedef enum
	{
		LIPC_UDP, 
		LIPC_TCP, 
		LIPC_FILE, 
		LIPC_NONE,

	} FdType;

	LIPCMessage(int cseq =0, Json::Value msg = Json::Value::null);

	void  setErrorCode(Error code);
	Error getErrorCode();

	int getCSeq() const { return _cSeq; }
	
	FdType getFdType() const;
	int    getFd() const;
	void   setFd(fd_t fd, FdType type);
	bool   hasFd();

	bool empty();

	std::string toString();
	bool fromString(const std::string& str);

protected:
	Json::Value		_msg;
	uint32		    _cSeq;
	int64			_stampCreated;
};

// ------------------------------------------------
// class LIPCRequest
// ------------------------------------------------
class LIPCRequest : public LIPCMessage
{
	friend class LIPCClient;

public:
	typedef ZQ::common::Pointer<LIPCRequest> Ptr;

	LIPCRequest(int cseq, Json::Value msg = Json::Value::null)
		: LIPCMessage(cseq, msg)
	{}

	// void setMethod(const std::string& methodName, Callback_t cb=NULL, void* data=NULL);
	// std::string getMethodName();
	// Callback& getCb();
	void setParam(const Json::Value& param);
	Json::Value getParam();

//private:
//	Callback _cbInfo;
};

// ------------------------------------------------
// class LIPCResponse
// ------------------------------------------------
class LIPCResponse : public LIPCMessage
{
	friend class LIPCClient;

public:
	typedef ZQ::common::Pointer<LIPCResponse> Ptr;

	LIPCResponse(int cseq, UnixSocket& conn)
		: _conn(conn), LIPCMessage(cseq)
	{}

	virtual ~LIPCResponse();

	void setResult(const Json::Value& result);
	void post();

	Json::Value getResult();

private:
	UnixSocket&	_conn;
};

// ------------------------------------------------
// class LIPCService
// ------------------------------------------------
class LIPCService : public ZQ::eloop::Pipe
{
	friend class PassiveConn;

public:
	typedef std::list< PassiveConn* > PipeClientList;

public:

	LIPCService(ZQ::common::Log& log):_lipcLog(log){}
	int init(ZQ::eloop::Loop &loop, int ipc=1);
	PipeClientList& getPipeClientList() {return _clients;}

protected:
	ZQ::common::Log& _lipcLog;
	void addConn(PassiveConn* conn);
	void delConn(PassiveConn* conn);
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void onError( int error, const char* errorDescription)
	{	
		_lipcLog(ZQ::common::Log::L_ERROR, CLOGFMT(LIPCService, "errCode = %d, errDesc:%s"), error, errorDescription);
	}

	//@note the child impl is expected to call resp->post() to send the response out
	virtual void execOrDispatch(const std::string& methodName, const LIPCRequest::Ptr& req, LIPCResponse::Ptr& resp)
	{ resp->setErrorCode(LIPCMessage::LIPC_METHOD_NOT_FOUND); resp->post(); }

private:
	PipeClientList _clients;
	int			   _ipc;
};

// ------------------------------------------------
// class LIPCClient
// ------------------------------------------------
class LIPCClient
{
	friend class ClientConn;

public:
	LIPCClient(Loop &loop, ZQ::common::Log& log, int ipc=1);

	uint lastCSeq();

	typedef void (*Callback_t)(LIPCMessage& msg, void* data);

	int  bind(const char *name);
	int connect(const char *name);
	ZQ::eloop::Loop& get_loop() const;

	int read_start();
	int read_stop();
	int sendRequest(const std::string& methodName, LIPCRequest::Ptr req);
	int shutdown();
	void close();

protected:
	virtual void OnRequestPrepared(const std::string& method, int cseq, Json::Value& pReq){}
	virtual void OnIndividualMessage(Json::Value& msg);
	virtual void OnResponse(LIPCResponse::Ptr resp) {}

protected: // redirect from UnixSocket
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status) {}
	virtual void OnWrote(int status) {}
	virtual void OnShutdown(ZQ::eloop::Handle::ElpeError status) {}
	virtual void OnClose() {}
	virtual void onError( int error, const char* errorDescription )
	{	
		_lipcLog(ZQ::common::Log::L_ERROR, CLOGFMT(LIPCClient, "errCode = %d, errDesc:%s"), error, errorDescription);
	}

	// supposed to receive a response of request just sent
	virtual void OnMessage(std::string& msg);

	ZQ::common::Log& _lipcLog;

private:
	void OnCloseHandle();

	std::string		_localPipeName;
	std::string		_peerPipeName;
	int			    _ipc;
	ClientConn*	    _conn; // for reconnect
	Loop&           _loop;
	bool		    _reconnect;

	ZQ::common::AtomicInt _lastCSeq;
};

}}//ZQ::eloop

#endif //__ZQ_COMMON_ELOOP_LIPC_H__
