#include "Log.h"
#include "SystemUtils.h"

#include "eloop.h"
// #include "eloop_net.h"
//#include <stdlib.h>
namespace ZQ {
namespace eloop {

#define DEFAULT_THREADPOOL_SIZE 4
#define ELOOP_THREADPOOL_SIZE  "UV_THREADPOOL_SIZE"

Handle::ElpeError Handle::uvErr2ElpeErr(int errCode)
{
	switch(errCode)
	{
	case 0								  : return elpeSuccess;
	case EPERM							  : return elpeNotPermitted;		// operation not permitted
	case ENOENT							  : return elpeNoFile;				//no such file or directory
	case ESRCH							  : return elpeNoProcess;			//no such process
	case EINTR							  : return elpeIntr;				//interrupted system call
	case EIO							  : return elpeIO;					//i/o error
	case ENXIO							  : return elpeNotDevOrAddress;		//no such device or address
	case E2BIG							  : return elpeArgTooLong;			//argument list too long
	case ENOEXEC						  : return elpeENOEXEC;
	case EBADF							  : return elpeBadFd;				//bad file descriptor

	case ECHILD							  : return elpeECHILD;				//bad file descriptor
	case EAGAIN							  : return elpeResUnavailable;		//resource temporarily unavailable
	case ENOMEM							  : return elpeNotEnoughMemory;		//not enough memory
	case EACCES							  : return elpePermissionDenied;	//permission denied
	case EFAULT							  : return elpeBadAddress;			//bad address in system call argument
	case EBUSY							  : return elpeResBusy;				//resource busy or locked
	case EEXIST							  : return elpeFileAlreadyExists;	//file already exists
	case EXDEV							  : return elpeNotPermittedLink;	//cross-device link not permitted
	case ENODEV							  : return elpeNotDevice;			//no such device
	case ENOTDIR						  : return elpeNotDirectory;		//not a directory
	case EISDIR							  : return elpeEISDIR;				//illegal operation on a directory
	case ENFILE							  : return elpeENFILE;				//file table overflow
	case EMFILE							  : return elpeTooMany;				//too many open files
	case ENOTTY							  : return elpeENOTTY;
	case EFBIG							  : return elpeFileTooLarge;		//file too large
	case ENOSPC							  : return elpeNoSpace;				//no space left on device
	case ESPIPE							  : return elpeInvalidSeek;			//invalid seek
	case EROFS							  : return elpeROFS;				//read-only file system
	case EMLINK							  : return elpeTooManyLinks;		//too many links
	case EPIPE							  : return elpeBorkenPipe;			//broken pipe
	case EDOM							  : return elpeEDOM;
	case EDEADLK						  : return elpeEDEADLK;
	case ENAMETOOLONG					  : return elpeNameTooLong;			//name too long
	case ENOLCK							  : return elpeNOLCK;
	case ENOSYS							  : return elpeFunNotImpl;			//function not implemented
	case ENOTEMPTY						  : return elpeDirNotEmpty;			//directory not empty

	case UV__EOF						  : return elpuEOF;
	case UV__UNKNOWN					  : return elpeUNKNOWN;
	case UV__EAI_ADDRFAMILY				  : return elpuEAI_ADDRFAMILY;
	case UV__EAI_AGAIN					  : return elpuEAI_AGAIN;
	case UV__EAI_BADFLAGS				  : return elpuEAI_BADFLAGS;
	case UV__EAI_CANCELED				  : return elpuEAI_CANCELED;
	case UV__EAI_FAIL					  : return elpuEAI_FAIL;
	case UV__EAI_FAMILY					  : return elpuEAI_FAMILY;
	case UV__EAI_MEMORY					  : return elpuEAI_MEMORY;
	case UV__EAI_NODATA					  : return elpuEAI_NODATA;
	case UV__EAI_NONAME					  : return elpuEAI_NONAME;
	case UV__EAI_OVERFLOW				  : return elpuEAI_OVERFLOW;
	case UV__EAI_SERVICE				  : return elpuEAI_SERVICE;
	case UV__EAI_SOCKTYPE				  : return elpuEAI_SOCKTYPE;
	case UV__EAI_BADHINTS				  : return elpuEAI_BADHINTS;
	case UV__EAI_PROTOCOL				  : return elpuEAI_PROTOCOL;

	case UV__E2BIG						  : return elpuE2BIG;
	case UV__EACCES						  : return elpuEACCES;
	case UV__EADDRINUSE					  : return elpuEADDRINUSE;
	case UV__EADDRNOTAVAIL				  : return elpuEADDRNOTAVAIL;
	case UV__EAGAIN						  : return elpuEAGAIN;
	case UV__EALREADY					  : return elpuEALREADY;
	case UV__EBADF						  : return elpuEBADF;
	case UV__EBUSY						  : return elpuEBUSY;
	case UV__ECANCELED					  : return elpuECANCELED;
	case UV__ECHARSET					  : return elpuECHARSET;
	case UV__ECONNABORTED				  : return elpuECONNABORTED;
	case UV__ECONNREFUSED				  : return elpuECONNREFUSED;
	case UV__ECONNRESET					  : return elpuECONNRESET;
	case UV__EDESTADDRREQ				  : return elpuEDESTADDRREQ;
	case UV__EEXIST						  : return elpuEEXIST;
	case UV__EFAULT				          : return elpuEFAULT;
	case UV__EHOSTUNREACH				  : return elpuEHOSTUNREACH;
	case UV__EINTR						  : return elpuEINTR;
	case UV__EINVAL						  : return elpuEINVAL;
	case UV__EIO						  : return elpuEIO;
	case UV__EISCONN					  : return elpuEISCONN;
	case UV__EISDIR						  : return elpuEISDIR;
	case UV__ELOOP						  : return elpuELOOP;
	case UV__EMFILE						  : return elpuEMFILE;
	case UV__EMSGSIZE					  : return elpuEMSGSIZE;
	case UV__ENAMETOOLONG				  : return elpuENAMETOOLONG;
	case UV__ENETDOWN					  : return elpuENETDOWN;
	case UV__ENETUNREACH				  : return elpuENETUNREACH;
	case UV__ENFILE						  : return elpuENFILE;
	case UV__ENOBUFS					  : return elpuENOBUFS;
	case UV__ENODEV						  : return elpuENODEV;
	case UV__ENOENT						  : return elpuENOENT;
	case UV__ENOMEM						  : return elpuENOMEM;
	case UV__ENONET						  : return elpuENONET;
	case UV__ENOSPC						  : return elpuENOSPC;
	case UV__ENOSYS						  : return elpuENOSYS;
	case UV__ENOTCONN					  : return elpuENOTCONN;
	case UV__ENOTDIR					  : return elpuENOTDIR;
	case UV__ENOTEMPTY					  : return elpuENOTEMPTY;
	case UV__ENOTSOCK					  : return elpuENOTSOCK;
	case UV__ENOTSUP					  : return elpuENOTSUP;
	case UV__EPERM						  : return elpuEPIPE;
	case UV__EPIPE						  : return elpuEPIPE;
	case UV__EPROTO						  : return elpuEPROTO;
	case UV__EPROTONOSUPPORT			  : return elpuEPROTONOSUPPORT;
	case UV__EPROTOTYPE					  : return elpuEPROTOTYPE;
	case UV__EROFS						  : return elpuEROFS;
	case UV__ESHUTDOWN					  : return elpuESHUTDOWN;
	case UV__ESPIPE						  : return elpuESPIPE;
	case UV__ESRCH						  : return elpuESRCH;
	case UV__ETIMEDOUT					  : return elpuETIMEDOUT;
	case UV__ETXTBSY					  : return elpuETXTBSY;
	case UV__EXDEV						  : return elpuEXDEV;
	case UV__EFBIG						  : return elpuEFBIG;
	case UV__ENOPROTOOPT				  : return elpuENOPROTOOPT;
	case UV__ERANGE						  : return elpuERANGE;
	case UV__ENXIO						  : return elpuENXIO;
	case UV__EMLINK						  : return elpuEMLINK;
	case UV__EHOSTDOWN					  : return elpuEHOSTDOWN;
	}

	return elpuUnKnown;
}

//static int64 usStampNow()
//{
//#ifdef ZQ_OS_LINUX

//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	return tv.tv_sec*1000*1000 + tv.tv_usec;
//#else
//	return 0;
//#endif
//}

// -----------------------------
// class Handle
// -----------------------------
Handle::Handle(Loop& loop)
	: _context(NULL), _loop(loop) // , _isClose(false), data(NULL)
{
	_context = new uv_any_handle;
	_context->handle.data = static_cast<void *>(this);
}

Handle::~Handle()
{
	if (_context == NULL)
		return;

	if (uv_is_active(&_context->handle))
		uv_close(&_context->handle, _cbDeactived);

	while (uv_is_closing(&_context->handle) && _loop.alive())
	{
		if (_loop.inLoop())
			uv_run(_context->handle.loop, UV_RUN_ONCE);
		else SYS::sleep(1);
	}

	delete _context;
	_context = NULL;
}

#define UVTYPED_HANDLE(_UVTYPE_T)  ((_UVTYPE_T *) &_context->handle)
#define CALL_ASSERT(_RET)         if (NULL == _context) return _RET
#define CALL_ASSERTV()            if (NULL == _context) return

#define CALLBACK_CTX(_CLASS, cbFUNC, PARAMS)  { _CLASS* ctx = static_cast<_CLASS *>(uvhandle->data); if (ctx) ctx->cbFUNC PARAMS; }

void Handle::_cbDeactived(uv_handle_t *uvhandle)
{
	CALLBACK_CTX(Handle, OnDeactived, ());
	//Handle *h = static_cast<Handle *>(uvhandle->data);
	//if (NULL != h)
	//	h->OnDeactived();
}

bool Handle::isActive()
{
	CALL_ASSERT(false);
	return 0 != uv_is_active(UVTYPED_HANDLE(uv_handle_t));
}


//Handle::Handle(Handle &other)
//{
//	_context = other._context;
//	data = other.data;
//	_loop = other._loop;
//	other.context = NULL;
//	other.data = NULL;
//	other._loop =NULL;
//}
//
//
//Handle& Handle::operator=(Handle &other) {
//
//	context = other.context;
//	data = other.data;
//	_loop = other._loop;
//
//	other.context = NULL;
//	other.data = NULL;
//	other._loop =NULL;
//	return *this;
//}

//
//uv_handle_t *Handle::context_ptr()
//{
//	if (context)
//		return &context->handle;
//
//	return NULL;
//}
//
//int Handle::is_active() {
//	if (context == NULL) return 0;
//	return uv_is_active(&context->handle);
//}
//
//int Handle::is_closing()
//{
//	if (context == NULL) return elpuEAI_NODATA;
//	return uv_is_closing(&context->handle);
//}

void Handle::deactive()
{
	if (_context == NULL || uv_is_closing(&_context->handle))
		return;

	if (uv_is_active(&_context->handle))
		uv_close(&_context->handle, _cbDeactived);
}

//void Handle::ref()
//{
//	if (context == NULL) return;
//	uv_ref(&context->handle);
//}
//
//void Handle::unref()
//{
//	if (context == NULL) return;
//	uv_unref(&context->handle);
//}
//
//int Handle::has_ref()
//{
//	if (context == NULL) return elpuEAI_NODATA;
//	return uv_has_ref(&context->handle);
//}

int Handle::send_buffer_size(int *value)
{
	if (_context == NULL) return elpuEAI_NODATA;
	return uv_send_buffer_size(&_context->handle, value);
}

int Handle::recv_buffer_size(int *value) 
{
	if (_context == NULL) return elpuEAI_NODATA;
	return uv_recv_buffer_size(&_context->handle, value);
}

int Handle::fileno(fd_t *fd)
{
	if (_context == NULL) return elpuEAI_NODATA;
	return uv_fileno(&_context->handle, fd);
}

// -----------------------------
// class EloopRequest
// -----------------------------
EloopRequest::EloopRequest()
{
	context = new uv_any_req;
	context->req.data = static_cast<void *>(this);
}

EloopRequest::~EloopRequest()
{
	if (context == NULL)
		return;

	cancel();
	delete context;
	context = NULL;
}

int EloopRequest::cancel()
{
	if (context == NULL) return Handle::elpuEAI_NODATA;
	return uv_cancel(&context->req);
}

uv_req_t *EloopRequest::context_ptr()
{
	if (context)
		return &context->req;
	return NULL;
}

// -----------------------------
// class Loop
// -----------------------------
Loop::Loop(bool defaultLoop)
:_uvLoop(defaultLoop ? uv_default_loop() : new uv_loop_t)
{
	if (!defaultLoop)
		uv_loop_init(_uvLoop);

	_uvLoop->data = this;
}

Loop::~Loop()
{
	close();
}

void Loop::_doWalk(uv_handle_t* uvhandle, void *arg)
{
	uv_loop_t* uvloop = uvhandle->loop;
	Loop* l = static_cast<Loop *>(uvloop->data);
	if (NULL != l)
		l->OnWalk(static_cast<Handle *>(uvhandle->data), arg);
}


bool Loop::run(RunMode mode)
{
	_thrdId = __THREADID__;
	return uv_run(_uvLoop, (uv_run_mode)mode) == 0;
}

void Loop::stop()
{
	uv_stop(_uvLoop);
}

int Loop::close()
{
	if (_uvLoop)
	{
		int r = uv_loop_close(_uvLoop);
		_uvLoop = NULL;
		return r;
	}

	return -1;
}

int Loop::alive()
{
	return uv_loop_alive(_uvLoop);
}

int Loop::backend_fd()
{
	return uv_backend_fd(_uvLoop);
}

int Loop::backend_timeout()
{
	return uv_backend_timeout(_uvLoop);
}

unsigned int Loop::getThreadPoolSize()
{
	const char* val = getenv(ELOOP_THREADPOOL_SIZE);
	if (val != NULL)
		return atoi(val);

	return DEFAULT_THREADPOOL_SIZE;
}
int Loop::setThreadPoolSize(const unsigned int size)
{
	char chSize[8];
	sprintf(chSize,"%d",size);

#ifdef ZQ_OS_LINUX
	return setenv(ELOOP_THREADPOOL_SIZE,chSize,1);
#else	
	return _putenv_s(ELOOP_THREADPOOL_SIZE,chSize);
#endif
}

uint64_t Loop::now()
{
	return uv_now(_uvLoop);
}

void Loop::update_time()
{
	uv_update_time(_uvLoop);
}

uv_loop_t* Loop::context_ptr()
{
	return _uvLoop;
}

void Loop::walk(void *arg)
{
	uv_walk(_uvLoop, _doWalk, arg);
}

// -----------------------------
// class IterationBlocker
// -----------------------------
void IterationBlocker::init()
{
	CALL_ASSERTV();
	uv_idle_init(_loop.context_ptr(), UVTYPED_HANDLE(uv_idle_t));
}

void IterationBlocker::_cbOnIdle(uv_idle_t* uvhandle)
{
	CALLBACK_CTX(IterationBlocker, OnIdle, ())
	//IterationBlocker *h = static_cast<IterationBlocker *>(uvhandle->data);
	//if (NULL != h)
	//	h->OnIdle();
}

int IterationBlocker::start()
{
	CALL_ASSERT(-1);
	return uv_idle_start(UVTYPED_HANDLE(uv_idle_t), _cbOnIdle);
}

int IterationBlocker::stop()
{
	CALL_ASSERT(-1);
	return uv_idle_stop(UVTYPED_HANDLE(uv_idle_t));
}

// -----------------------------
// class Timer
// -----------------------------
void Timer::_cbOnTimer(uv_timer_t *uvhandle)
{
	CALLBACK_CTX(Timer, OnTimer, ());
}

void Timer::init()
{
	CALL_ASSERTV();
	uv_timer_init(_loop.context_ptr(), UVTYPED_HANDLE(uv_timer_t));

	//this->Handle::init(loop);
	//uv_timer_t * timer = (uv_timer_t *)context_ptr();
	//return uv_timer_init(loop.context_ptr(), timer);
}

int Timer::start(uint64_t timeout, bool bRepeat)
{
	CALL_ASSERT(-1);
	uint64_t repeat = timeout;
	if (!bRepeat && timeout <=0)
		repeat =-1;

	return uv_timer_start(UVTYPED_HANDLE(uv_timer_t), _cbOnTimer, timeout, repeat);

	//uv_timer_t* timer = (uv_timer_t *)context_ptr();
	//return uv_timer_start(timer, _cbOnTimer, timeout, repeat);
}

int Timer::stop()
{
	CALL_ASSERT(-1);
	return uv_timer_stop(UVTYPED_HANDLE(uv_timer_t));
}

int Timer::again() {
	CALL_ASSERT(-1);
	return uv_timer_again(UVTYPED_HANDLE(uv_timer_t));
}

void Timer::set_repeat(uint64_t repeat)
{
	CALL_ASSERTV();
	uv_timer_set_repeat(UVTYPED_HANDLE(uv_timer_t), repeat);
}

uint64_t Timer::get_repeat()
{
	CALL_ASSERT(-1);
	return uv_timer_get_repeat(UVTYPED_HANDLE(uv_timer_t));
}

// -----------------------------
// class Interruptor
// -----------------------------
// used
void Interruptor::_cbAsync(uv_async_t *uvhandle)
{
	CALLBACK_CTX(Interruptor, OnWakedUp, ());
	//Interruptor *h = static_cast<Interruptor *>(async->data);
	//if (NULL != h)
	//	h->OnWakedUp();
}

void Interruptor::init()
{
	CALL_ASSERTV();
	uv_async_init(_loop.context_ptr(), UVTYPED_HANDLE(uv_async_t), _cbAsync);

	//this->Handle::init(loop);
	//uv_async_t * async = (uv_async_t *)context_ptr();
	//return uv_async_init(loop.context_ptr(), async, _cbAsync);
}

int Interruptor::wakeup()
{
	CALL_ASSERT(-1);
	return uv_async_send(UVTYPED_HANDLE(uv_async_t));

	//uv_async_t* async = (uv_async_t *)context_ptr();
	//return uv_async_send(async);
}

// -----------------------------
// class SysSignalSink
// -----------------------------
void SysSignalSink::_cbSignal(uv_signal_t *uvhandle, int signum)
{
	CALLBACK_CTX(SysSignalSink, OnSignal, (signum));

	//SysSignalSink* self = static_cast<SysSignalSink *>(signal->data);
	//if (self != NULL) {
	//	self->OnSignal(signum);
	//}
}

void SysSignalSink::init()
{
	CALL_ASSERTV();
	uv_signal_init(_loop.context_ptr(), UVTYPED_HANDLE(uv_signal_t));
	//this->Handle::init(loop);
	//uv_signal_t* signal = (uv_signal_t *)context_ptr();
	//return uv_signal_init(loop.context_ptr(), signal);
}

int SysSignalSink::subscribe(int signum)
{
	CALL_ASSERT(-1);
	return uv_signal_start(UVTYPED_HANDLE(uv_signal_t), _cbSignal, signum);
	//uv_signal_t* signal = (uv_signal_t *)context_ptr();
	//return uv_signal_start(signal, _cbSignal, signum);
}

int SysSignalSink::stop() {
	CALL_ASSERT(-1);
	return uv_signal_stop(UVTYPED_HANDLE(uv_signal_t));
	//uv_signal_t* signal = (uv_signal_t *)context_ptr();
	//return uv_signal_stop(signal);
}

// -----------------------------
// class CpuInfo
// -----------------------------
CpuInfo::CpuInfo()
	:_count(0),
	_info(NULL),
	_bIsAccess(false)
{
	getCpuInfo();
}

CpuInfo::~CpuInfo()
{
	freeCpuInfo();
}

int CpuInfo::getCpuCount()
{
	return _count;
}

CpuInfo::cpu_info* CpuInfo::getCpuInfo()
{
	if (_bIsAccess)
	{
		freeCpuInfo();
	}
	int ret = uv_cpu_info(&_info,&_count);
	if (ret != 0)
		return NULL;
	_bIsAccess = true;
	return _info;
}

void CpuInfo::freeCpuInfo()
{
	if (_bIsAccess)
	{
		uv_free_cpu_info(_info,_count);
		_info = NULL;
		_count = 0;
		_bIsAccess =false;
	}
}

// -----------------------------
// class ChildProcess
// -----------------------------
ChildProcess::ChildProcess(Loop& loop, const char* cwd, char** env, uid_t uid, gid_t gid)
: Handle(loop)
{
	memset(&_opt, 0, sizeof(_opt));
	_opt.env = env;
	_opt.cwd = cwd;
	_opt.uid = uid;
	_opt.gid = gid;
}

void ChildProcess::_cbExit(uv_process_t* uvhandle, int64_t exit_status, int term_signal)
{
	CALLBACK_CTX(ChildProcess, OnExit, (exit_status, term_signal));
	//ChildProcess* self = static_cast<ChildProcess *>(handle->data);
	//if (self != NULL) {
	//	self->OnExit(exit_status,term_signal);
	//}
}

//void ChildProcess::setenv(char** env)
//{
//	_opt.env = env;
//}
//void ChildProcess::setcwd(const char* cwd)
//{
//	_opt.cwd = cwd;
//}
//void ChildProcess::setflags(eloop_process_flags flags)
//{
//	_opt.flags = flags;
//}
//void ChildProcess::setuid(eloop_uid_t uid)
//{
//	_opt.uid = uid;
//}
//
//void ChildProcess::setgid(eloop_gid_t gid)
//{
//	_opt.gid = gid;
//}
//

int ChildProcess::spawn(const char* exec, char** args, eloop_process_flags flags, const ChildProcess::Streams& pipes)
{
	_opt.file = exec;
	_opt.args = args;
	_opt.exit_cb = _cbExit;
	_opt.flags = flags;

	for (size_t i=0; i <pipes.size(); i++)
	{
// TODO
	}

	//if (stdio_count > 0)
	//{
	//	_opt.stdio_count = stdio_count;
	//	_opt.stdio = container;
	//}

	return uv_spawn(_loop.context_ptr(), UVTYPED_HANDLE(uv_process_t), &_opt);
}

int ChildProcess::pid()
{
	CALL_ASSERT(-1);
	return UVTYPED_HANDLE(uv_process_t)->pid;

	//uv_process_t* h = (uv_process_t*) context_ptr();
	//return h->pid;
}

int ChildProcess::kill(int signum)
{
	CALL_ASSERT(-1);
	return uv_process_kill(UVTYPED_HANDLE(uv_process_t), signum);
	//uv_process_t* h = (uv_process_t*)context_ptr();
	//return uv_process_kill(h,signum);
}

int ChildProcess::kill(int pid,int signum)
{
	return uv_kill(pid, signum);
}

}} // namespace ZQ::eloop
