#include "eloop.h"

namespace ZQ {
	namespace eloop {

		// -----------------------------
		// class Handle
		// -----------------------------
		Handle::Handle() {
			context = nullptr;
			//	data = NULL;
		}

		Handle::~Handle() {
			if (context) {
				if (is_active()) {
					close();
					uv_run(context->handle.loop, UV_RUN_DEFAULT);
				}
				delete context;
			}
		}

		Handle::Handle(Handle &&other) {
			context = other.context;
			other.context = nullptr;
		}

		Handle &Handle::operator=(Handle &&other) {
			context = other.context;
			other.context = nullptr;
			return *this;
		}

		void Handle::init() {
			context = new uv_any_handle;
			context->handle.data = static_cast<void *>(this);
		}

		uv_handle_t *Handle::context_ptr() {
			if (context)
				return &context->handle;
			return nullptr;
		}

		int Handle::is_active() {
			return uv_is_active(&context->handle);
		}

		int Handle::is_closing() {
			return uv_is_closing(&context->handle);
		}

		void Handle::close() {
			uv_close(&context->handle, _cbClose);
		}

		void Handle::ref() {
			uv_ref(&context->handle);
		}

		void Handle::unref() {
			uv_unref(&context->handle);
		}

		int Handle::has_ref() {
			return uv_has_ref(&context->handle);
		}

		int Handle::send_buffer_size(int *value) {
			return uv_send_buffer_size(&context->handle, value);
		}

		int Handle::recv_buffer_size(int *value) {
			return uv_recv_buffer_size(&context->handle, value);
		}

		int Handle::fileno(fd_t *fd) {
			return uv_fileno(&context->handle, fd);
		}

		// -----------------------------
		// class Loop
		// -----------------------------
		Loop::Loop(bool defaultLoop)
			:_uvLoop(defaultLoop ? uv_default_loop() : new uv_loop_t)
		{
			if (!defaultLoop)
			{
				uv_loop_init(_uvLoop);
			}
			_uvLoop->data = this;
		}

		Loop::~Loop()
		{
			close();
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
				return uv_loop_close(_uvLoop);
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

		void Loop::walk(void *arg) {

			uv_walk(_uvLoop, _doWalk, arg);
		}

		// -----------------------------
		// class Idle
		// -----------------------------
		Idle::Idle() {
		}

		int Idle::init(Loop &loop) {
			this->Handle::init();
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

		Timer::Timer() {
		}

		int Timer::init(Loop &loop) {
			this->Handle::init();
			uv_timer_t * timer = (uv_timer_t *)context_ptr();
			return uv_timer_init(loop.context_ptr(), timer);
		}

		int Timer::start(uint64_t timeout, uint64_t repeat) {
			uv_timer_t* timer = (uv_timer_t *)context_ptr();
			return uv_timer_start(timer, timer_cb, timeout, repeat);
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

		int Async::init(Loop &loop) {
			this->Handle::init();
			uv_async_t * async = (uv_async_t *)context_ptr();
			return uv_async_init(loop.context_ptr(), async, async_cb);
		}

		int Async::send() {
			uv_async_t* async = (uv_async_t *)context_ptr();
			return uv_async_send(async);
		}

		// -----------------------------
		// class Signal
		// -----------------------------
		Signal::Signal() {
		}

		int Signal::init(Loop &loop) {
			this->Handle::init();
			uv_signal_t* signal = (uv_signal_t *)context_ptr();
			return uv_signal_init(loop.context_ptr(), signal);
		}

		int Signal::start(int signum) {
			uv_signal_t* signal = (uv_signal_t *)context_ptr();

			return uv_signal_start(signal, signal_cb, signum);
		}

		int Signal::stop() {
			uv_signal_t* signal = (uv_signal_t *)context_ptr();
			return uv_signal_stop(signal);
		}

		void Signal::signal_cb(uv_signal_t *signal, int signum) {

			Signal* self = static_cast<Signal *>(signal->data);
			if (self != NULL) {
				self->OnSignal(self, signum);
			}
		}
	}
} // namespace ZQ::eloop