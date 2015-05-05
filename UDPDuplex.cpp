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
// Ident : $Id: UDPDuplex.cpp,v 1.2 2004/06/22 03:32:27 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl UDPDuplex, a two-way UDP connection
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/UDPDuplex.cpp $
// 
// 2     3/19/15 10:07a Hui.shao
// x64 compile warnings
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 2     08-03-06 16:36 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// Revision 1.2  2004/06/22 03:32:27  shao
// exports UDPReceive in UDPSocket.h
//
// Revision 1.1  2004/06/22 02:27:12  shao
// slipped UDPDuplex into a new cpp/h file pair
//
// ===========================================================================

#include "UDPDuplex.h"

namespace ZQ {
namespace common {

// -----------------------------
// class UDPTransmit
// -----------------------------

UDPTransmit::UDPTransmit() : UDPSocket()
{
	disconnect();
	shutdown(_so, 0);
}

UDPTransmit::UDPTransmit(const InetAddress &ia, tpport_t port)
            :UDPSocket(ia, port)
{
	disconnect();	// assure not started live
	shutdown(_so, 0);
}

Socket::Error UDPTransmit::cConnect(const InetAddress &ia, tpport_t port)
{
	memset(&peer, 0, sizeof(peer));
	int len = sizeof(peer.sa.a);
	sockaddr *soaddr = (sockaddr *) &peer.sa.a;

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
		soaddr = (sockaddr *) &peer.sa.a6;
	}
#endif // IPV6_ENABLED

	if(::connect(_so, soaddr, len))
		return connectError();

	return errSuccess;
}

Socket::Error UDPTransmit::connect(const InetHostAddress &ia, tpport_t port)
{
	if(isBroadcast())
		setBroadcast(false);

	return cConnect((InetAddress)ia,port);
}

Socket::Error UDPTransmit::connect(const BroadcastAddress &subnet, tpport_t  port)
{
	if(!isBroadcast())
		setBroadcast(true);

	return cConnect((InetAddress)subnet,port);
}

Socket::Error UDPTransmit::connect(const InetMcastAddress &group, tpport_t  port)
{
	Error error;
	if(!(error = setMulticast(true)))
		return error;

	return cConnect((InetAddress)group,port);
}

#ifdef	AF_UNSPEC

Socket::Error UDPTransmit::disconnect(void)
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
Socket::Error UDPTransmit::disconnect(void)
{
	return connect(getLocal());
}
#endif

int UDPTransmit::send(const void *buf, int len)
{
	return ::send(_so, (const char *)buf, len, 0);
}

void UDPTransmit::endTransmitter(void)
{
	Socket::endSocket();
}

int UDPTransmit::transmit(const char *buffer, size_t len)
{
	return ::send(_so, buffer, (int)len, MSG_DONTWAIT);
}

bool UDPTransmit::isOutputReady(unsigned long timeout)
{
	return Socket::isPending(Socket::pendingOutput, timeout);
}

// -----------------------------
// class UDPDuplex
// -----------------------------

UDPDuplex::UDPDuplex(const InetAddress &bind, tpport_t port)
          :UDPTransmit(bind, port + 1), UDPReceive(bind, port)
{
}

Socket::Error UDPDuplex::connect(const InetHostAddress &host, tpport_t port)
{
	Error rtn = UDPTransmit::connect(host, port);
	if(rtn)
	{
		UDPTransmit::disconnect();
		UDPReceive::disconnect();
		return rtn;
	}
	return UDPReceive::connect(host, port + 1);
}

Socket::Error UDPDuplex::disconnect(void)
{
	Error rtn = UDPTransmit::disconnect();
	Error rtn2 = UDPReceive::disconnect();
	if (rtn) return rtn;
	return rtn2;
}

} // namespace common
} // namespace ZQ
