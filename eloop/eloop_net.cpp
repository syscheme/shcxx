#include "eloop_net.h"
namespace ZQ {
	namespace eloop {

		// -----------------------------
		// class Stream
		// -----------------------------

		Stream::Stream() {
		}

		int Stream::shutdown() {
			uv_shutdown_t* req = new uv_shutdown_t;
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_shutdown(req, stream, shutdown_cb);
		}

		int Stream::listen() {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_listen(stream, SOMAXCONN, connection_cb);
		}

		int Stream::accept(Stream &client) {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_accept(stream, (uv_stream_t *)client.context_ptr());
		}

		int Stream::read_start() {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_read_start(stream, alloc_cb, read_cb);
		}

		int Stream::read_stop() {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_read_stop(stream);
		}

		int Stream::write(const char *buf, size_t length) {
			uv_buf_t wbuf = uv_buf_init((char *)buf, length);

			uv_write_t *req = new uv_write_t;
			uv_stream_t * stream = (uv_stream_t *)context_ptr();
			return uv_write(req, stream, &wbuf, 1, write_cb);
		}


		int Stream::write(const char *buf, size_t length, Stream *send_handle) {

			uv_buf_t wbuf = uv_buf_init((char *)buf, length);
			uv_write_t *req = new uv_write_t;
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_write2(req, stream, &wbuf, 1, (uv_stream_t *)send_handle->context_ptr(), write_cb);
		}

		int Stream::try_write(const char *buf, size_t length) {
			uv_buf_t wbuf = uv_buf_init((char *)buf, length);

			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_try_write(stream, &wbuf, 1);
		}


		int Stream::set_blocking(int blocking) {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_stream_set_blocking(stream, blocking);
		}

		int Stream::is_readable() {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_is_readable(stream);
		}

		int Stream::is_writeable() {
			uv_stream_t* stream = (uv_stream_t *)context_ptr();
			return uv_is_writable(stream);
		}

		void Stream::shutdown_cb(uv_shutdown_t *req, int status) {
			uv_stream_t *stream = req->handle;

			//			Handle* handle = static_cast<Handle *>(stream->data);
			//			Stream* self = static_cast<Stream *>(handle);
			Stream* self = static_cast<Stream *>(stream->data);
			if (self != NULL) {
				self->OnShutdown_cb(self, status);
			}
			delete req;
		}

		void Stream::connection_cb(uv_stream_t *stream, int status) {

			Stream* self = static_cast<Stream *>(stream->data);
			if (self != NULL) {
				self->OnConnection_cb(self, status);
			}
		}

		void Stream::alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {

			Stream* self = static_cast<Stream *>(handle->data);
			if (self != NULL) {
				self->OnAlloc_cb(self, suggested_size, buf);
			}
		}

		void Stream::read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {

			Stream* self = static_cast<Stream *>(stream->data);
			if (self != NULL) {
				self->OnRead_cb(self, nread, buf);
			}
		}

		void Stream::write_cb(uv_write_t *req, int status) {
			uv_stream_t *stream = req->handle;

			Stream* self = static_cast<Stream *>(stream->data);
			if (self != NULL) {
				self->OnWrite_cb(self, status);
			}
			delete req;
		}


		// -----------------------------
		// class TCP
		// -----------------------------
		TCP::TCP() {
		}

		int TCP::init(Loop &loop) {
			this->Handle::init();
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_init(loop.context_ptr(), tcp);
		}

		int TCP::init_ex(Loop &loop, int flags) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_init_ex(loop.context_ptr(), tcp, flags);
		}

		int TCP::open(sock_t sock) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_open(tcp, sock);
		}

		int TCP::nodelay(int enable) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_nodelay(tcp, enable);
		}

		int TCP::keepalive(int enable, unsigned int delay) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_keepalive(tcp, enable, delay);
		}

		int TCP::simultaneous_accepts(int enable) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_simultaneous_accepts(tcp, enable);
		}

		int TCP::bind4(const char *ipv4, int port) {
			struct sockaddr_in addr;
			int r = uv_ip4_addr(ipv4, port, &addr);
			if (r < 0)
				return r;
			return bind((const sockaddr *)&addr, 0);
		}

		int TCP::bind6(const char *ipv6, int port) {
			struct sockaddr_in6 addr;
			int r = uv_ip6_addr(ipv6, port, &addr);
			if (r < 0)
				return r;
			return bind((const sockaddr *)&addr, 0);
		}

		int TCP::bind(const sockaddr *addr, unsigned int flags) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_bind(tcp, addr, flags);
		}

		int TCP::getsockname(struct sockaddr *name, int *namelen) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_getsockname(tcp, name, namelen);
		}

		int TCP::getpeername(struct sockaddr *name, int *namelen) {
			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();
			return uv_tcp_getpeername(tcp, name, namelen);
		}

		int TCP::connect4(const char *ipv4, int port) {
			struct sockaddr_in addr;
			int r = uv_ip4_addr(ipv4, port, &addr);
			if (r < 0)
				return r;
			return connect((const struct sockaddr *)&addr);
		}

		int TCP::connect6(const char *ipv6, int port) {
			struct sockaddr_in6 addr;
			int r = uv_ip6_addr(ipv6, port, &addr);
			if (r < 0)
				return r;
			return connect((const struct sockaddr *)&addr);
		}

		int TCP::connect(const struct sockaddr *addr) {
			uv_connect_t *req = new uv_connect_t;

			uv_tcp_t* tcp = (uv_tcp_t *)context_ptr();

			return uv_tcp_connect(req, tcp, addr, connect_cb);
		}

		void TCP::connect_cb(uv_connect_t *req, int status) {
			uv_stream_t *stream = req->handle;

			TCP* self = static_cast<TCP *>(stream->data);
			if (self != NULL) {
				self->OnConnect_cb(self, status);
			}

			delete req;
		}


		// -----------------------------
		// class UDP
		// -----------------------------
		UDP::UDP()
		{
		}

		int UDP::init(Loop &loop) {
			this->Handle::init();
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_init(loop.context_ptr(), udp);
		}

		int UDP::init_ex(Loop &loop, unsigned int flags) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_init_ex(loop.context_ptr(), udp, flags);
		}

		int UDP::open(sock_t sock) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_open(udp, sock);
		}

		int UDP::bind(const struct sockaddr *addr, unsigned int flags) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_bind(udp, addr, flags);
		}

		int UDP::getsockname(struct sockaddr *name, int *namelen) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_getsockname(udp, name, namelen);
		}

		int UDP::set_membership(const char *multicast_addr, const char *interface_addr, membership_t membership) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_membership(udp, multicast_addr, interface_addr, membership);
		}

		int UDP::set_multicast_loop(int on) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_multicast_loop(udp, on);
		}

		int UDP::set_multicast_ttl(int ttl) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_multicast_ttl(udp, ttl);
		}

		int UDP::set_multicast_interface(const char *interface_addr) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_multicast_interface(udp, interface_addr);
		}

		int UDP::set_broadcast(int on) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_broadcast(udp, on);
		}

		int UDP::set_ttl(int ttl) {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_set_ttl(udp, ttl);
		}

		int UDP::send(const char *buf, size_t length, const struct sockaddr *addr) {

			uv_buf_t sendbuf = uv_buf_init((char *)buf, length);
			uv_udp_send_t* req = new uv_udp_send_t;
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_send(req, udp, &sendbuf, 1, addr, send_cb);
		}

		int UDP::try_send(const char *buf, size_t length, const struct sockaddr *addr) {

			uv_buf_t sendbuf = uv_buf_init((char *)buf, length);
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_try_send(udp, &sendbuf, 1, addr);
		}

		int UDP::recv_start() {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_recv_start(udp, alloc_cb, recv_cb);
		}
		int UDP::recv_stop() {
			uv_udp_t* udp = (uv_udp_t *)context_ptr();
			return uv_udp_recv_stop(udp);
		}

		void UDP::send_cb(uv_udp_send_t *req, int status) {
			uv_udp_t* udp = req->handle;

			UDP* self = static_cast<UDP *>(udp->data);
			if (self != NULL) {
				self->OnSend_cb(self, status);
			}
			delete req;
		}

		void UDP::alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {

			UDP* self = static_cast<UDP *>(handle->data);
			if (self != NULL) {
				self->OnAlloc_cb(self, suggested_size, buf);
			}
		}

		void UDP::recv_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {

			UDP* self = static_cast<UDP *>(handle->data);
			if (self != NULL) {
				self->OnRead_cb(self, nread, buf, addr, flags);
			}
		}
	}
} // namespace ZQ::eloop
