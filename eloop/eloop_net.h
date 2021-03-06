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
// of the above copyright notice.  This software or any other copies thereof
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
// $Log: /ZQProjs/Common/eloop/eloop_net.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_Net_H__
#define __ZQ_COMMON_ELOOP_Net_H__

#include "ZQ_common_conf.h"
#include "eloop.h"

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API Stream;
class ZQ_ELOOP_API TCP;
class ZQ_ELOOP_API UDP;
class ZQ_ELOOP_API DNS;

#define RECV_BUF_SIZE (32*1024)
// -----------------------------
// class AbstractStream
// -----------------------------
class AbstractStream : public Handle
{
	friend class Process;
protected:
	AbstractStream(Loop& loop): Handle(loop), _byteSeen(0) { _recvBuf.base = NULL, _recvBuf.len=0; }
	//virtual void init() =0;

public:
	virtual ~AbstractStream();

	int shutdown();
	int listen();
	int accept(Handle* client);			//windows handle

	int read_start();
	int read_stop();
	int write(const eloop_buf_t bufs[],unsigned int nbufs,Handle *send_handle = NULL);
//	int write(const char *buf, size_t length);
	int write(const char *buf, size_t length,Handle *send_handle = NULL);
	int try_write(const char *buf, size_t length);

	int set_blocking(int blocking);
	int is_readable();
	int is_writeable();

protected:
	// called when the stream is shutdown
	virtual void OnShutdown(ElpeError status) { deactive(); }

	// tiggered when a new incomming connection is detected by listen()
	virtual void doAccept(ElpeError status) {}
	
	// called after buffer has been written into the stream
	virtual void OnWrote(int status) {}
	// called after buffer has been read from the stream
	virtual void OnRead(ssize_t nread, const char *buf) {} // TODO: uv_buf_t is unacceptable to appear here, must take a new manner known in this C++ wrapper level

	virtual void doAllocate(eloop_buf_t* buf, size_t suggested_size);

	virtual void doFree(eloop_buf_t* buf){}

	int write(const eloop_buf_t bufs[],unsigned int nbufs,uv_stream_t *send_handle);

private:
	static void _cbShutdown(uv_shutdown_t *req, int status);
	static void _cbConnection(uv_stream_t *stream, int status);
	static void _cbAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	static void _cbRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	static void _cbWrote(uv_write_t *req, int status);
//	static void _cbWrote2(uv_write_t *req, int status);

protected:
	eloop_buf_t	_recvBuf;
	int			_byteSeen;
};

// -----------------------------
// class TCP
// -----------------------------
class TCP : public AbstractStream
{
public:
	typedef uv_os_sock_t sock_t;

public:
	TCP(Loop& loop, int initFlags=0);
	int open(sock_t sock);
	int connected_open(sock_t sock);
	int nodelay(int enable);
	int keepalive(int enable, unsigned int delay);
	int simultaneous_accepts(int enable);
	int bind4(const char *ipv4, int port);
	int bind6(const char *ipv6, int port);
	int getsockname(struct sockaddr *name, int *namelen);
	int getpeername(struct sockaddr *name, int *namelen);
	void getlocaleIpPort(char* ip,int& port);
	void getpeerIpPort(char* ip,int& port);
	int connect4(const char *ipv4, int prot);
	int connect6(const char *ipv6, int port);

	int set_send_buf_size(int* value);
	int set_recv_buf_size(int* value);

protected:
	friend class ServiceSocket;

	// TODO: must enumerate all the status in the class
	virtual void OnConnected(ElpeError status) {}

private: // impl of Handle
	void init();

private:
	int connect(const struct sockaddr *addr);
	int bind(const sockaddr *addr, unsigned int flags);
	static void _cbConnect(uv_connect_t *req, int status);
	int _initFlags;

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
	UDP(Loop& loop, int initFlags=0);
	virtual ~UDP();

	int open(sock_t sock);
	int bind4(const char *ipv4, int port, unsigned int flags = Reuseaddr);
	int bind6(const char *ipv6, int port, unsigned int flags = Reuseaddr);

	int getsockname(struct sockaddr *name, int *namelen);
	void getlocaleIpPort(char* ip,int& port);
	int set_membership(const char *multicast_addr, const char *interface_addr, membership_t membership);
	int set_multicast_loop(int on);
	int set_multicast_ttl(int ttl);
	int set_multicast_interface(const char *interface_addr);
	int set_broadcast(int on);
	int set_ttl(int ttl);

	void get_ip4_name(const struct sockaddr_in* src, char* dst, size_t size);

	int set_send_buf_size(int* value);
	int set_recv_buf_size(int* value);


//	int connect4(const char *ipv4, int prot);
	int send4(const eloop_buf_t bufs[],unsigned int nbufs, const char *ipv4,int port);
	int send4(const char *buf, size_t length, const char *ipv4,int port);
	int send6(const char *buf, size_t length, const char *ipv6,int port);
	int send(const char *buf, size_t length,  const struct sockaddr *addr);

	int try_send4(const eloop_buf_t bufs[],unsigned int nbufs, const char *ipv4,int port);
	int try_send4(const char *buf, size_t length, const char *ipv4, int port);
	int try_send6(const char *buf, size_t length, const char *ipv6, int port);
	
	int recv_start();
	int recv_stop();

protected:
	// 
	virtual void OnSent(ElpeError status) {}
	virtual void doAllocate(eloop_buf_t* buf, size_t suggested_size);
	virtual void doFree(eloop_buf_t* buf);
	virtual void OnReceived(ssize_t nread, const char *buf, const struct sockaddr *addr, unsigned flags) {}

private: // impl of Handle
	void init();

private:
	eloop_buf_t   _buf;
	int _initFlags;

	int bind(const struct sockaddr *addr, unsigned int flags);
	int try_send(const char *buf, size_t length, const struct sockaddr *addr);

	static void _cbSent(uv_udp_send_t *req, int status);
	static void _cbAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
	static void _cbRecv(uv_udp_t *udp, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
};

// -----------------------------
// class DNS
// -----------------------------
class DNS
{
public:
	DNS() {}
	int getAddrInfo(Loop &loop,const char* node,const char* service,const struct addrinfo* hints);

protected:
	virtual void onResolved(Handle::ElpeError status,const char* ip) {}

private:
	static void _cbResolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res);
};

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_Net_H__
