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
/*#include "Pointer.h"
#include "FileLog.h"
#include "Locks.h"
#include "NativeThread.h"*/
#include <uv.h>

#include <string>
#include <vector>
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
class ZQ_ELOOP_API CpuInfo;
class ZQ_ELOOP_API Process;

// -----------------------------
// class Handle
// -----------------------------
class Handle
{
public:
	typedef uv_os_fd_t fd_t;

	typedef uv_buf_t eloop_buf_t;

	typedef enum _ElpHandleType
	{
		ELOOP_UNKNOWN_HANDLE = UV_UNKNOWN_HANDLE,

		ELOOP_ASYNC = UV_ASYNC,
		ELOOP_CHECK = UV_CHECK,
		ELOOP_FS_EVENT = UV_FS_EVENT,
		ELOOP_FS_POLL = UV_FS_POLL,
		ELOOP_HANDLE = UV_HANDLE,
		ELOOP_IDLE = UV_IDLE,
		ELOOP_NAMED_PIPE = UV_NAMED_PIPE,
		ELOOP_POLL = UV_POLL,
		ELOOP_PREPARE = UV_PREPARE,
		ELOOP_PROCESS = UV_PROCESS,
		ELOOP_STREAM = UV_STREAM,
		ELOOP_TCP = UV_TCP,
		ELOOP_TIMER = UV_TIMER,
		ELOOP_TTY = UV_TTY,
		ELOOP_UDP = UV_UDP,
		ELOOP_SIGNAL = UV_SIGNAL,
		
		ELOOP_FILE = UV_FILE,
		ELOOP_HANDLE_TYPE_MAX = UV_HANDLE_TYPE_MAX
	} eloop_handle_type;

	typedef enum _ElpeError
	{
		elpeSuccess = 0,
		elpeNotPermitted = EPERM,	   // operation not permitted
		elpeNoFile = ENOENT,		   //no such file or directory
		elpeNoProcess = ESRCH,		   //no such process
		elpeIntr = EINTR,			   //interrupted system call
		elpeIO = EIO,				   //i/o error
		elpeNotDevOrAddress = ENXIO,			//no such device or address
		elpeArgTooLong = E2BIG,			//argument list too long
		elpeENOEXEC = ENOEXEC,
		elpeBadFd = EBADF,				//bad file descriptor
		elpeECHILD = ECHILD,			//
		elpeResUnavailable = EAGAIN,			//resource temporarily unavailable
		elpeNotEnoughMemory = ENOMEM,			//not enough memory
		elpePermissionDenied = EACCES,			//permission denied
		elpeBadAddress = EFAULT,			//bad address in system call argument
		elpeResBusy = EBUSY,				//resource busy or locked
		elpeFileAlreadyExists = EEXIST,			//file already exists
		elpeNotPermittedLink = EXDEV,				//cross-device link not permitted
		elpeNotDevice = ENODEV,			//no such device
		elpeNotDirectory = ENOTDIR,		//not a directory
		elpeEISDIR = EISDIR,			//illegal operation on a directory
		elpeENFILE = ENFILE,			//file table overflow
		elpeTooMany = EMFILE,			//too many open files
		elpeENOTTY = ENOTTY,
		elpeFileTooLarge = EFBIG,				//file too large
		elpeNoSpace = ENOSPC,			//no space left on device
		elpeInvalidSeek = ESPIPE,			//invalid seek
		elpeROFS = EROFS,				//read-only file system
		elpeTooManyLinks = EMLINK,			//too many links
		elpeBorkenPipe = EPIPE,				//broken pipe
		elpeEDOM = EDOM,
		elpeEDEADLK = EDEADLK,
		elpeNameTooLong = ENAMETOOLONG, //name too long
		elpeNOLCK = ENOLCK,
		elpeFunNotImpl = ENOSYS,			//function not implemented
		elpeDirNotEmpty = ENOTEMPTY,		//directory not empty
		elpe__EOF = UV__EOF,
		elpe__UNKNOWN = UV__UNKNOWN,
		elpe__EAI_ADDRFAMILY = UV__EAI_ADDRFAMILY,
		elpe__EAI_AGAIN = UV__EAI_AGAIN,
		elpe__EAI_BADFLAGS = UV__EAI_BADFLAGS,
		elpe__EAI_CANCELED = UV__EAI_CANCELED,
		elpe__EAI_FAIL = UV__EAI_FAIL,
		elpe__EAI_FAMILY = UV__EAI_FAMILY,
		elpe__EAI_MEMORY = UV__EAI_MEMORY,
		elpe__EAI_NODATA = UV__EAI_NODATA,
		elpe__EAI_NONAME = UV__EAI_NONAME,
		elpe__EAI_OVERFLOW = UV__EAI_OVERFLOW,
		elpe__EAI_SERVICE = UV__EAI_SERVICE,
		elpe__EAI_SOCKTYPE = UV__EAI_SOCKTYPE,
		elpe__EAI_BADHINTS = UV__EAI_BADHINTS,
		elpe__EAI_PROTOCOL = UV__EAI_PROTOCOL,
		elpe__E2BIG = UV__E2BIG,
		elpe__EACCES = UV__EACCES,
		elpe__EADDRINUSE = UV__EADDRINUSE,
		elpe__EADDRNOTAVAIL = UV__EADDRNOTAVAIL,
		elpe__EAFNOSUPPORT = UV__EAFNOSUPPORT,
		elpe__EAGAIN = UV__EAGAIN,
		elpe__EALREADY = UV__EALREADY,
		elpe__EBADF = UV__EBADF,
		elpe__EBUSY = UV__EBUSY,
		elpe__ECANCELED = UV__ECANCELED,
		elpe__ECHARSET = UV__ECHARSET,
		elpe__ECONNABORTED = UV__ECONNABORTED,
		elpe__ECONNREFUSED = UV__ECONNREFUSED,
		elpe__ECONNRESET = UV__ECONNRESET,
		elpe__EDESTADDRREQ = UV__EDESTADDRREQ,
		elpe__EEXIST = UV__EEXIST,
		elpe__EFAULT = UV__EFAULT,
		elpe__EHOSTUNREACH = UV__EHOSTUNREACH,
		elpe__EINTR = UV__EINTR,
		elpe__EINVAL = UV__EINVAL,
		elpe__EIO = UV__EIO,
		elpe__EISCONN = UV__EISCONN,
		elpe__EISDIR = UV__EISDIR,
		elpe__ELOOP = UV__ELOOP,
		elpe__EMFILE = UV__EMFILE,
		elpe__EMSGSIZE = UV__EMSGSIZE,
		elpe__ENAMETOOLONG = UV__ENAMETOOLONG,
		elpe__ENETDOWN = UV__ENETDOWN,
		elpe__ENETUNREACH = UV__ENETUNREACH,
		elpe__ENFILE = UV__ENFILE,
		elpe__ENOBUFS = UV__ENOBUFS,
		elpe__ENODEV = UV__ENODEV,
		elpe__ENOENT = UV__ENOENT,
		elpe__ENOMEM = UV__ENOMEM,
		elpe__ENONET = UV__ENONET,
		elpe__ENOSPC = UV__ENOSPC,
		elpe__ENOSYS = UV__ENOSYS,
		elpe__ENOTCONN = UV__ENOTCONN,
		elpe__ENOTDIR = UV__ENOTDIR,
		elpe__ENOTEMPTY = UV__ENOTEMPTY,
		elpe__ENOTSOCK = UV__ENOTSOCK,
		elpe__ENOTSUP = UV__ENOTSUP,
		elpe__EPERM = UV__EPERM,
		elpe__EPIPE = UV__EPIPE,
		elpe__EPROTO = UV__EPROTO,
		elpe__EPROTONOSUPPORT = UV__EPROTONOSUPPORT,
		elpe__EPROTOTYPE = UV__EPROTOTYPE,
		elpe__EROFS = UV__EROFS,
		elpe__ESHUTDOWN = UV__ESHUTDOWN,
		elpe__ESPIPE = UV__ESPIPE,
		elpe__ESRCH = UV__ESRCH,
		elpe__ETIMEDOUT = UV__ETIMEDOUT,
		elpe__ETXTBSY = UV__ETXTBSY,
		elpe__EXDEV = UV__EXDEV,
		elpe__EFBIG = UV__EFBIG,
		elpe__ENOPROTOOPT = UV__ENOPROTOOPT,
		elpe__ERANGE = UV__ERANGE,
		elpe__ENXIO = UV__ENXIO,
		elpe__EMLINK = UV__EMLINK,
		elpe__EHOSTDOWN = UV__EHOSTDOWN,
	} ElpeError;

	static const char* errDesc(ElpeError err)  { return errDesc((int)err); }
	static const char* errDesc(int err)  { return uv_strerror(err); }
	static const char* errName(ElpeError err) { return errName((int) err); }
	static const char* errName(int errNum)    { return uv_err_name(errNum); }

	static int exepath(char* buf,size_t* size){return uv_exepath(buf,size);}

protected:
	Handle();
	Handle(Handle &);
	Handle &operator=(Handle &);
	virtual ~Handle();

public:
	void init(Loop &loop);

	void* data;
	void close();
	void ref();
	void unref();
	int has_ref();

	int is_active();
	int is_closing();
	int send_buffer_size(int *value);
	int recv_buffer_size(int *value);
	int fileno(fd_t* fd);
	void _deleteContext();
	Loop& get_loop();
	uv_handle_t *context_ptr();

protected:
	virtual void OnClose(){delete this;}
	bool		_isStart;
	bool		_isClose;
	

private:
	static void _cbClose(uv_handle_t *uvhandle);

	uv_any_handle* context;
	Loop* _loop;
};


// -----------------------------
// class Request
// -----------------------------
class Request
{
protected:
	Request();
	virtual ~Request();
	void init();
	int cancel();

private:
	Request(Request &){}
	Request &operator=(Request &){}
protected:
	uv_req_t *context_ptr();
	
private:
	uv_any_req* context;
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
// 
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

// -----------------------------
// class CpuInfo
// -----------------------------
class CpuInfo
{
public:
	typedef uv_cpu_info_t cpu_info;
	CpuInfo();
	~CpuInfo();
	int getCpuCount();
	cpu_info*	getCpuInfo();
	void		freeCpuInfo(); 

private:
	cpu_info*		_info;
	int				_count;
	bool			_bIsAccess;
};

// -----------------------------
// class Process
// -----------------------------
class Stream;
class Process : public Handle
{
public:
	 enum {
		ELOOP_IGNORE = UV_IGNORE,
		ELOOP_CREATE_PIPE = UV_CREATE_PIPE,
		ELOOP_INHERIT_FD = UV_INHERIT_FD,
		ELOOP_INHERIT_STREAM = UV_INHERIT_STREAM,

		// When UV_CREATE_PIPE is specified, UV_READABLE_PIPE and UV_WRITABLE_PIPE
		// determine the direction of flow, from the child process' perspective. Both
		// flags may be specified to create a duplex data stream.
		ELOOP_READABLE_PIPE = UV_READABLE_PIPE,
		ELOOP_WRITABLE_PIPE = UV_WRITABLE_PIPE
	};

	typedef uv_stdio_flags eloop_stdio_flags;
	typedef uv_stdio_container_t eloop_stdio_container_t;
	typedef uv_uid_t	eloop_uid_t;
	typedef uv_gid_t	eloop_gid_t;

public:
	void setenv(char** env);
	void setcwd(const char* cwd);
	void setflags(unsigned int flags);
	void setuid(eloop_uid_t uid);
	void setgid(eloop_gid_t gid);

	int spawn(const char* file,char** args,eloop_stdio_container_t* container,int stdio_count);

	int pid();
	int kill(int signum);
	int kill(int pid,int signum);

protected:
	virtual void OnExit(int64_t exit_status,int term_signal) { close();}

private:
	static void _cbExit(uv_process_t* handle,int64_t exit_status,int term_signal);
	uv_process_options_t _opt;
};



/*
// -----------------------------
// class AsyncBuf
// -----------------------------
class AsyncBuf
{
public:
	typedef std::vector<uv_buf_t> ELBUFS;
public:
	AsyncBuf(){}
	~AsyncBuf(){}

	void push_back(char* base,size_t len)
	{
		uv_buf_t buf;
		buf.base = base;
		buf.len = len;
		_bufs.push_back(buf);
	}

private:
	ELBUFS _bufs;
};
*/

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_H__
