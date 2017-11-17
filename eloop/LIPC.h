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
// $Log: /ZQProjs/Common/eloop/LIPC.h $
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

class PassiveConn;
class ClientConn;
class ClientTimer;
// ------------------------------------------------
// class LIPCMessage
// ------------------------------------------------
class LIPCMessage : virtual public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<LIPCMessage> Ptr;

	typedef enum
	{
		LIPC_OK               = 200, 

		LIPC_CLIENT_ERROR     = 400, 
		LIPC_NOT_FOUND        = 404, 
		LIPC_REQUEST_TIMEOUT  = 408, 
		LIPC_REQUEST_CONFLICT = 409, 
		LIPC_SERVER_ERROR     = 500, 
		LIPC_NOT_IMPLEMENTED  = 501, 
		LIPC_SERVICE_UNAVAIL  = 503, 
		LIPC_UNSUPPORT_VERSION= 505, 
		LIPC_SERVICE_TIMEOUT  = 506, 

		LIPC_IO_ERROR         = 507, 

		LIPC_PARSING_ERROR    = -32700, ///< Invalid JSON. An error occurred on the server while parsing the JSON text
		LIPC_INVALID_REQUEST  = -32600, ///< The received JSON not a valid JSON-RPC LIPCRequest
		LIPC_METHOD_NOT_FOUND = -32601, ///< The requested remote-procedure does not exist / is not available
		LIPC_INVALID_PARAMS   = -32602, ///< Invalid method parameters
		LIPC_INTERNAL_ERROR   = -32603, ///< Internal JSON-RPC error

		//user define error
		LIPC_INVALID_FD	      = -33000, 
		LIPC_INVALID_FD_TYPE  = -33001, 

	} Error;

	static const char* errDesc(Error code);
	static int64 hexvalue(const char* hexstr);
	static std::string hexstr(int64 value, bool prefix0X=true);

	typedef int fd_t;
	typedef enum
	{
		LIPC_UDP, 
		LIPC_TCP, 
		LIPC_FILE, 
		LIPC_NONE,

	} FdType;

	LIPCMessage(int cseq =0, Json::Value msg = Json::Value::null);

	void  setErrorCode(int code,std::string errMsg = "");
	Error getErrorCode();
	
	int getCSeq() const;

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

#define LIPC_SET_JSON_PARAM3(JVAR,PARAM)									JVAR[#PARAM] = PARAM
#define LIPC_SET_JSON_PARAM(JVAR, CVAR, PARAM)                              JVAR[#PARAM] = CVAR.PARAM
#define LIPC_SET_JSON_PARAM2(JVAR, CVAR, PARAM, CTYPE)                      JVAR[#PARAM] = (CTYPE)CVAR.PARAM
#define LIPC_SET_JSON_PARAM_I64(JVAR, CVAR, PARAM)				            JVAR[#PARAM] = ZQ::eloop::LIPCMessage::hexstr(CVAR.PARAM)

#define LIPC_EXIST_AND_TAKE_PARAM(JVAR, CVAR, PARAM, VALTYPE, OTHERWISE)    if (JVAR.isMember(#PARAM)) CVAR.PARAM = JVAR[#PARAM].as##VALTYPE(); else OTHERWISE
#define LIPC_EXIST_AND_TAKE_PARAM2(JVAR, CVAR, PARAM, JVALTYPE, CVALTYPE, OTHERWISE)    if (JVAR.isMember(#PARAM)) CVAR.PARAM = (CVALTYPE)JVAR[#PARAM].as##JVALTYPE(); else OTHERWISE
#define LIPC_EXIST_AND_TAKE_PARAM_I64(JVAR, CVAR, PARAM, OTHERWISE)         if (JVAR.isMember(#PARAM)) CVAR.PARAM = ZQ::eloop::LIPCMessage::hexvalue(JVAR[#PARAM].asString().c_str()); else OTHERWISE

// ------------------------------------------------
// class LIPCRequest
// ------------------------------------------------
class LIPCRequest : public LIPCMessage
{
	friend class LIPCClient;

public:
	typedef ZQ::common::Pointer<LIPCRequest> Ptr;

	LIPCRequest(Json::Value msg = Json::Value::null)
		: LIPCMessage(0, msg)
	{}

	void setParam(const Json::Value& param);
	Json::Value getParam();
	std::string getMethod();
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
	Json::Value getResult();

	void post();
	void postResult(const Json::Value& result) { setResult(result); post(); } 
	void postException(int code,std::string errMsg = "");

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
	LIPCService(ZQ::common::Log& log) : _log(log),_isOnClose(false) {}

	uint32	getVerbosity() { return (ZQ::common::Log::loglevel_t)_log.getVerbosity() | (_verboseFlags<<8); }
	void    setVerbosity(uint32 verbose = (0 | ZQ::common::Log::L_ERROR)) { _log.setVerbosity(verbose & 0x0f); _verboseFlags =verbose>>8; }

	int init(ZQ::eloop::Loop &loop, int ipc=1);
	PipeClientList& getPipeClientList() { return _clients; }

	void UnInit();

protected:
	ZQ::common::LogWrapper _log;
	static uint32 _verboseFlags;

	void addConn(PassiveConn* conn);
	void delConn(PassiveConn* conn);
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void onError( int error, const char* errorDescription)
	{	
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(LIPCService, "errCode = %d, errDesc:%s"), error, errorDescription);
	}

	//@note the child impl is expected to call resp->post() to send the response out
	virtual void execOrDispatch(const std::string& methodName, const LIPCRequest::Ptr& req, LIPCResponse::Ptr& resp)
	{ resp->postException(LIPCMessage::LIPC_METHOD_NOT_FOUND); }


private:
	PipeClientList _clients;
	int			   _ipc;
	bool		   _isOnClose;
};

// ------------------------------------------------
// class LIPCClient
// ------------------------------------------------
class LIPCClient
{
	friend class ClientConn;
	friend class ClientTimer;

public:
	LIPCClient(Loop &loop, ZQ::common::Log& log, int64 timeout =500, int ipc=1); 

	uint32	getVerbosity() { return (ZQ::common::Log::loglevel_t)_log.getVerbosity() | (_verboseFlags<<8); }
	void    setVerbosity(uint32 verbose = (0 | ZQ::common::Log::L_ERROR)) { _log.setVerbosity(verbose & 0x0f); _verboseFlags =verbose>>8; }

	typedef void (*Callback_t)(LIPCMessage& msg, void* data);

	int  bind(const char *name);
	int connect(const char *name);
	ZQ::eloop::Loop& get_loop() const;

	int read_start();
	int read_stop();
	int sendRequest(const std::string& methodName, LIPCRequest::Ptr req, int64 timeout = 500, bool expectResp = true);		//default timeout = 500ms
	int shutdown();
	void close();

protected:
	virtual void OnRequestPrepared(LIPCRequest::Ptr req) {}
	virtual void OnResponse(const std::string& method, LIPCResponse::Ptr resp) {}
	virtual void OnRequestDone(int cseq, int ret) {}

	virtual void OnIndividualMessage(Json::Value& msg);

protected: // redirect from UnixSocket
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status);
	virtual void OnWrote(int status) {}
	virtual void OnShutdown(ZQ::eloop::Handle::ElpeError status);
	virtual void OnClose();
	virtual void onError( int error, const char* errorDescription );

	// supposed to receive a response of request just sent
	virtual void OnMessage(std::string& msg);

protected: // impl of ZQ::eloop::Timer

	ZQ::common::LogWrapper _log;
	static uint32 _verboseFlags;

	virtual void OnTimer();

private:
	void OnCloseConn();
	void OnCloseTimer();
	uint lastCSeq();

	std::string		_localPipeName;
	std::string		_peerPipeName;
	int			    _ipc;
	ClientConn*	    _conn; // for reconnect
	ClientTimer*	_timer;
	Loop&           _loop;
	bool		    _reconnect;
	int64			_timeout;

	ZQ::common::AtomicInt _lastCSeq;
	typedef struct
	{
		LIPCRequest::Ptr req;
		int64            expiration;
		std::string      method;
	} AwaitRequest;

	typedef std::map<uint, AwaitRequest> AwaitRequestMap;
	AwaitRequestMap _awaits;
	ZQ::common::Mutex _lkAwaits; // because sendRequest() is open for other threads out of eloop to call
	typedef std::vector<AwaitRequest> AwaitRequestList;
};

}}//ZQ::eloop

#endif //__ZQ_COMMON_ELOOP_LIPC_H__
