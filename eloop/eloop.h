// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This  software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
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

#include "uv.h"
#include "uv-errno.h"

extern "C"{
#ifdef ZQ_OS_LINUX
#include <sys/time.h>
#endif
};

#include <string>
#include <vector>

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
class ZQ_ELOOP_API IterationBlocker;
class ZQ_ELOOP_API Timer;
class ZQ_ELOOP_API Wakeup;
class ZQ_ELOOP_API SysSignalSink;
class ZQ_ELOOP_API CpuInfo;
class ZQ_ELOOP_API Process;

// ----------------------------------------------------
// class Handle
// ----------------------------------------------------
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
		elpeSuccess               = 0,
		elpeNotPermitted          = EPERM,	   // operation not permitted
		elpeNoFile                = ENOENT,		   //no such file or directory
		elpeNoProcess             = ESRCH,		   //no such process
		elpeIntr                  = EINTR,			   //interrupted system call
		elpeIO                    = EIO,				   //i/o error
		elpeNotDevOrAddress       = ENXIO,			//no such device or address
		elpeArgTooLong            = E2BIG,			//argument list too long
		elpeENOEXEC               = ENOEXEC,
		elpeBadFd                 = EBADF,				//bad file descriptor
		elpeECHILD                = ECHILD,			//
		elpeResUnavailable        = EAGAIN,			//resource temporarily unavailable
		elpeNotEnoughMemory       = ENOMEM,			//not enough memory
		elpePermissionDenied      = EACCES,			//permission denied
		elpeBadAddress            = EFAULT,			//bad address in system call argument
		elpeResBusy               = EBUSY,				//resource busy or locked
		elpeFileAlreadyExists     = EEXIST,			//file already exists
		elpeNotPermittedLink      = EXDEV,				//cross-device link not permitted
		elpeNotDevice             = ENODEV,			//no such device
		elpeNotDirectory          = ENOTDIR,		//not a directory
		elpeEISDIR                = EISDIR,			//illegal operation on a directory
		elpeENFILE                = ENFILE,			//file table overflow
		elpeTooMany               = EMFILE,			//too many open files
		elpeENOTTY                = ENOTTY,
		elpeFileTooLarge          = EFBIG,				//file too large
		elpeNoSpace               = ENOSPC,			//no space left on device
		elpeInvalidSeek           = ESPIPE,			//invalid seek
		elpeROFS                  = EROFS,				//read-only file system
		elpeTooManyLinks          = EMLINK,			//too many links
		elpeBorkenPipe            = EPIPE,				//broken pipe
		elpeEDOM                  = EDOM,
		elpeEDEADLK               = EDEADLK,
		elpeNameTooLong           = ENAMETOOLONG, //name too long
		elpeNOLCK                 = ENOLCK,
		elpeFunNotImpl            = ENOSYS,			//function not implemented
		elpeDirNotEmpty           = ENOTEMPTY,		//directory not empty

		// error code from libuv
		elpuEOF                 = UV__EOF,
		elpeUNKNOWN             = UV__UNKNOWN,
		elpuEAI_ADDRFAMILY      = UV__EAI_ADDRFAMILY,
		elpuEAI_AGAIN           = UV__EAI_AGAIN,
		elpuEAI_BADFLAGS        = UV__EAI_BADFLAGS,
		elpuEAI_CANCELED        = UV__EAI_CANCELED,
		elpuEAI_FAIL            = UV__EAI_FAIL,
		elpuEAI_FAMILY          = UV__EAI_FAMILY,
		elpuEAI_MEMORY          = UV__EAI_MEMORY,
		elpuEAI_NODATA          = UV__EAI_NODATA,
		elpuEAI_NONAME          = UV__EAI_NONAME,
		elpuEAI_OVERFLOW        = UV__EAI_OVERFLOW,
		elpuEAI_SERVICE         = UV__EAI_SERVICE,
		elpuEAI_SOCKTYPE        = UV__EAI_SOCKTYPE,
		elpuEAI_BADHINTS        = UV__EAI_BADHINTS,
		elpuEAI_PROTOCOL        = UV__EAI_PROTOCOL,

		elpuE2BIG               = UV__E2BIG,
		elpuEACCES              = UV__EACCES,
		elpuEADDRINUSE          = UV__EADDRINUSE,
		elpuEADDRNOTAVAIL       = UV__EADDRNOTAVAIL,
		elpuEAFNOSUPPORT        = UV__EAFNOSUPPORT,
		elpuEAGAIN              = UV__EAGAIN,
		elpuEALREADY            = UV__EALREADY,
		elpuEBADF               = UV__EBADF,
		elpuEBUSY               = UV__EBUSY,
		elpuECANCELED           = UV__ECANCELED,
		elpuECHARSET            = UV__ECHARSET,
		elpuECONNABORTED        = UV__ECONNABORTED,
		elpuECONNREFUSED        = UV__ECONNREFUSED,
		elpuECONNRESET          = UV__ECONNRESET,
		elpuEDESTADDRREQ        = UV__EDESTADDRREQ,
		elpuEEXIST              = UV__EEXIST,
		elpuEFAULT              = UV__EFAULT,
		elpuEHOSTUNREACH        = UV__EHOSTUNREACH,
		elpuEINTR               = UV__EINTR,
		elpuEINVAL              = UV__EINVAL,
		elpuEIO                 = UV__EIO,
		elpuEISCONN             = UV__EISCONN,
		elpuEISDIR              = UV__EISDIR,
		elpuELOOP               = UV__ELOOP,
		elpuEMFILE              = UV__EMFILE,
		elpuEMSGSIZE            = UV__EMSGSIZE,
		elpuENAMETOOLONG        = UV__ENAMETOOLONG,
		elpuENETDOWN            = UV__ENETDOWN,
		elpuENETUNREACH         = UV__ENETUNREACH,
		elpuENFILE              = UV__ENFILE,
		elpuENOBUFS             = UV__ENOBUFS,
		elpuENODEV              = UV__ENODEV,
		elpuENOENT              = UV__ENOENT,
		elpuENOMEM              = UV__ENOMEM,
		elpuENONET              = UV__ENONET,
		elpuENOSPC              = UV__ENOSPC,
		elpuENOSYS              = UV__ENOSYS,
		elpuENOTCONN            = UV__ENOTCONN,
		elpuENOTDIR             = UV__ENOTDIR,
		elpuENOTEMPTY           = UV__ENOTEMPTY,
		elpuENOTSOCK            = UV__ENOTSOCK,
		elpuENOTSUP             = UV__ENOTSUP,
		elpuEPERM               = UV__EPERM,
		elpuEPIPE               = UV__EPIPE,
		elpuEPROTO              = UV__EPROTO,
		elpuEPROTONOSUPPORT     = UV__EPROTONOSUPPORT,
		elpuEPROTOTYPE          = UV__EPROTOTYPE,
		elpuEROFS               = UV__EROFS,
		elpuESHUTDOWN           = UV__ESHUTDOWN,
		elpuESPIPE              = UV__ESPIPE,
		elpuESRCH               = UV__ESRCH,
		elpuETIMEDOUT           = UV__ETIMEDOUT,
		elpuETXTBSY             = UV__ETXTBSY,
		elpuEXDEV               = UV__EXDEV,
		elpuEFBIG               = UV__EFBIG,
		elpuENOPROTOOPT         = UV__ENOPROTOOPT,
		elpuERANGE              = UV__ERANGE,
		elpuENXIO               = UV__ENXIO,
		elpuEMLINK              = UV__EMLINK,
		elpuEHOSTDOWN           = UV__EHOSTDOWN,
		elpuUnKnown

	} ElpeError;

	static ElpeError uvErr2ElpeErr(int errCode);
	static const char* errDesc(ElpeError err)  { return errDesc((int)err); }
	static const char* errDesc(int err)  { return uv_strerror(err); }
	static const char* errName(ElpeError err) { return errName((int) err); }
	static const char* errName(int errNum)    { return uv_err_name(errNum); }

	static int exepath(char* buf,size_t* size){return uv_exepath(buf,size);}

protected:
	Handle(Loop& loop);
	// Handle(Handle &);
	// Handle &operator=(Handle &);
	virtual void init() =0;

public:
	Loop& loop() { return _loop; }
	virtual ~Handle();

	void close(); // void close();

	//void* data;
	//void close();
	//void ref();
	//void unref();
	//int has_ref();

	//int is_active();
	//int is_closing();
	int send_buffer_size(int *value);
	int recv_buffer_size(int *value);
	int fileno(fd_t* fd);
    //void _deleteContext();
    //Loop& get_loop();
    //uv_handle_t *context_ptr();

protected:
	virtual void OnClose() { delete this; }
	//	bool		_isStart;
	// bool		_isClose;

	Loop& _loop;
	uv_any_handle* _context;

private:
	static void _cbClose(uv_handle_t *uvhandle);
};

// -----------------------------
// class Request
// -----------------------------
class EloopRequest
{
protected:
	EloopRequest();
	virtual ~EloopRequest();
	int cancel();

private:
	EloopRequest(EloopRequest &){}
	EloopRequest &operator=(EloopRequest &){}
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

	unsigned int getThreadPoolSize();
	int setThreadPoolSize(const unsigned int size);

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
// class IterationBlocker
// -----------------------------
// this handle will be run once per loop iteration, right before the Prepare handles
// if registered, it will overwrite the IO blocking step
class IterationBlocker : public Handle
{
public:
	IterationBlocker(Loop& loop) : Handle(loop) {}

	int start();
	int stop();

protected: // event callback
	virtual void OnIdle() {}

private: // impl of Handle
	void init();

private:
	static void _cbOnIdle(uv_idle_t* uvhandle);
};

// -----------------------------
// class Timer
// -----------------------------
class Timer : public Handle
{
public:
	Timer(Loop& loop) : Handle(loop) {}
	int start(uint64_t timeout, bool bRepeat =false); // int start(uint64_t timeout, uint64_t repeat);
	int stop();
	int again();
	
	// Set the repeat interval in msec. The interval will be regardless the duration of OnTimer(), for example, a 50msec
	// repeating timer will be scheduled to run again 33msec later if the previous OnTimer() takes 17msec
	void set_repeat(uint64_t interval);
	uint64_t get_repeat();

protected: // event callback
	virtual void OnTimer() {}

private: // impl of Handle
	void init();

private:
	static void _cbOnTimer(uv_timer_t *timer);
};

// -----------------------------
// class Wakeup
// -----------------------------
// old name Async
// to allow others, maybe the threads out of the eloop, to wakeup the eloop
class Wakeup : public Handle
{
public:
	Wakeup(Loop& loop) : Handle(loop) {}
	int wakeup();

protected: // event callback
	virtual void OnWakedUp() {}

private: // impl of Handle
	void init();

private:
	static void _cbAsync(uv_async_t *async);
};

// -----------------------------
// class SysSignalSink
// -----------------------------
// sinks Unix-style OS signals on a per-event loop base
class SysSignalSink : public Handle
{
public:
	SysSignalSink(Loop& loop) : Handle(loop) {}
	int subscribe(int signum);
	int stop();

protected: // event callback
	virtual void OnSignal(int signum) {}

private: // impl of Handle
	void init();

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
// class ChildProcess
// -----------------------------
// old name Process
// manage to child process and allowing communication with the child using streams or named pipes
class Stream;
class ChildProcess : public Handle
{
public:
	typedef uint uid_t;
	typedef uint gid_t;
	typedef enum _stdioFlags {
		ELOOP_IGNORE = UV_IGNORE,
		ELOOP_CREATE_PIPE = UV_CREATE_PIPE,
		ELOOP_INHERIT_FD = UV_INHERIT_FD,
		ELOOP_INHERIT_STREAM = UV_INHERIT_STREAM,

		// When UV_CREATE_PIPE is specified, UV_READABLE_PIPE and UV_WRITABLE_PIPE
		// determine the direction of flow, from the child process' perspective. Both
		// flags may be specified to create a duplex data stream.
		ELOOP_READABLE_PIPE = UV_READABLE_PIPE,
		ELOOP_WRITABLE_PIPE = UV_WRITABLE_PIPE
	} eloop_stdio_flags;

	typedef enum _processFlags {
		/*
		* Set the child process' user id. The user id is supplied in the `uid` field
		* of the options struct. This does not work on windows; setting this flag
		* will cause uv_spawn() to fail.
		*/
		ELOOP_PROCESS_SETUID = UV_PROCESS_SETUID,
		/*
		* Set the child process' group id. The user id is supplied in the `gid`
		* field of the options struct. This does not work on windows; setting this
		* flag will cause uv_spawn() to fail.
		*/
		ELOOP_PROCESS_SETGID = UV_PROCESS_SETGID,
		/*
		* Do not wrap any arguments in quotes, or perform any other escaping, when
		* converting the argument list into a command line string. This option is
		* only meaningful on Windows systems. On Unix it is silently ignored.
		*/
		ELOOP_PROCESS_WINDOWS_VERBATIM_ARGUMENTS = UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS,
		/*
		* Spawn the child process in a detached state - this will make it a process
		* group leader, and will effectively enable the child to keep running after
		* the parent exits.  Note that the child process will still keep the
		* parent's event loop alive unless the parent process calls uv_unref() on
		* the child's process handle.
		*/
		ELOOP_PROCESS_DETACHED = UV_PROCESS_DETACHED,
		/*
		* Hide the subprocess console window that would normally be created. This
		* option is only meaningful on Windows systems. On Unix it is silently
		* ignored.
		*/
		ELOOP_PROCESS_WINDOWS_HIDE = UV_PROCESS_WINDOWS_HIDE
	} eloop_process_flags;

//typedef struct uv_stdio_container_s {
//  uv_stdio_flags flags;
//
//  union {
//    uv_stream_t* stream;
//    int fd;
//  } data;
//} eloop_stdio_container_t;

	typedef struct _StreamNode {
		uint8_t flags;
		union {
			uv_stream_t* stream;
			int fd;
		} pipe;

	} StreamNode;

	typedef std::vector<StreamNode> Streams;

public:
	ChildProcess(Loop& loop, const char* cwd, char** env, uid_t uid, gid_t gid);
	int spawn(const char* exec, char** args, eloop_process_flags flags, Streams& pipes =Streams());

	int pid();
	int kill(int signum);
	static int kill(int pid, int signum);

protected: // event callback
	virtual void OnExit(int64_t exit_status, int term_signal) { close(); }

private: // impl of Handle
	void init() {} // do nothing because spawn() covers init()

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
