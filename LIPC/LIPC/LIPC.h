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
// ------------------------------------------------
// class Service
// ------------------------------------------------
// template<PipePassiveConn>
class Service : public Handler,public ZQ::eloop::Pipe
{
public:
	friend class PipePassiveConn;
public:
	typedef std::list< PipePassiveConn* > PipeClientList;

public:
	int init(ZQ::eloop::Loop &loop, int ipc=1);
	PipeClientList& getPipeClientList(){return _ClientList;}
	int getPendingCount();

	int acceptPendingHandle(ZQ::eloop::Handle* h);
	ZQ::eloop::Handle::eloop_handle_type getPendingHandleType();

protected:
	void addConn(PipePassiveConn* conn);
	void delConn(PipePassiveConn* conn);
	
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);

	PipePassiveConn* getConn(){return _conn;}

private:
	PipePassiveConn* _conn;
	PipeClientList _ClientList;
	int				_ipc;

};


// ------------------------------------------------
// class Client
// ------------------------------------------------
class Client : public PipeConnection, public ZQ::LIPC::Handler
{
public:
	virtual int sendHandlerRequest(ZQ::LIPC::Arbitrary& value,RpcCB cb = NULL,void* data = NULL,ZQ::eloop::Handle* send_Handler = NULL);
	virtual int sendRequest(std::string method,ZQ::LIPC::Arbitrary param,RpcCB cb = NULL,void* data = NULL,int fd = -1);

	// supposed to receive a response of request just sent
	virtual void OnMessage(std::string& msg);
};



}}//ZQ::LIPC
#endif
