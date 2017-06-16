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

#include "eloop_net.h"
#include "TransferFd.h"
#include "JsonRpcHandler.h"
#include <vector>
#include <list>

#ifdef ZQ_OS_MSWIN
#  ifdef LIPC_EXPORTS
#    define ZQ_LIPC_API __EXPORT
#  else
#    define ZQ_LIPC_API __DLLRTL
#  endif
#else
#  define ZQ_LIPC_API
#endif // OS


namespace ZQ {
	namespace LIPC {

class ZQ_LIPC_API Dispatcher;

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



// -----------------------------
// class Servant
// -----------------------------
class Servant:public ZQ::eloop::TCP
{
public:
	Servant(ServantManager& mgr);
	~Servant();
	void start();
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);
	virtual void OnClose();

private:
	ServantManager&	_Mgr;
};

// ------------------------------------------------
// class JsonRpcService
// ------------------------------------------------
class JsonRpcService:public TransferFdService,public Handler
{
public:
	JsonRpcService();
	~JsonRpcService();
//	void start(Loop& loop,const char* pathname);
	virtual void onRequest(const char* req,Servant* conn);
};


// ------------------------------------------------
// class JsonRpcClient
// ------------------------------------------------
class JsonRpcClient:public ZQ::eloop::TCP
{
public:
	JsonRpcClient();
	~JsonRpcClient();

	void beginRequest(const char* ip,int port,Request::Ptr req);
	void Request(Request::Ptr req);
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status);
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);

private:
	Request::Ptr m_req;
};

}}//ZQ::LIPC


#endif