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
// Ident : $Id: ZQSnmp.h,v 1.8 2014/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define SNMP exports
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/eloop/eloop.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_H__
#define __ZQ_COMMON_ELOOP_H__

#include "ZQ_common_conf.h"
#include "Pointer.h"
/*#include "FileLog.h"
#include "Locks.h"
#include "NativeThread.h"*/


#include <uv.h>



//#include <string>
//#include <vector>
//#include <map>

#ifdef ZQ_OS_MSWIN
#  ifdef ELOOP_EXPORTS
#    define ZQ_ELOOP_API __EXPORT
#  else
#    define ZQ_ELOOP_API __DLLRTL
#  endif
#else
#  define ZQ_ELOOP_API
#endif // OS

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API Loop;
class ZQ_ELOOP_API Handle;
class ZQ_ELOOP_API Idle;
class ZQ_ELOOP_API Timer;
class ZQ_ELOOP_API Async;
class ZQ_ELOOP_API Signal;

// -----------------------------
// class Handle
// -----------------------------
class Handle
{
public:
	typedef uv_os_fd_t fd_t;

protected:
	Handle();
	Handle(Handle &);
	Handle &operator=(Handle &);
	~Handle();
	void init();

public:
	int is_active();
	int is_closing();
	void close();
	void ref();
	void unref();
	int has_ref();
	int send_buffer_size(int *value);
	int recv_buffer_size(int *value);
	int fileno(fd_t* fd);
	const char* eloop_err_name(int err);
	const char* eloop_strerror(int err);
	void* data;

protected:
	virtual void OnClose(Handle *handle){}
	uv_handle_t *context_ptr();


private:
	void _deleteContext();
	static void _cbClose(uv_handle_t *uvhandle)
	{
		Handle *h = static_cast<Handle *>(uvhandle->data);
		if (NULL != h)
			h->OnClose(h);
			h->_deleteContext();
	}

	uv_any_handle* context;
};

// -----------------------------
// class Loop
// -----------------------------
class Loop
{
public:
	enum RunMode {
		Default = UV_RUN_DEFAULT,
		Once = UV_RUN_ONCE,
		NoWait = UV_RUN_NOWAIT
	};

	explicit Loop(bool defaultLoop = true);
	virtual ~Loop();

	bool run(RunMode mode);
	void stop();
	int close();
	int alive();
	int backend_fd();
	int backend_timeout();
	uint64_t now();
	void update_time();
	uv_loop_t *context_ptr();
	void walk(void* arg);

protected:
	virtual void OnWalk(Handle *handle, void* arg) {}

private:
	uv_loop_t* _uvLoop;

	static void _doWalk(uv_handle_t* uvhandle, void *arg);
};

// -----------------------------
// class Idle
// -----------------------------
class Idle : public Handle
{
public:
	Idle();
	int init(Loop &loop);
	int start();
	int stop();

protected:
	virtual void OnIdle() {}

private:
	static void _cbOnIdle(uv_idle_t* uvhandle);
};

// -----------------------------
// class Timer
// -----------------------------
class Timer : public Handle
{

public:
	Timer();
	int init(Loop &loop);
	int start(uint64_t timeout, uint64_t repeat);
	int stop();
	int again();
	void set_repeat(uint64_t repeat);
	uint64_t get_repeat();

protected:
	virtual void OnTimer() {}

private:
	static void _cbOnTimer(uv_timer_t *timer);
};

// -----------------------------
// class Async
// -----------------------------
class Async : public Handle 
{
public:
	Async();
	int init(Loop &loop);
	int send();

protected:
	virtual void OnAsync() {}

private:
	static void _cbAsync(uv_async_t *async);
};

// -----------------------------
// class Signal
// -----------------------------
class Signal : public Handle
{
public:
	Signal();
	int init(Loop &loop);
	int start(int signum);
	int stop();

protected:
	virtual void OnSignal(int signum) {}

private:
	static void _cbSignal(uv_signal_t *signal, int signum);
};



 } } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_H__
