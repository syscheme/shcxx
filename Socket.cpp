// ===========================================================================
// Copyright (c) 1997, 1998 by
// ZQ Interactive, Inc., Shanghai, PRC.,
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
// Ident : $Id: Socket.cpp,v 1.7 2004/07/29 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define Socket class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Socket.cpp $
// 
// 5     3/09/16 12:09p Hongquan.zhang
// 
// 4     5/30/12 5:31p Hui.shao
// 
// 4     5/30/12 5:30p Hui.shao
// 
// 5     5/30/12 5:11p Hui.shao
// fixed socket leak per HuangFei's 09-11-23 17:11 checkin
// 
// 3     3/27/12 2:09p Hui.shao
// 
// 2     1/17/11 12:21p Haoyuan.lu
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     09-11-23 17:11 Fei.huang
// * use shutdown to force return of some blocking system call
// 
// 7     09-05-11 18:35 Fei.huang
// 
// 6     08-03-06 16:32 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 5     08-03-03 18:13 Yixin.tian
// merged changes for linux
// 
// 4     06-09-07 18:02 Hui.shao
// 
// 3     3/23/06 11:06a Hui.shao
// 
// 2     04-10-09 18:11 Hui.shao
// Revision 1.7  2004/07/29 06:25:44  shao
// no message
//
// Revision 1.6  2004/07/28 09:31:24  shao
// IPv6 join mgroup
//
// Revision 1.5  2004/06/21 05:48:53  shao
// fixed unmatched inet addr field
//
// Revision 1.4  2004/06/17 04:07:40  jshen
// no message
//
// Revision 1.3  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.2  2004/05/11 08:38:03  shao
// no message
//
// Revision 1.1  2004/05/11 07:31:15  shao
// contributed to ZQ::common
//
// ===========================================================================

#include "Socket.h"

extern "C" {
#include <fcntl.h>
#include <errno.h>

#ifndef ZQ_OS_MSWIN
#  include <sys/socket.h>
#  include <unistd.h>
#  include <sys/types.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#else
#  include <io.h>
#endif //ZQ_OS_MSWIN
}

#ifdef	ZQ_OS_MSWIN
#  include <cerrno>
#  include <cstdlib>
#endif

#include <sstream>

#ifndef	ZQ_OS_MSWIN
# ifndef  O_NONBLOCK
#  define O_NONBLOCK	O_NDELAY
# endif
# ifdef	IPPROTO_IP
#  ifndef  SOL_IP
#   define SOL_IP	IPPROTO_IP
#  endif // !SOL_IP
# endif	 // IPPROTO_IP
#endif	 // !ZQ_OS_MSWIN

namespace ZQ {
namespace common {

#ifdef ZQ_OS_MSWIN
SOCKET dupSocket(SOCKET so, Socket::State state)
{
	if (state == Socket::stStream)
		return dup((int)so);

	HANDLE pidHandle = GetCurrentProcess();
	HANDLE dupHandle;
	if(DuplicateHandle(pidHandle, reinterpret_cast<HANDLE>(so), pidHandle, &dupHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return reinterpret_cast<SOCKET>(dupHandle);
	return INVALID_SOCKET;
}
#endif 

#if defined(ZQ_OS_MSWIN) // Winsock processing
// -----------------------------
// class init_WSA
// -----------------------------
// class init_WSA used to initalise windows sockets specfifc stuff : there is
// an MS - specific init sequence for Winsock 2 this class attempts to 
// initalise Winsock 2.2 - needed for non - blocking I/O. It will fall back 
// on 1.2 or lower if 2.0 or higher is not available,  but < 2.0 does not 
// support non - blocking I/O
// TO DO : might be an idea to have a method that reports version of Winsock in
// use or a predicate to test if non - blocking is OK
class init_WSA
{
public:
	init_WSA();
	~init_WSA();
};
#endif // ZQ_OS_MSWIN

// -----------------------------
// class Socket
// -----------------------------
Socket::Socket(int domain, int type, int protocol)
{
	setSocket();
	_so = socket(domain, type, protocol);
	if(_so == INVALID_SOCKET)
	{
		error(errCreateFailed);
		return;
	}
	_state = stAvailable;
}

Socket::Socket(SOCKET fd)
{
	setSocket();
	if (fd == INVALID_SOCKET)
	{
		error(errCreateFailed);
		return;
	}
	_so = fd;
	_state = stAvailable;
}

Socket::Socket(const Socket &orig)
{
	setSocket();
	_so = DUP_SOCK(orig._so, orig._state);
	if(_so == INVALID_SOCKET)
		error(errCopyFailed);
	_state = orig._state;
}

Socket::~Socket()
{
	endSocket();
}

void Socket::setSocket(void)
{
	flags.thrown    = false;
	flags.broadcast = false;
	flags.route     = true;
	flags.keepalive = false;
	flags.loopback  = true;
	flags.multicast = false;
	flags.linger	= false;
	flags.ttl	= 1;
	_errid           = errSuccess;
	_errstr          = NULL;
	_state           = stInitial;
	_so              = INVALID_SOCKET;
}

ssize_t Socket::readLine(char *str, size_t request, timeout_t timeout)
{
	bool crlf = false;
	bool nl = false;
	size_t nleft = request - 1; // leave also space for terminator
	int nstat,c;

	if(request < 1)
		return 0;

	str[0] = 0;

	while(nleft && !nl)
	{
		if(timeout)
		{
			if(!isPending(pendingInput, timeout))
			{
				error(errTimeout);
				return -1;
			}
		}
		nstat = ::recv(_so, str, (int)nleft, MSG_PEEK);
		if(nstat <= 0)
		{
			error(errInput);
			return -1;
		}

		// FIXME: if unique char in buffer is '\r' return "\r"
		//        if buffer end in \r try to read another char?
		//        and if timeout ??
		//        remember last \r

		for(c=0; c < nstat; ++c)
		{
			if(str[c] == '\n')
			{
				if (c > 0 && str[c-1] == '\r')
					crlf = true;
				++c;
				nl = true;
				break;
			}
		}
		nstat = ::recv(_so, str, c, 0);
		// TODO: correct ???
		if(nstat < 0)
			break;

		// adjust ending \r\n in \n
		if(crlf)
		{
			--nstat;
			str[nstat - 1] = '\n';
		}

		str += nstat;
		nleft -= nstat;
	}
	*str = 0;
	return (ssize_t)(request - nleft - 1);
}

bool Socket::isConnected(void) const
{
	return (Socket::_state == stConnected) ? true : false;
}

bool Socket::isActive(void) const
{
	return (_state != stInitial) ? true : false;
}

bool Socket::operator!() const
{
	return (Socket::_state == stInitial) ? true : false;
}

void Socket::endSocket(void)
{
	if (Socket::_state == stStream)
	{
#ifdef	ZQ_OS_MSWIN
		if(_so != INVALID_SOCKET)
			closesocket(_so);
#else
		if(_so > INVALID_SOCKET)
			close(_so);
#endif
		_so = INVALID_SOCKET;
		_state = stInitial;
		return;
	}

	_state = stInitial;
	if (INVALID_SOCKET == _so)
		return;

#ifdef	SO_LINGER
	struct linger linger;

	if (flags.linger)
	{
		linger.l_onoff = 1;
		linger.l_linger = 60;
	}
	else
		linger.l_onoff = linger.l_linger = 0;

	setsockopt(_so, SOL_SOCKET, SO_LINGER, (char *)&linger,	(socklen_t)sizeof(linger));
#endif

//	shutdown(_so, 2);
#ifdef ZQ_OS_MSWIN
	closesocket(_so);
#else
    // stop a blocking call
	shutdown(_so, SHUT_RDWR);
	close(_so);
#endif

	_so = INVALID_SOCKET;
}

#ifdef ZQ_OS_MSWIN
Socket::Error Socket::connectError(void)
{
	switch(WSAGetLastError())
	{
	case WSAENETDOWN:
		return error(errResourceFailure);
	case WSAEWOULDBLOCK://fall through
	case WSAEINPROGRESS:
		return error(errConnectBusy);
	case WSAEADDRNOTAVAIL:
		return error(errConnectInvalid);
	case WSAECONNREFUSED:
		return error(errConnectRefused);
	case WSAENETUNREACH:
		return error(errConnectNoRoute);
	default:
		return error(errConnectFailed);
	}
}
#else
Socket::Error Socket::connectError(void)
{
	switch(errno)
	{
#ifdef	EHOSTUNREACH
	case EHOSTUNREACH:
		return error(errConnectNoRoute);
#endif
#ifdef	ENETUNREACH
	case ENETUNREACH:
		return error(errConnectNoRoute);
#endif
	case EINPROGRESS:
		return error(errConnectBusy);
#ifdef	EADDRNOTAVAIL
	case EADDRNOTAVAIL:
		return error(errConnectInvalid);
#endif
	case ECONNREFUSED:
		return error(errConnectRefused);
	case ETIMEDOUT:
		return error(errConnectTimeout);
	default:
		return error(errConnectFailed);
	}
}
#endif

Socket::Error Socket::error(const Error err, const char *errs, const uint16 errnum) const
{
	_errid  = err;
	_errstr = errs;
	_errnum = errnum;

	if(!err)
		return err;

	if(flags.thrown)
		return err;

	// prevents recursive throws

	flags.thrown = true;

	return err;
}


void Socket::error(char *estr)
{
	error(errExtended, estr);
}


void Socket::setError(bool enable)
{
	flags.thrown = !enable;
}

Socket::Error Socket::setBroadcast(bool enable)
{
	int opt = (enable ? 1 : 0);
	if(setsockopt(_so, SOL_SOCKET, SO_BROADCAST,(char *)&opt, (socklen_t)sizeof(opt)))
		return error(errBroadcastDenied);

	flags.broadcast = enable;
	return errSuccess;
}

Socket::Error Socket::setKeepAlive(bool enable)
{
	int opt = (enable ? ~0: 0);
#if (defined(SO_KEEPALIVE) || defined(ZQ_OS_MSWIN))
	if(setsockopt(_so, SOL_SOCKET, SO_KEEPALIVE,
		      (char *)&opt, (socklen_t)sizeof(opt)))
		return error(errKeepaliveDenied);
#endif
	flags.keepalive = enable;
	return errSuccess;
}

Socket::Error Socket::setLinger(bool linger)
{
#ifdef	SO_LINGER
	flags.linger = linger;
	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}

Socket::Error Socket::setRouting(bool enable)
{
	int opt = (enable ? 1 : 0);

#ifdef	SO_DONTROUTE
	if(setsockopt(_so, SOL_SOCKET, SO_DONTROUTE,
		      (char *)&opt, (socklen_t)sizeof(opt)))
		return error(errRoutingDenied);
#endif
	flags.route = enable;
	return errSuccess;
}

Socket::Error Socket::setNoDelay(bool enable)
{
	int opt = (enable ? 1 : 0);

	if(setsockopt(_so, IPPROTO_TCP, TCP_NODELAY,
		      (char *)&opt, (socklen_t)sizeof(opt)))
		return error(errNoDelay);

	return errSuccess;
}

Socket::Error Socket::setTypeOfService(Tos service)
{
#ifdef	SOL_IP
	unsigned char tos;
	switch(service)
	{
#ifdef 	IPTOS_LOWDELAY
	case tosLowDelay:
		tos = IPTOS_LOWDELAY;
		break;
#endif
#ifdef 	IPTOS_THROUGHPUT
	case tosThroughput:
		tos = IPTOS_THROUGHPUT;
		break;
#endif
#ifdef	IPTOS_RELIABILITY
	case tosReliability:
		tos = IPTOS_RELIABILITY;
		break;
#endif
#ifdef	IPTOS_MINCOST
	case tosMinCost:
		tos = IPTOS_MINCOST;
		break;
#endif
	default:
		return error(errServiceUnavailable);
	}
	if(setsockopt(_so, SOL_IP, IP_TOS,(char *)&tos, (socklen_t)sizeof(tos)))
		return error(errServiceDenied);
	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}

Socket::Error Socket::setTimeToLive(unsigned char ttl)
{
#ifdef	IP_MULTICAST_TTL
	if(!flags.multicast)
		return error(errMulticastDisabled);

	flags.ttl = ttl;
	setsockopt(_so, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl));
	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}

Socket::Error Socket::setLoopback(bool enable)
{
#ifdef	IP_MULTICAST_LOOP
	unsigned char loop;

	if(!flags.multicast)
		return error(errMulticastDisabled);

	if(enable)
		loop = 1;
	else
		loop = 0;
	flags.loopback = enable;
	setsockopt(_so, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop));
	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}

bool Socket::isPending(Pending pending, timeout_t timeout)
{
	int status;
	struct timeval tv;
	struct timeval *tvp = &tv;
	fd_set grp;

	if(timeout == TIMEOUT_INF)
		tvp = NULL;
	else
	{
		tv.tv_usec = (timeout % 1000) * 1000;
		tv.tv_sec = timeout / 1000;
	}

	FD_ZERO(&grp);
	FD_SET(_so, &grp);
	switch(pending)
	{
	case pendingInput:
		status = select((int)_so + 1, &grp, NULL, NULL, tvp);
		break;
	case pendingOutput:
		status = select((int)_so + 1, NULL, &grp, NULL, tvp);
		break;
	case pendingError:
		status = select((int)_so + 1, NULL, NULL, &grp, tvp);
		break;
	}
	if(status < 1)
		return false;
	if(FD_ISSET(_so, &grp))
		return true;
	return false;
}

Socket &Socket::operator=(const Socket &from)
{
	if(_so == from._so)
		return *this;

	if(_state != stInitial)
		endSocket();

	_so = DUP_SOCK(from._so,from._state);
	if(_so == INVALID_SOCKET)
	{
		error(errCopyFailed);
		_state = stInitial;
	}
	else
		_state = from._state;

	return *this;
}

void Socket::setCompletion(bool blockIO)
{
	//if (flags.completion == immediate)
	//	return;

#ifdef ZQ_OS_MSWIN
	// note that this will not work on some versions of Windows for Workgroups. Tough. -- jfc
	unsigned long mode= (blockIO ? 0:1);
	ioctlsocket(_so, FIONBIO, &mode);
#else
	int fflags = fcntl(_so, F_GETFL);

	if (blockIO)
		fflags &=~ O_NONBLOCK;
	else
		fflags |= O_NONBLOCK;

	fcntl(_so, F_SETFL, fflags);
#endif

	flags.blockIO = blockIO ?1:0;
}

InetHostAddress Socket::getSender(tpport_t *port) const
{
	struct sockaddr_in from;
	char buf;
	socklen_t len = sizeof(from);
	int rc = ::recvfrom(_so, &buf, 1, MSG_PEEK,
			    (struct sockaddr *)&from, &len);

	if(rc == 1)
	{
		if(port)
			*port = ntohs(from.sin_port);
	}
	else
	{
		if(port)
			*port = 0;
		memset(&from.sin_addr, 0, sizeof(from.sin_addr));
	}
	if(rc < 0)
	{
#ifdef ZQ_OS_MSWIN
		error(errInput);
#else
		switch(errno)
		{
		case EINTR:
			error(errInputInterrupt);
			break;
		default:
			error(errInput);
		}
#endif
	}

	return InetHostAddress(from.sin_addr);
}

InetHostAddress Socket::getLocal(tpport_t *port) const
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	if(getsockname(_so, (struct sockaddr *)&addr, &len))
	{
		error(errResourceFailure);
	      	if(port)
			*port = 0;
		memset(&addr.sin_addr, 0, sizeof(addr.sin_addr));
	}
	else
	{
		if(port)
		    	*port = ntohs(addr.sin_port);
	}
	return InetHostAddress(addr.sin_addr);
}

InetHostAddress Socket::getPeer(tpport_t *port) const
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	if(getpeername(_so, (struct sockaddr *)&addr, &len))
	{
#ifndef ZQ_OS_MSWIN
		if(errno == ENOTCONN)
			error(errNotConnected);
		else
#endif
			error(errResourceFailure);
		if(port)
			*port = 0;
		memset(&addr.sin_addr, 0, sizeof(addr.sin_addr));
	}
	else
	{
		if(port)
	    		*port = ntohs(addr.sin_port);
	}
	return InetHostAddress(addr.sin_addr);
}

Socket::Error Socket::setMulticast(bool enable)
{
#ifdef	IP_MULTICAST_IF

#ifdef IPV6_ENABLED
	struct sockaddr_in6 addr;
#else
	struct sockaddr_in addr;
#endif

	memset(&addr, 0, sizeof(addr));
	socklen_t len = sizeof(addr);
	struct sockaddr_in* paddr4 = (struct sockaddr_in*) &addr;

	if(enable == flags.multicast)
		return errSuccess;

	flags.multicast = enable;
	getsockname(_so, (struct sockaddr *)&addr, &len);

	int level = IPPROTO_IP;
	int opt = IP_MULTICAST_IF;
	char* psin_addr = (char*) &(paddr4->sin_addr);
	int   sin_len = sizeof(paddr4->sin_addr);

#ifdef IPV6_ENABLED
	if (addr.sin6_family == PF_INET6)
	{
		level = IPPROTO_IPV6;
		opt = IPV6_MULTICAST_IF;
		psin_addr = (char*) &(addr.sin6_addr);
		sin_len = sizeof(addr.sin6_addr);
	}
#endif

	if(!enable)
		memset(psin_addr, 0, sin_len);

	setsockopt(_so, level, opt, psin_addr, sin_len);

#ifdef IPV6_ENABLED
	int hops = 255;
	setsockopt(_so, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&hops, sizeof(hops));
#endif

	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}

Socket::Error Socket::join(const InetMcastAddress &ia)
{
#ifdef	IP_ADD_MEMBERSHIP

#ifdef IPV6_ENABLED
	struct sockaddr_in6 myaddr;
#else
	struct sockaddr_in myaddr;
#endif

	socklen_t len = sizeof(myaddr);
	memset(&myaddr, 0, len);
	struct sockaddr_in* paddr4 = (struct sockaddr_in*) &myaddr;

	if(!flags.multicast)
		return error(errMulticastDisabled);

	getsockname(_so, (struct sockaddr *)&myaddr, &len);

#ifdef IPV6_ENABLED
	if (myaddr.sin6_family == PF_INET6)
	{
		struct ipv6_mreq group;
		memcpy(&group.ipv6mr_interface, &(myaddr.sin6_addr), sizeof(group.ipv6mr_interface));
		group.ipv6mr_multiaddr = getaddress(ia).addr.a6;

		setsockopt(_so, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
		return errSuccess;
	} else
#endif
	{
		struct ip_mreq group;
		memcpy(&group.imr_interface, &(paddr4->sin_addr), sizeof(group.imr_interface));
		group.imr_multiaddr = getaddress(ia).addr.a;
		setsockopt(_so, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
		return errSuccess;
	}
#else
	return error(errServiceUnavailable);
#endif
}

Socket::Error Socket::drop(const InetMcastAddress &ia)
{
#ifdef	IP_DROP_MEMBERSHIP
	struct ip_mreq group;
	struct sockaddr_in myaddr;
	socklen_t len = sizeof(myaddr);

	if(!flags.multicast)
		return error(errMulticastDisabled);

	getsockname(_so, (struct sockaddr *)&myaddr, &len);
	memcpy(&group.imr_interface, &myaddr.sin_addr, sizeof(&myaddr.sin_addr));
	group.imr_multiaddr = getaddress(ia).addr.a;
	setsockopt(_so, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group));
	return errSuccess;
#else
	return error(errServiceUnavailable);
#endif
}


#ifdef ZQ_OS_MSWIN

// -----------------------------
// class init_WSA
// -----------------------------
init_WSA::init_WSA()
{
	//-initialize OS socket resources!
	WSADATA	wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		abort();
	}
};

init_WSA::~init_WSA() 
{ 
	WSACleanup(); 
} 

/// a static instance to init winsock
static init_WSA init_wsa;

#endif

} // namespace common
} // namespace ZQ
