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

#include "eloop/eloop_net.h"
#include "PipeConnection.h"
#include "Handler.h"
#include <vector>
#include <list>

namespace ZQ {
namespace LIPC {

class ZQ_LIPC_API Service;
class ZQ_LIPC_API Client;

// -----------------------------
// class Message
// -----------------------------
typedef struct _Message
{
	Json::Value params;
	int cseq;

	_Message() : params(Json::Value::null), cseq(-1) {}

	std::string toString();
	bool        fromString(const std::string& str);
} Message;

// -----------------------------
// interface IMethod
// -----------------------------
class IMethod
{
public:
	virtual void Call(const Json::Value& params, Json::Value& result) = 0;

    virtual std::string GetName() const = 0;
	virtual std::string GetDescription() const = 0;
};

// -----------------------------
// class RpcMethod
// -----------------------------
template<class T>
class RpcMethod : public IMethod
{
public:
    typedef void (T::*Method)(const Message& msg, PipeConnection& conn);

	RpcMethod(T& obj, Method method, const std::string& name, const Json::Value& description)
		: m_obj(&obj), m_name(name), m_method(method), m_description(description)
    {}

	virtual void Call(const Json::Value& params, Json::Value& result)
	{
		(m_obj->*m_method)(params, result);
	}

    virtual std::string GetName() const	{ return m_name; }
	virtual std::string GetDescription() const { return m_description; }

private:
	//private to avoid copy
    RpcMethod(const RpcMethod& obj);
    RpcMethod& operator=(const RpcMethod& obj);

    Json::Reader m_reader;
    T* m_obj;
    Method m_method;
    std::string m_name;
    std::string m_description;
};

// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
/// class to process the messages exchanged thru a PipeConnection
class Handler
{
public:
	enum ErrorCode
	{
		PARSING_ERROR = -32700, /**< Invalid JSON. An error occurred on the server while parsing the JSON text. */
		INVALID_REQUEST = -32600, /**< The received JSON not a valid JSON-RPC Request. */
		METHOD_NOT_FOUND = -32601, /**< The requested remote-procedure does not exist / is not available. */
		INVALID_PARAMS = -32602, /**< Invalid method parameters. */
		INTERNAL_ERROR = -32603 /**< Internal JSON-RPC error. */
	};

	typedef void (*RpcCB)(const Message& resp, void* data);

	typedef struct
	{
		RpcCB cb;
		void* data;
	} RpcCBInfo;

	typedef std::map<int, RpcCBInfo> seqToCBInfoMap;

public:

	Handler();
	virtual ~Handler();

	void AddMethod(IMethod* method);
	void Addcb(int seqId,RpcCB cb,void* data);
	void DeleteMethod(const std::string& name);
	void SystemDescribe(const Json::Value& v, PipeConnection& conn);

	void Process(const std::string& msg, PipeConnection& conn);

	IMethod* Lookup(const std::string& name) const;
	uint lastCSeq();
	static const char* errDesc(ErrorCode code);

private:

	Handler(const Handler& obj);
	Handler& operator=(const Handler& obj);

	std::list<IMethod*> m_methods;
	bool invoke(const Json::Value& params, Json::Value& result);
	bool Check(const Message& root, Json::Value& errResult);

	Json::Reader m_reader;
	seqToCBInfoMap m_seqIds;
	ZQ::common::AtomicInt _lastCSeq;
};

// ------------------------------------------------
// class Service
// ------------------------------------------------
// template<PipePassiveConn>
class Service : public Handler, public ZQ::eloop::Pipe
{
public:
	friend class PipePassiveConn;
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
		_lipcLog(ZQ::common::Log::L_ERROR, CLOGFMT(Service, "LIPC Service error,code:%d,Description:%s"),error,errorDescription);
	}

private:
	PipeClientList _ClientList;
	int				_ipc;
};


// ------------------------------------------------
// class Client
// ------------------------------------------------
class Client : public PipeConnection, public ZQ::LIPC::Handler
{
public:
	Client(ZQ::common::Log& log):_lipcLog(log),PipeConnection(log) {}
	int sendHandlerRequest(std::string method, Json::Value param, RpcCB cb = NULL, void* data = NULL, ZQ::eloop::Handle* send_Handler = NULL);
	int sendRequest(std::string method, Json::Value param, RpcCB cb = NULL, void* data = NULL, int fd = -1);

protected:

	// the request has been composed and about to send out, this override-able method give the child class a chance to fixup to out-going request
	virtual void OnRequestPrepared(const std::string& method, Message& pReq) {}

	// supposed to receive a response of request just sent
	virtual void OnMessage(std::string& msg);
	virtual void onError( int error,const char* errorDescription )
	{	
		fprintf(stderr, "errCode=%d errDesc:%s\n",error,errorDescription);
	}

	ZQ::common::Log& _lipcLog;
};

}}//ZQ::LIPC


#endif
