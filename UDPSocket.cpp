// ===========================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai,,
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
// Ident : $Id: UDPSocket.cpp,v 1.10 2004/07/21 12:05:52 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define UDPSocket classes
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/UDPSocket.cpp $
// 
// 5     3/19/15 10:07a Hui.shao
// x64 compile warnings
// 
// 4     7/26/12 6:20p Li.huang
// 
// 3     7/26/12 5:14p Hui.shao
// UDPSocket to export bind()
// 
// 2     2/15/12 11:30a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     09-08-13 16:38 Yixin.tian
// 
// 7     08-10-08 10:12 Yixin.tian
// 
// 6     08-03-06 16:36 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 5     08-03-03 18:16 Yixin.tian
// merged changes for linux
// 
// 4     06-09-07 18:02 Hui.shao
// 
// 3     04-10-10 16:53 Hui.shao
// Revision 1.10  2004/07/21 12:05:52  shao
// swap the port value
//
// Revision 1.9  2004/07/07 08:04:48  shao
// impl recvFrom()
//
// Revision 1.8  2004/06/22 03:32:27  shao
// exports UDPReceive in UDPSocket.h
//
// Revision 1.7  2004/06/22 02:27:12  shao
// slipped UDPDuplex into a new cpp/h file pair
//
// Revision 1.6  2004/06/21 10:40:54  shao
// added class UDPMulticast
//
// Revision 1.5  2004/06/21 04:14:16  shao
// enabled IPv6 for UDPSocket/UDPBroadcast
//
// Revision 1.2  2004/05/13 02:02:27  shao
// contributed to ZQ::common
//
// Revision 1.1  2004/05/13 01:34:23  shao
// no message
//
// ===========================================================================

#include "UDPSocket.h"

#undef _WSAERRMSG
namespace ZQ {
namespace common {

// -----------------------------
// class UDPSocket
// -----------------------------

UDPSocket::UDPSocket(void)
          :Socket(PF_INET, SOCK_DGRAM, 0)
{
	memset(&peer, 0, sizeof(peer));
	peer.family = peer.sa.a.sin_family = PF_INET;
}

#ifdef _WSAERRMSG
LPSTR DecodeError(int ErrorCode)
{
    static char Message[1024];

    // If this program was multi-threaded, we'd want to use
    // FORMAT_MESSAGE_ALLOCATE_BUFFER instead of a static buffer here.
    // (And of course, free the buffer when we were done with it)

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, ErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)Message, 1024, NULL);
    return Message;
}
#endif // _WSAERRMSG

UDPSocket::UDPSocket(const InetAddress &localAddr, tpport_t port)
          :Socket(localAddr.family(), SOCK_DGRAM, 0)
{
	bind(localAddr, port);
}

void UDPSocket::bind(const InetAddress &localAddr, tpport_t port)
{
	if (_state > stBound)
		return;

	setPeer(localAddr, port);

#ifdef	SOCK_REUSEADDR
	int opt = 1;
	setsockopt(_so, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, (socklen_t)sizeof(opt));
#endif

	if (peer.family == PF_INET)
	{
		int ret = ::bind(_so, (struct sockaddr *)&(peer.sa.a), sizeof(peer.sa.a));
		if (ret !=0)
		{
			endSocket();
			error(errBindingFailed);
			return;
		}
	}

#ifdef IPV6_ENABLED
	if(peer.family == PF_INET6)
	{
		int ret = ::bind(_so, (struct sockaddr *)&(peer.sa.a6), sizeof(peer.sa.a6));
#ifdef ZQ_OS_MSWIN
		if (ret == SOCKET_ERROR)
		{
#ifdef _WSAERRMSG
			char* err=DecodeError(WSAGetLastError());
#endif //_WSAERRMSG
			endSocket();
			error(errBindingFailed);
			return;
		}
#else
		if (ret == -1)//has error
		{
			endSocket();
			error(errBindingFailed);
			return;
		}
#endif
	}
#endif // IPV6_ENABLED

	_state = stBound;
}

UDPSocket::~UDPSocket()
{
	endSocket();
}

Socket::Error UDPSocket::setKeepAlive(bool enable)
{
	return Socket::setKeepAlive(enable);
}

void UDPSocket::setPeer(const InetAddress &ia, tpport_t port)
{
	memset(&peer, 0, sizeof(peer));
	InetAddress::inetaddr_t addr = getaddress(ia); 

	peer.family = addr.family;
    if (peer.family == PF_INET)
	{
		peer.sa.a.sin_family = peer.family;
		peer.sa.a.sin_addr = addr.addr.a;
		peer.sa.a.sin_port = htons(port);

		return;
	}

#ifdef IPV6_ENABLED
    if (peer.family == PF_INET6)
	{
		peer.sa.a6.sin6_family = peer.family;
		peer.sa.a6.sin6_addr = addr.addr.a6;
		peer.sa.a6.sin6_port = htons(port);
		peer.sa.a6.sin6_scope_id = addr.scopeid;

		return;
	}
#endif // IPV6_ENABLED
}

InetHostAddress UDPSocket::getPeer(tpport_t *port)
{
	// FIXME: insufficient buffer
	//        how to retrieve peer ??
	char buf;
	Socket::saddr_t source;
	socklen_t len = sizeof(source.sa);
	struct sockaddr *soaddr = (struct sockaddr *)&(source.sa);
	const static InetHostAddress dummyAddr;

	int rtn = ::recvfrom(_so, &buf, 1, MSG_PEEK, soaddr, &len);

#ifdef ZQ_OS_MSWIN
	if(rtn < 1 && WSAGetLastError() != WSAEMSGSIZE)
	{
		if(port)
			*port = 0;

		memset((void*) &peer, 0, sizeof(peer));

		return dummyAddr;
	}
#else
	if(rtn < 1)
	{
		if(port)
			*port = 0;

		memset((void*) &peer, 0, sizeof(peer));
		return dummyAddr;
	}
#endif

	if (source.sa.a.sin_family == PF_INET)
	{
		peer.family = source.sa.a.sin_family;
		memcpy(&(peer.sa.a), &(source.sa.a), sizeof(peer.sa.a));
		if(port)
			*port = ntohs(peer.sa.a.sin_port);

		return InetHostAddress(peer.sa.a.sin_addr);
	}

#ifdef IPV6_ENABLED
	if (source.sa.a6.sin6_family == PF_INET6)
	{
		peer.family = source.sa.a6.sin6_family;
		memcpy(&(peer.sa.a6), &(source.sa.a6), sizeof(peer.sa.a6));
		if(port)
			*port = ntohs(peer.sa.a6.sin6_port);
		return InetHostAddress(peer.sa.a6.sin6_addr);
	}
#endif // IPV6_ENABLED

	return dummyAddr;
}

int UDPSocket::send(const void *buf, size_t len)
{
	struct sockaddr * saddr = (struct sockaddr *) &(peer.sa.a);
	socklen_t slen = sizeof(peer.sa.a);

#ifdef IPV6_ENABLED
	if (peer.family == PF_INET6)
	{
		saddr = (struct sockaddr *) &(peer.sa.a6);
		slen = sizeof(peer.sa.a6);
	}
#endif // IPV6_ENABLED

	int ret = ::sendto(_so, (const char*)buf, (int)len, 0, saddr, slen);
#ifdef ZQ_OS_MSWIN
	if (ret == SOCKET_ERROR)
	{
#ifdef _WSAERRMSG
			char* err=DecodeError(WSAGetLastError());
#endif //_WSAERRMSG
	}
#else
	if(ret == -1)//has error
	{
#ifdef _DEBUG
		perror("UDPSocket::send");
#endif
	}
#endif
	return ret;
}

int UDPSocket::sendto(const void *buf, size_t len, const InetHostAddress& peerAddr, int peerPort)
{
	InetAddress::inetaddr_t peeraddr = peerAddr.getAddress();

	saddr_t tosaddr;
	memset(&tosaddr, 0, sizeof(tosaddr));
	tosaddr.family = tosaddr.sa.a.sin_family = peeraddr.family;
	tosaddr.sa.a.sin_addr = peeraddr.addr.a;
	tosaddr.sa.a.sin_port = htons(peerPort);

	struct sockaddr* saddr = (struct sockaddr *) &(tosaddr.sa.a);
	socklen_t slen = sizeof(tosaddr.sa.a);
#ifdef IPV6_ENABLED
	if (tosaddr.family == PF_INET6)
	{
		tosaddr.sa.a6.sin6_family = peeraddr.family;
		tosaddr.sa.a6.sin6_addr = peeraddr.addr.a6;
		tosaddr.sa.a6.sin6_port = htons(peerPort);
		saddr = (struct sockaddr *) &(tosaddr.sa.a6);
		slen = sizeof(peer.sa.a6);
	}
#endif // IPV6_ENABLED

	int ret = ::sendto(_so, (const char*)buf, (int)len, 0, saddr, slen);
#ifdef ZQ_OS_MSWIN
	if (ret == SOCKET_ERROR)
	{
#ifdef _WSAERRMSG
			char* err=DecodeError(WSAGetLastError());
#endif //_WSAERRMSG
	}
#else
	if(ret == -1)//has error
	{
#ifdef _DEBUG
		perror("UDPSocket::send");
#endif
	}
#endif
	return ret;
}

int UDPSocket::receive(void *buf, size_t len)
{
	return ::recv(_so, (char *)buf, (int)len, 0);
}

int UDPSocket::receiveFrom(void *buf, size_t len, InetHostAddress& peerAddr, int& peerPort)
{
	if (buf == NULL)
		return 0;

	saddr_t usa;
	memset (&usa, 0, sizeof(usa));

#ifdef ZQ_OS_MSWIN
	int salen = sizeof(usa.sa);
	int ret = ::recvfrom(_so, (char*)buf, (int)len, 0, (sockaddr*) &(usa.sa), &salen);
	if (SOCKET_ERROR == ret)
		return ret;
#else
	socklen_t slen = sizeof(usa.sa);
	int ret = ::recvfrom(_so, (char*)buf, len,0, (sockaddr*) &(usa.sa), &slen);
	if (ret < 0)
		return ret;
#endif
	if (usa.sa.a.sin_family == PF_INET)
	{
		peerPort = usa.sa.a.sin_port;
		peerAddr = usa.sa.a.sin_addr;
	}

#ifdef IPV6_ENABLED
	if (usa.sa.a.sin_family == PF_INET6)
	{
		peerPort = usa.sa.a6.sin6_port;
		peerAddr = usa.sa.a6.sin6_addr;
	}
#endif // IPV6_ENABLED

	// swap the byte order
//	peerPort = (peerPort >>8) + ((peerPort & 0xff) <<8);
	peerPort = ntohs(peerPort);
	return ret;
}


int UDPSocket::peek(void *buf, size_t len)
{
	return ::recv(_so, (char *)buf, (int)len, MSG_PEEK);
}


// -----------------------------
// class UDPBroadcast
// -----------------------------

UDPBroadcast::UDPBroadcast(const InetAddress &bind, tpport_t port)
             :UDPSocket(bind, port)
{
	if(_so != INVALID_SOCKET)
		setBroadcast(true);
}

void UDPBroadcast::setPeer(const BroadcastAddress &ia, tpport_t port)
{
	memset(&peer, 0, sizeof(peer));
	InetAddress::inetaddr_t addr = getaddress(ia); 

	peer.family = addr.family;
    if (peer.family == PF_INET)
	{
		peer.sa.a.sin_family = peer.family;
		peer.sa.a.sin_addr = addr.addr.a;
		peer.sa.a.sin_port = htons(port);

		return;
	}

#ifdef IPV6_ENABLED
    if (peer.family == PF_INET6)
	{
		peer.sa.a6.sin6_family = peer.family;
		peer.sa.a6.sin6_addr = addr.addr.a6;
		peer.sa.a6.sin6_port = htons(port);

		return;
	}
#endif // IPV6_ENABLED
}

// -----------------------------
// class UDPMulticast
// -----------------------------
UDPMulticast::UDPMulticast()
{
	if(_so != INVALID_SOCKET)
		setMulticast(true);
}

UDPMulticast::UDPMulticast(const InetAddress &bind, tpport_t port)
             :UDPSocket(bind, port)
{
	if(_so != INVALID_SOCKET)
		setMulticast(true);
}

void UDPMulticast::setGroup(const InetMcastAddress &group, tpport_t port)
{
	memset(&peer, 0, sizeof(peer));
	InetAddress::inetaddr_t addr = getaddress(group); 

	peer.family = addr.family;
    if (peer.family == PF_INET)
	{
		peer.sa.a.sin_family = peer.family;
		peer.sa.a.sin_addr = addr.addr.a;
		peer.sa.a.sin_port = htons(port);

		join(group);

		return;
	}

#ifdef IPV6_ENABLED
    if (peer.family == PF_INET6)
	{
		peer.sa.a6.sin6_family = peer.family;
		peer.sa.a6.sin6_addr = addr.addr.a6;
		peer.sa.a6.sin6_port = htons(port);

		join(group);

		return;
	}
#endif // IPV6_ENABLED
}

// -----------------------------
// class UDPReceive
// -----------------------------
UDPReceive::UDPReceive()
{
}

UDPReceive::UDPReceive(const InetAddress &ia, tpport_t port)
           :UDPSocket(ia, port)
{
//	shutdown(_so, 1);
}

Socket::Error UDPReceive::connect(const InetHostAddress &ia, tpport_t port)
{
	memset(&peer, 0, sizeof(peer));
	int len = sizeof(peer.sa.a);
//	sockaddr *soaddr = (sockaddr *) &peer.sa.a;

	InetAddress::inetaddr_t addr = getaddress(ia); 

	peer.family = addr.family;
    if (peer.family == PF_INET)
	{
		peer.sa.a.sin_family = peer.family;
		peer.sa.a.sin_addr = addr.addr.a;
		peer.sa.a.sin_port = htons(port);
	}

#ifdef IPV6_ENABLED
    if (peer.family == PF_INET6)
	{
		peer.sa.a6.sin6_family = peer.family;
		peer.sa.a6.sin6_addr = addr.addr.a6;
		peer.sa.a6.sin6_port = htons(port);

		// switch the len and soaddr
		len = sizeof(peer.sa.a6);
//		soaddr = (sockaddr *) &peer.sa.a6;
	}
#endif // IPV6_ENABLED

	if(::connect(_so, (sockaddr *)&peer, len))
		return connectError();
	return errSuccess;
}

// TODO: FIXME same implementation as UDPTrasmit::Disconnect
#ifdef	AF_UNSPEC

Socket::Error UDPReceive::disconnect(void)
{
	struct sockaddr_in addr;
	int len = sizeof(addr);

	memset(&addr, 0, len);
#ifndef ZQ_OS_MSWIN
	addr.sin_family = AF_UNSPEC;
#else
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = INADDR_NONE;
#endif
	if(::connect(_so, (sockaddr *)&addr, len))
		return connectError();
	return errSuccess;
}
#else
Socket::Error UDPReceive::disconnect(void)
{
	return connect(getLocal());
}
#endif

bool UDPReceive::isPendingReceive(timeout_t timeout)
{
	return Socket::isPending(Socket::pendingInput, timeout);
}

int UDPReceive::receive(void *buf, size_t len)
{
	return ::recv(_so, (char *)buf, (int)len, 0);
}

bool UDPReceive::isInputReady(timeout_t timeout)
{
	return Socket::isPending(Socket::pendingInput, timeout);
}

} // namespace common
} // namespace ZQ
