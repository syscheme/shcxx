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
// Ident : $Id: UDPDuplex.h,v 1.2 2004/06/22 03:32:27 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define UDPDuplex, a two-way UDP connection
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/UDPDuplex.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// Revision 1.2  2004/06/22 03:32:27  shao
// exports UDPReceive in UDPSocket.h
//
// Revision 1.1  2004/06/22 02:27:12  shao
// slipped UDPDuplex into a new cpp/h file pair
//
// ===========================================================================

#ifndef __ZQ_COM_UDPDUPLEX_H__
#define __ZQ_COM_UDPDUPLEX_H__

#include "ZQ_common_conf.h"
#include "UDPSocket.h"

namespace ZQ {
namespace common {

class ZQ_COMMON_API UDPTransmit;
class ZQ_COMMON_API UDPReceive2;
class ZQ_COMMON_API UDPDuplex;

// -----------------------------
// class UDPTransmit
// -----------------------------
/// This class is ONLY used to build the UDP Duplex.
/// Representing half of a two-way UDP connection, the UDP transmitter can
/// broadcast data to another selected peer host or to an entire subnet.
class UDPTransmit : protected UDPSocket
{
private:

	/// Common code for diferent flavours of Connect (host, broadcast,
	/// multicast).
	/// @param ia network address to associate with
	/// @param port port number to associate with
	Error cConnect(const InetAddress &ia, tpport_t port);

protected:

	/// Create a UDP transmitter.
	UDPTransmit();

	/// Create a UDP transmitter, bind it to a specific interface and port
	/// address so that other UDP sockets may find and send UDP messages
	/// to it, and associate it with a given port on a peer host. On
	/// failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	/// @param port number on peer host to associate with.
	UDPTransmit(const InetAddress &bind, tpport_t port = 5005);

	/// Associate this socket with a specified peer host.  The port number
	/// from the constructor will be used.  All UDP packets will be sent to
	/// and received from the specified host.
	/// @return 0 on success, -1 on error.
	/// @param host address to connect socket to.
	Error connect(const InetHostAddress &host, tpport_t port);

	/// Associate this socket with a subnet of peer hosts for subnet
	/// broadcasting.  The server must be able to assert broadcast
	/// permission for the socket.
	/// @return 0 on success, -1 on error.
	/// @param subnet address to broadcast into.
	Error connect(const BroadcastAddress &subnet, tpport_t port);

	/// Associate this socket with a multicast group.
	/// @return 0 on success, -1 on error.
	/// @param mgroup address of the multicast group to send to.
	/// @param port port number
	Error connect(const InetMcastAddress &group, tpport_t port);

	/// Disassociate this socket from any host connection. No data should
	/// be read or written until a connection is established.
	Error disconnect(void);

	/// Overwrite UDPSocket's, use "connected" send rather than sendto.
	/// @return number of bytes sent.
	/// @param address of buffer to send.
	/// @param len of bytes to send.
	int send(const void *buf, int len);

	/// Stop transmitter.
	void endTransmitter(void);

	/// Get transmitter socket.
	/// @return transmitter.
	inline SOCKET getTransmitter(void)
		{return _so;};

	inline Error setMulticast(bool enable)
		{return Socket::setMulticast(enable);};

	inline Error setTimeToLive(unsigned char ttl)
		{return Socket::setTimeToLive(ttl);};

public:

	/// use "connected" send rather than sendto.
	/// Windows does not support MSG_DONTWAIT, so it is defined as 0
	/// on that platform.
	/// @return number of bytes sent.
	/// @param address of buffer to send.
	/// @param len of bytes to send.
	int transmit(const char *buffer, size_t len);

	/// See if output queue is empty for sending more packets.
	/// @return true if output available.
	/// @param timeout in milliseconds to wait.
	bool isOutputReady(unsigned long timeout = 0l);

	inline Error setRouting(bool enable)
		{return Socket::setRouting(enable);};

	inline Error setTypeOfService(Tos tos)
		{return Socket::setTypeOfService(tos);};

	inline Error setBroadcast(bool enable)
		{return Socket::setBroadcast(enable);};
};

// -----------------------------
// class UDPDuplex
// -----------------------------
/// UDP duplex connections impliment a bi-directional point-to-point UDP
/// session between two peer hosts.  Two UDP sockets are typically used
/// on alternating port addresses to assure that sender and receiver data
/// does not collide or echo back.  A UDP Duplex is commonly used for full
/// duplex real-time streaming of UDP data between hosts.
class UDPDuplex : public UDPTransmit, protected UDPReceive
{
public:

	/// Create a UDP duplex as a pair of UDP simplex objects bound to
	/// alternating and interconnected port addresses.
	/// @param bind address to bind this socket to.
	/// @param port number to bind sender.
	/// @param port number to bind reciever.
	UDPDuplex(const InetAddress &bind, tpport_t port);

	/// Associate the duplex with a specified peer host. Both the sender
	/// and receiver will be interconnected with the remote host.
	/// @return 0 on success, error code on error.
	/// @param host address to connect socket to.
	/// @param port number to connect socket to.
	Error connect(const InetHostAddress &host, tpport_t port);

	/// Disassociate this duplex from any host connection. No data should
	/// be read or written until a connection is established.
	/// @return 0 on success, error code on error.
	Error disconnect(void);

public:

	/// Receive a data packet from the connected peer host.
	/// @return num of bytes actually received.
	/// @param addr of data receive buffer.
	/// @param size of data receive buffer.
	int receive(void *buf, size_t len)
		{ return UDPReceive::receive(buf, len); }

	/// See if input queue has data packets available.
	/// @return true if data packets available.
	/// @param timeout in milliseconds.
	bool isInputReady(timeout_t timeout = TIMEOUT_INF)
		{ return UDPReceive::isInputReady(timeout); }
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COM_UDPDUPLEX_H__
