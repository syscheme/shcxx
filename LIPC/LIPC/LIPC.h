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

#include "eloop_net.h"
#include "TransferFd.h"
#include "JsonRpcHandler.h"
#include <vector>
#include <list>

namespace ZQ {
	namespace LIPC {

class ZQ_LIPC_API Dispatcher;
class ZQ_LIPC_API JsonRpcService;
class ZQ_LIPC_API JsonRpcClient;

// ------------------------------------------------
// class Dispatcher
// ------------------------------------------------
class Dispatcher:public ZQ::eloop::TCP
{
public:
	Dispatcher();
	~Dispatcher();
	
	void scan(const char* pathname);
	void addServant(TransferFdClient* client);

	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);

private:
	std::vector<TransferFdClient*> _servantVec;
};

// ------------------------------------------------
// class JsonRpcService
// ------------------------------------------------
class JsonRpcService:public TransferFdService,public Handler
{
public:
	JsonRpcService();
	~JsonRpcService();
	
	virtual void onRequest(std::string& req,PipePassiveConn* conn);	
	PipePassiveConn* getConn(){return _conn;}
	int acceptPendingHandle(ZQ::eloop::Handle* h);
	ZQ::eloop::Handle::eloop_handle_type getPendingHandleType();
	int getPendingCount();
private:
	PipePassiveConn* _conn;
};


// ------------------------------------------------
// class JsonRpcClient
// ------------------------------------------------
class JsonRpcClient:public PipeConnection,public ZQ::LIPC::Handler
{
public:
	int sendRequest(ZQ::LIPC::Arbitrary& value,ZQ::eloop::Handle* send_Handler = NULL);
	virtual void OnRequest(std::string& req);
};

}}//ZQ::LIPC


#endif
