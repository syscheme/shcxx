#include "eloop_file.h"

namespace ZQ {
namespace eloop {

#define UVTYPED_HANDLE(_UVTYPE_T)  ((_UVTYPE_T *) &_context->handle)
#define CALL_ASSERT(_RET)         if (NULL == _context) return _RET
#define CALL_ASSERTV()            if (NULL == _context) return

#define CALLBACK_CTX(_CLASS, cbFUNC, PARAMS)  { _CLASS* ctx = static_cast<_CLASS *>(uvhandle->data); if (ctx) ctx->cbFUNC PARAMS; }

// -----------------------------
// class File
// -----------------------------
void FSMonitor::_cbFSevent(uv_fs_event_t *uvhandle, const char *filename, int events, int status)
{
	CALLBACK_CTX(FSMonitor, OnFileEvent, (filename, (uint) events, (ElpeError)status));

	//FSMonitor* self = static_cast<FSMonitor *>(handle->data);
	//if (self != NULL) {
	//	self->OnFileEvent(filename, (uint) events, (ElpeError)status);
	//}
}

void FSMonitor::init()
{
	CALL_ASSERTV();
	uv_fs_event_init(_loop.context_ptr(), UVTYPED_HANDLE(uv_fs_event_t));
	//this->Handle::init(loop);
	//uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	//return uv_fs_event_init(loop.context_ptr(), event);
}

int FSMonitor::monitor(const char *path, uint flags)
{
	CALL_ASSERT(-1);
	return uv_fs_event_start(UVTYPED_HANDLE(uv_fs_event_t), _cbFSevent, path, flags);

	//uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	//return uv_fs_event_start(event, _cbFSevent, path, flags);
}

int FSMonitor::stop()
{
	CALL_ASSERT(-1);
	return uv_fs_event_stop(UVTYPED_HANDLE(uv_fs_event_t));

	//uv_fs_event_t* event = (uv_fs_event_t *) context_ptr();
	//return uv_fs_event_stop(event);
}

std::string FSMonitor::path()
{
	char buffer[1024];
	size_t len = sizeof(buffer)-2;

	CALL_ASSERT("");
	if (0==uv_fs_event_getpath(UVTYPED_HANDLE(uv_fs_event_t), buffer, &len))
		return std::string(buffer, len);
	return "";

	//uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	//return uv_fs_event_getpath(event, buffer, size);
}

// -----------------------------
// class File
// -----------------------------
File::File(Loop& loop)
	:_loop(loop),
	_fb(0),
	_isAlloc(false),
	_buf(NULL),
	_len(0)
{
}

int File::open(const char* filename, uint flags, uint mode)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_open(_loop.context_ptr(),req,filename,flags,mode,_cbFileOpen);
}

int File::read(size_t len,int64_t offset)
{
	if (len > _len)
	{
		if (_isAlloc)
		{
			free(_buf);
			_buf = NULL;
		}
		_buf = (char*)malloc(len+1);
		_len = len;
	}
	memset(_buf,0,_len);
	uv_buf_t buf = uv_buf_init(_buf,len);
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	int ret =  uv_fs_read(_loop.context_ptr(),req,_fb,&buf,1,offset,_cbFileRead);
	return ret;
}

int File::read(char* data,size_t len, int64_t offset)
{
	uv_buf_t buf = uv_buf_init(data,len);
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_read(_loop.context_ptr(),req,_fb,&buf,1,offset,_cbFileRead);
}

int File::write(const char* data, size_t len, int64_t offset)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	uv_buf_t buf = uv_buf_init((char*)data,len);
	return uv_fs_write(_loop.context_ptr(),req,_fb,&buf,1,offset,_cbFileWrite);
}

int File::write(const ZQ::eloop::Handle::eloop_buf_t bufs[],unsigned int nbufs,int64_t offset)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_write(_loop.context_ptr(),req,_fb,bufs,nbufs,offset,_cbFileWrite);
}

int File::mkdir(const char* dirname, uint mode)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_mkdir(_loop.context_ptr(),req,dirname,mode,_cbMkdir);
}

int File::close()
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_close(_loop.context_ptr(),req,_fb,_cbFileClose);
}

void File::clean()
{
	if (_buf != NULL)
	{
		free(_buf);
		_buf = NULL;
		_len = 0;
	}
}

void File::setfb(int fb)
{
	_fb = fb;
}

void File::_cbFileOpen(uv_fs_t* req)
{
	File *h = static_cast<File *>(req->data);
	if (NULL != h)
	{
		h->setfb(req->result);
		h->OnOpen(req->result);
	}
	uv_fs_req_cleanup(req);
	delete req;
}

void File::_cbFileClose(uv_fs_t* req)
{
	File *h = static_cast<File *>(req->data);
	if (NULL != h)
	{
		h->clean();
		h->OnClose(req->result);
	}
	if (req != NULL)
	{
		uv_fs_req_cleanup(req);
		delete req;
		req = NULL;
	}
}

void File::_cbFileWrite(uv_fs_t* req)
{
	File *h = static_cast<File *>(req->data);
	if (NULL != h)
	{
		h->OnWrite(req->result);
	}
	uv_fs_req_cleanup(req);
	delete req;
}

void File::_cbFileRead(uv_fs_t* req)
{
	File *h = static_cast<File *>(req->data);
	if (NULL != h)
	{
		if (h->_buf)
			h->OnRead(h->_buf,req->result);
		else
			h->OnRead(req->result);
	}
	uv_fs_req_cleanup(req);
	delete req;
}

void File::_cbMkdir(uv_fs_t* req)
{
	File *h = static_cast<File *>(req->data);
	if (NULL != h)
	{
		h->OnMkdir(req->result);
	}
	uv_fs_req_cleanup(req);
	delete req;
}

// -----------------------------
// class Pipe
// -----------------------------
Pipe::Pipe() {
}

int Pipe::init(Loop &loop, int ipc) {
	if(ipc == 1)
	   uv_pipe_init(loop.context_ptr(),&_fdContainer,ipc);
	this->Handle::init(loop);
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_init(loop.context_ptr(), pipe, ipc);
}

int Pipe::open(uv_file file) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_open(pipe, file);
}

int Pipe::bind(const char *name) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_bind(pipe, name);
}

void Pipe::connect(const char *name) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	uv_connect_t* req = new uv_connect_t;

	uv_pipe_connect(req, pipe, name, _cbConnected);
}

int Pipe::getsockname(char *buffer, size_t *size) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_getsockname(pipe, buffer, size);
}

int Pipe::getpeername(char *buffer, size_t *size) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_getpeername(pipe, buffer, size);
}

void Pipe::pending_instances(int count) {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	uv_pipe_pending_instances(pipe, count);
}

int Pipe::pending_count() {
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_pipe_pending_count(pipe);
}

Pipe::eloop_handle_type Pipe::pending_type()
{
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return (eloop_handle_type)uv_pipe_pending_type(pipe);
}
#ifdef ZQ_OS_LINUX
int Pipe::sendfd(const eloop_buf_t bufs[],unsigned int nbufs,int fd)
{
	uv_setfd(&_fdContainer,fd);
	return write(bufs,nbufs,(uv_stream_t*)&_fdContainer);
}

int Pipe::acceptfd()
{
	uv_pipe_t* pipe = (uv_pipe_t *)context_ptr();
	return uv_acceptfd((uv_stream_t*)pipe);
}
#endif

void Pipe::_cbConnected(uv_connect_t *req, int status) {
	uv_stream_t* stream = req->handle;

	Pipe* self = static_cast<Pipe *>(stream->data);

	if (self != NULL) {
		//TODO: why wiped uv_connect_t here??
		self->OnConnected((ElpeError)status);
	}

	delete req;
}

} } // namespace ZQ::eloop
