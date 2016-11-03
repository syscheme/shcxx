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
// of the above copyright notice.  This software or any other copies thereof
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
// $Log: /ZQProjs/Common/eloop/eloop_net.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_Net_H__
#define __ZQ_COMMON_ELOOP_Net_H__

#include "eloop.h"
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API Stream;
class ZQ_ELOOP_API TCP;
class ZQ_ELOOP_API UDP;

// -----------------------------
// class Stream
// -----------------------------
class Stream : public Handle
{

protected:
	Stream();

public:
	int shutdown();
	int listen();
	int accept(Stream &client);
	int read_start();
	int read_stop();
	int write(const char *buf, size_t length);
	int write(const char *buf, size_t length, Stream *send_handle);
	int try_write(const char *buf, size_t length);

	int set_blocking(int blocking);
	int is_readable();
	int is_writeable();

protected:
	virtual void* OnShutdown_cb(Stream *self, int status) {}
	virtual void* OnConnection_cb(Stream *self, int status) {}
	virtual void* OnWrite_cb(Stream *self, int status) {}
	virtual void* OnRead_cb(Stream *self, ssize_t nread, const uv_buf_t *buf) {}
	virtual void* OnAlloc_cb(Stream *self, size_t suggested_size, uv_buf_t *buf) {}

private:
	static void shutdown_cb(uv_shutdown_t *req, int status);
	static void connection_cb(uv_stream_t *stream, int status);
	static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	static void write_cb(uv_write_t *req, int status);
};

// -----------------------------
// class TCP
// -----------------------------
class TCP : public Stream
{
public:
	typedef uv_os_sock_t sock_t;

public:
	TCP();
	int init(Loop &loop);
	int init_ex(Loop &loop, int flags);
	int open(sock_t sock);
	int nodelay(int enable);
	int keepalive(int enable, unsigned int delay);
	int simultaneous_accepts(int enable);
	int bind4(const char *ipv4, int port);
	int bind6(const char *ipv6, int port);
	int getsockname(struct sockaddr *name, int *namelen);
	int getpeername(struct sockaddr *name, int *namelen);
	int connect4(const char *ipv4, int prot);
	int connect6(const char *ipv6, int port);


protected:
	virtual void OnConnect_cb(TCP *self, int status) {}

private:
	int connect(const struct sockaddr *addr);
	int bind(const sockaddr *addr, unsigned int flags);
	static void connect_cb(uv_connect_t *req, int status);

};


// -----------------------------
// class UDP
// -----------------------------
class UDP : public Handle
{
public:
	enum udp_flags {
		Ipv6Only = UV_UDP_IPV6ONLY,
		Partial = UV_UDP_PARTIAL,
		Reuseaddr = UV_UDP_REUSEADDR
	};

	typedef uv_os_sock_t sock_t;
	typedef uv_membership membership_t;

public:
	UDP();
	int init(Loop &loop);
	int init_ex(Loop &loop, unsigned int flags);
	int open(sock_t sock);
	int bind(const struct sockaddr *addr, unsigned int flags);
	int getsockname(struct sockaddr *name, int *namelen);
	int set_membership(const char *multicast_addr, const char *interface_addr, membership_t membership);
	int set_multicast_loop(int on);
	int set_multicast_ttl(int ttl);
	int set_multicast_interface(const char *interface_addr);
	int set_broadcast(int on);
	int set_ttl(int ttl);
	int send(const char *buf, size_t length, const struct sockaddr *addr);
	int try_send(const char *buf, size_t length, const struct sockaddr *addr);
	int recv_start();
	int recv_stop();

protected:
	virtual void OnSend_cb(UDP *self, int status) {}
	virtual void OnAlloc_cb(UDP *self, size_t suggested_size, uv_buf_t *buf) {}
	virtual void OnRead_cb(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {}

private:
	static void send_cb(uv_udp_send_t *req, int status);
	static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	static void recv_cb(uv_udp_t *udp, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
};

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_Net_H__
