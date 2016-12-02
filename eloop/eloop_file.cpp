#include "eloop_file.h"

namespace ZQ {
namespace eloop {

// -----------------------------
// class File
// -----------------------------
FileEvent::FileEvent() {
}

int FileEvent::init(Loop &loop) {
	this->Handle::init(loop);
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	return uv_fs_event_init(loop.context_ptr(), event);
}

int FileEvent::start(const char *path, fs_event_flags flags) {
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();

	return uv_fs_event_start(event, _cbFSevent, path,flags);
}

int FileEvent::stop() {
	uv_fs_event_t* event = (uv_fs_event_t *) context_ptr();
	return uv_fs_event_stop(event);
}

int FileEvent::getpath(char *buffer, size_t *size) {
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	return uv_fs_event_getpath(event, buffer, size);
}

void FileEvent::_cbFSevent(uv_fs_event_t *handle, const char *filename, int events, int status) {

	FileEvent* self = static_cast<FileEvent *>(handle->data);
	if (self != NULL) {
		self->OnFileEvent(filename, (fs_event)events, (ElpeError)status);
	}
}


// -----------------------------
// class File
// -----------------------------
File::File(Loop& loop)
	:_loop(loop),
	_fb(0)
{
}

int File::open(const char* filename,int flags,int mode)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	return uv_fs_open(_loop.context_ptr(),req,filename,flags,mode,_cbFileOpen);
}

int File::read(size_t len)
{
	_buf = (char*)malloc(len);
	memset(_buf,0,len);
	uv_buf_t buf = uv_buf_init(_buf,len);
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	int ret =  uv_fs_read(_loop.context_ptr(),req,_fb,&buf,1,-1,_cbFileRead);
	return ret;
}

int File::write(const char* data,size_t len)
{
	uv_fs_t *req = new uv_fs_t;
	req->data = static_cast<void *>(this);
	uv_buf_t buf = uv_buf_init((char*)data,len);
	return uv_fs_write(_loop.context_ptr(),req,_fb,&buf,1,-1,_cbFileWrite);
}

int File::mkdir(const char* dirname,int mode)
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
		h->OnClose(req->result);
	}
	uv_fs_req_cleanup(req);
	delete req;
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
		h->OnRead(h->_buf,req->result);
		free(h->_buf);
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
