#include "eloop.h"
#include "eloop_net.h"
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



// -----------------------------
// class Handle
// -----------------------------
Handle::Handle()
	:data(NULL),
	context(NULL),
	_isClose(false),
	_loop(NULL) 
{
}

Handle::~Handle() {

	if (context == NULL || !is_active())
		return;

	close();
	uv_run(context->handle.loop, UV_RUN_DEFAULT);
}

Handle::Handle(Handle &other) {
	context = other.context;
	data = other.data;
	_loop = other._loop;
	other.context = NULL;
	other.data = NULL;
	other._loop =NULL;
}

void Handle::_cbClose(uv_handle_t *uvhandle)
{
	Handle *h = static_cast<Handle *>(uvhandle->data);
	if (NULL != h)
	{
		h->_deleteContext();
		h->OnClose();
	}
}

Handle& Handle::operator=(Handle &other) {

	context = other.context;
	data = other.data;
	_loop = other._loop;

	other.context = NULL;
	other.data = NULL;
	other._loop =NULL;
	return *this;
}

void Handle::init(Loop &loop) {
	
	_loop = &loop;
	if (context != NULL)
		return;

	context = new uv_any_handle;
	context->handle.data = static_cast<void *>(this);
}

uv_handle_t *Handle::context_ptr()
{
	if (context)
		return &context->handle;

	return NULL;
}

int Handle::is_active() {
	if (context == NULL) return 0;
	return uv_is_active(&context->handle);
}

int Handle::is_closing()
{
	if (context == NULL) return elpuEAI_NODATA;
	return uv_is_closing(&context->handle);
}

void Handle::close()
{
	if (_isClose) return;
	_isClose = true;
	uv_close(&context->handle, _cbClose);
}

void Handle::_deleteContext()
{
	if (context == NULL) return;

	delete context;
	context = NULL;
}

void Handle::ref()
{
	if (context == NULL) return;
	uv_ref(&context->handle);
}

void Handle::unref()
{
	if (context == NULL) return;
	uv_unref(&context->handle);
}

int Handle::has_ref()
{
	if (context == NULL) return elpuEAI_NODATA;
	return uv_has_ref(&context->handle);
}

int Handle::send_buffer_size(int *value)
{
	if (context == NULL) return elpuEAI_NODATA;
	return uv_send_buffer_size(&context->handle, value);
}

int Handle::recv_buffer_size(int *value) 
{
	if (context == NULL) return elpuEAI_NODATA;
	return uv_recv_buffer_size(&context->handle, value);
}

int Handle::fileno(fd_t *fd)
{
	if (context == NULL) return elpuEAI_NODATA;
	return uv_fileno(&context->handle, fd);
}

Loop& Handle::get_loop()
{
	return *_loop;
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
// class Idle
// -----------------------------
Idle::Idle()
{
}

void Idle::_cbOnIdle(uv_idle_t* uvhandle)
{
	Idle *h = static_cast<Idle *>(uvhandle->data);
	if (NULL != h)
		h->OnIdle();
}

int Idle::init(Loop &loop) {
	this->Handle::init(loop);
	uv_idle_t* handle = (uv_idle_t *)context_ptr();
	return uv_idle_init(loop.context_ptr(), handle);
}

int Idle::start() {
	uv_idle_t* handle = (uv_idle_t *)context_ptr();
	return uv_idle_start(handle, _cbOnIdle);
}

int Idle::stop() {
	uv_idle_t* handle = (uv_idle_t *)context_ptr();
	return uv_idle_stop(handle);
}

// -----------------------------
// class Timer
// -----------------------------
/*
Timer::Timer()
{
}
*/
void Timer::_cbOnTimer(uv_timer_t *timer)
{
	Timer *h = static_cast<Timer *>(timer->data);
	if (NULL != h)
		h->OnTimer();
}

int Timer::init(Loop &loop) {
	this->Handle::init(loop);
	uv_timer_t * timer = (uv_timer_t *)context_ptr();
	return uv_timer_init(loop.context_ptr(), timer);
}

int Timer::start(uint64_t timeout, uint64_t repeat) {

	uv_timer_t* timer = (uv_timer_t *)context_ptr();
	return uv_timer_start(timer, _cbOnTimer, timeout, repeat);
}

int Timer::stop() {

	uv_timer_t* timer = (uv_timer_t *)context_ptr();
	return uv_timer_stop(timer);
}

int Timer::again() {
	uv_timer_t* timer = (uv_timer_t *)context_ptr();
	return uv_timer_again(timer);
}

void Timer::set_repeat(uint64_t repeat) {
	uv_timer_t* timer = (uv_timer_t *)context_ptr();
	uv_timer_set_repeat(timer, repeat);
}

uint64_t Timer::get_repeat() {
	uv_timer_t* timer = (uv_timer_t *)context_ptr();
	return uv_timer_get_repeat(timer);
}

// -----------------------------
// class Async
// -----------------------------
Async::Async() {
}

void Async::_cbAsync(uv_async_t *async)
{
	Async *h = static_cast<Async *>(async->data);
	if (NULL != h)
		h->OnAsync();
}

int Async::init(Loop &loop) {
	this->Handle::init(loop);
	uv_async_t * async = (uv_async_t *)context_ptr();
	return uv_async_init(loop.context_ptr(), async, _cbAsync);
}

int Async::send() {
	uv_async_t* async = (uv_async_t *)context_ptr();
	return uv_async_send(async);
}

// -----------------------------
// class Signal
// -----------------------------
Signal::Signal()
{
}

int Signal::init(Loop &loop) {
	this->Handle::init(loop);
	uv_signal_t* signal = (uv_signal_t *)context_ptr();
	return uv_signal_init(loop.context_ptr(), signal);
}

int Signal::start(int signum) {
	uv_signal_t* signal = (uv_signal_t *)context_ptr();

	return uv_signal_start(signal, _cbSignal, signum);
}

int Signal::stop() {
	uv_signal_t* signal = (uv_signal_t *)context_ptr();
	return uv_signal_stop(signal);
}

void Signal::_cbSignal(uv_signal_t *signal, int signum) {

	Signal* self = static_cast<Signal *>(signal->data);
	if (self != NULL) {
		self->OnSignal(signum);
	}
}

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
// class Process
// -----------------------------

void Process::setenv(char** env)
{
	_opt.env = env;
}
void Process::setcwd(const char* cwd)
{
	_opt.cwd = cwd;
}
void Process::setflags(eloop_process_flags flags)
{
	_opt.flags = flags;
}
void Process::setuid(eloop_uid_t uid)
{
	_opt.uid = uid;
}

void Process::setgid(eloop_gid_t gid)
{
	_opt.gid = gid;
}

int Process::spawn(const char* file,char** args,eloop_stdio_container_t* container,int stdio_count)
{
	_opt.file = file;
	_opt.args = args;
	_opt.exit_cb = _cbExit;
// 	_opt.flags = 0;
// 	_opt.cwd = NULL;
// 	_opt.env = NULL;
	if (stdio_count > 0)
	{
		_opt.stdio_count = stdio_count;
		_opt.stdio = container;
	}
	uv_process_t* h = (uv_process_t*)context_ptr();
	return uv_spawn(get_loop().context_ptr(),h,&_opt);
}
/*
int Process::spawn()
{
	Process::eloop_stdio_container_t child_stdio[3];

	child_stdio[0].flags = (Process::eloop_stdio_flags)(Process::ELOOP_CREATE_PIPE | Process::ELOOP_READABLE_PIPE);
	child_stdio[0].data.stream = (uv_stream_t*)(work.pipe->context_ptr());
	child_stdio[1].flags = (Process::eloop_stdio_flags)Process::ELOOP_IGNORE;
	child_stdio[2].flags = (Process::eloop_stdio_flags)Process::ELOOP_INHERIT_FD;
	child_stdio[2].data.fd = 2;

	_opt.stdio_count = 3;
	_opt.stdio = child_stdio;
}
*/
int Process::pid()
{
	uv_process_t* h = (uv_process_t*)context_ptr();
	return h->pid;
}

int Process::kill(int signum)
{
	uv_process_t* h = (uv_process_t*)context_ptr();
	return uv_process_kill(h,signum);
}

int Process::kill(int pid,int signum)
{
	return uv_kill(pid,signum);
}

void Process::_cbExit(uv_process_t* handle,int64_t exit_status,int term_signal)
{
	Process* self = static_cast<Process *>(handle->data);
	if (self != NULL) {
		self->OnExit(exit_status,term_signal);
	}
}

}} // namespace ZQ::eloop