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
#include <vector>

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

class ZQ_LIPC_API TransferService;

class TransferFdClient;
// -----------------------------
// class Dispatcher
// -----------------------------
class Dispatcher:public ZQ::eloop::TCP
{
public:
	Dispatcher(const char* pathname);
	~Dispatcher();
	
	void scan(const char* pathname);
	void addServant(TransferFdClient* client);

	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);
	virtual void OnRead(ssize_t nread, const char *buf);

private:
	std::vector<TransferFdClient*> _servantVec;

};

// -----------------------------
// class JsonRpcService
// -----------------------------
class JsonRpcService:public ZQ::eloop::TCP
{
public:
	JsonRpcService();
	~JsonRpcService();

	void addMethod();
	void addClass();
};



}}//ZQ::LIPC


#endif