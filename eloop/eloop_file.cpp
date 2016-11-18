#include "eloop_file.h"

namespace ZQ {
namespace eloop {

// -----------------------------
// class File
// -----------------------------
File::File() {
}

int File::init(Loop &loop) {
	this->Handle::init();
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	return uv_fs_event_init(loop.context_ptr(), event);
}

int File::start(const char *path, unsigned int flags) {
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();

	return uv_fs_event_start(event, _cbFSevent, path, flags);
}

int File::stop() {
	uv_fs_event_t* event = (uv_fs_event_t *) context_ptr();
	return uv_fs_event_stop(event);
}

int File::getpath(char *buffer, size_t *size) {
	uv_fs_event_t* event = (uv_fs_event_t *)context_ptr();
	return uv_fs_event_getpath(event, buffer, size);
}

void File::_cbFSevent(uv_fs_event_t *handle, const char *filename, int events, int status) {

	File* self = static_cast<File *>(handle->data);
	if (self != NULL) {
		self->OnFileEvent(filename, events, status);
	}
}


// -----------------------------
// class Pipe
// -----------------------------
Pipe::Pipe() {
}

int Pipe::init(Loop &loop, int ipc) {
	this->Handle::init();
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
		self->OnConnected(status);
	}

	delete req;
}

} } // namespace ZQ::eloop
