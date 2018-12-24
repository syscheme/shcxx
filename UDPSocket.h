// ===========================================================================
// Copyright (c) 1997, 1998 by
// syscheme, Shanghai
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
// Ident : $Id: UDPSocket.h,v 1.9 2004/07/07 08:04:48 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define UDPSocket classes
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/UDPSocket.h $
// 
// 4     7/26/12 6:20p Li.huang
// 
// 3     7/26/12 5:14p Hui.shao
// UDPSocket to export bind()
// 
// 2     2/15/12 11:27a Hui.shao
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
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
// Revision 1.5  2004/06/21 04:05:32  shao
// enabled IPv6 for UDPSocket/UDPBroadcast
//
// Revision 1.4  2004/05/26 09:32:35  mwang
// no message
//
// Revision 1.3  2004/05/13 03:06:09  shao
// added UDPSocket::sendto() and recvfrom()
//
// Revision 1.2  2004/05/13 02:02:27  shao
// contributed to ZQ::common
//
// ===========================================================================

#ifndef __ZQ_COM_UDPSOCKET_H__
#define __ZQ_COM_UDPSOCKET_H__

#include "ZQ_common_conf.h"
#include "Socket.h"

namespace ZQ {
namespace common {

class ZQ_COMMON_API UDPSocket;
class ZQ_COMMON_API UDPBroadcast;
class ZQ_COMMON_API UDPMulticast;
class ZQ_COMMON_API UDPReceive;

// -----------------------------
// class UDPSocket
// -----------------------------
/// UDP sockets implement the SOCK_DGRAM UDP protocol. They can be used 
/// to pass unverified messages between hosts, or to broadcast a specific
/// message to an entire subnet.  Please note that Streaming of realtime
/// data commonly use UDPDuplex related classes rather than UDPSocket.
/// A UDPSocket can be created bound to a specific network interface and/or
/// port address, though this is not required. UDP sockets also are usually
/// either connected or otherwise "associated" with a specific "peer" UDP
/// socket. Since UDP sockets operate through discreet packets, there are
/// NO streaming operators used with UDP sockets.
class UDPSocket : public Socket
{
private:
	Error setKeepAlive(bool enable);

protected:
	saddr_t peer;

public:

	/// Create an unbound UDP socket, mostly for internal use.
	UDPSocket(void);

	/// Create a UDP socket and bind it to a specific interface and port
	/// address so that other UDP sockets may find and send UDP messages
	/// to it. On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	UDPSocket(const InetAddress &bind, tpport_t port);

	/// Destructor
	virtual ~UDPSocket();

	/// bind onto a specific local address
	/// no later than state = stBound, overwrite previous call of setPeer()
	/// @param host address to send packets to.
	/// @param port number to deliver packets to.
	void bind(const InetAddress &localhost, tpport_t port =0);

	/// set the peer address to send message packets to. This can be set
	/// before every Send() call if nessisary.
	/// @param host address to send packets to.
	/// @param port number to deliver packets to.
	void setPeer(const InetAddress &host, tpport_t port);

	/// Examine address of sender of next waiting packet. This also sets
	/// "peer" address to the sender so that the next "send" message
	/// acts as a "reply".  This additional behavior overides the
	/// standard socket getSender behavior.
	/// @param pointer to hold port number.
	InetHostAddress getPeer(tpport_t *port = NULL);

	/// Send a message packet to a peer host.
	/// @param pointer to packet buffer to send.
	/// @param len of packet buffer to send.
	/// @return number of bytes sent.
	int send(const void *buf, size_t len);

	/// Send a message packet to a peer host.
	/// @param pointer to packet buffer to send.
	/// @param len of packet buffer to send.
	/// @param peerAddr the destination address.
	/// @param peerPort the destination port.
	/// @return number of bytes sent.
	int sendto(const void *buf, size_t len, const InetHostAddress& peerAddr, int peerPort);

	/// Receive a message from any host.
	/// @param pointer to packet buffer to receive.
	/// @param len of packet buffer to receive.
	/// @return number of bytes received.
	int receive(void *buf, size_t len);

	/// Receive a message from any host.
	/// @param pointer to packet buffer to receive.
	/// @param len of packet buffer to receive.
	/// @param peerAddr the source address.
	/// @param peerPort the source port.
	/// @return number of bytes received.
	int receiveFrom(void *buf, size_t len, InetHostAddress& peerAddr, int& peerPort);

	/// Examine contents of next waiting packet.
	/// @param pointer to packet buffer for contents.
	/// @param len of packet buffer.
	/// @return number of bytes examined.
	int peek(void *buf, size_t len);
};


// -----------------------------
// class UDPBroadcast
// -----------------------------
/// In addition to the UDP "socket" class, the UDPBroadcast is a socket that
/// is set to send messages to a subnet as a whole rather than to an
/// individual peer socket that it may be associated with. This class
/// provides an alternate binding and setPeer() capability for UDP sockets.
class UDPBroadcast : public UDPSocket
{
private:

	void setPeer(const InetHostAddress &ia, tpport_t port) {};

	Error setBroadcast(bool enable)
		{return Socket::setBroadcast(enable);};

public:

	/// Create and bind a subnet broadcast socket.
	/// @param address to bind socket under locally.
	/// @param port to bind socket under locally.
	UDPBroadcast(const InetAddress &bind, tpport_t port);

	/// Set peer by subnet rather than specific host.
	/// @param subnet of peer hosts to send to.
	/// @param port number to use.
	void setPeer(const BroadcastAddress &subnet, tpport_t port);
};	

// -----------------------------
// class UDPMulticast
// -----------------------------
/// In addition to the UDP "socket" class, the UDPMulticast is a socket that
/// is set to send messages to a multicast group rather than to an
/// individual peer socket that it may be associated with. This class
/// provides an alternate binding and setPeer() capability for UDP sockets.
class UDPMulticast : public UDPSocket
{
private:

	void setGroup(const InetHostAddress &ia, tpport_t port) {};

	Error setBroadcast(bool enable)
		{return Socket::setBroadcast(enable);};

public:

	/// Default constructor.
	UDPMulticast();

	/// Create and bind a subnet broadcast socket.
	/// @param address to bind socket under locally.
	/// @param port to bind socket under locally.
	UDPMulticast(const InetAddress &bind, tpport_t port);

	/// Set peer by subnet rather than specific host.
	/// @param group of multicast nodes to send to.
	/// @param port number to use.
	void setGroup(const InetMcastAddress &group, tpport_t port);

	/// Set the multicast time to live for a multicast socket.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param time to live.
	Error setTimeToLive(unsigned char ttl)
		{ return Socket::setTimeToLive(ttl); }
};

// -----------------------------
// class UDPReceive
// -----------------------------
/// the UDP receiver can receive data from another peer host or subnet.
class UDPReceive : public UDPSocket
{
public:

	/// Default constructor.
	UDPReceive();

	/// Create a UDP receiver, bind it to a specific interface and port
	/// address so that other UDP sockets may find and send UDP messages
	/// to it, and associate it with a given port on a peer host.
    /// On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	UDPReceive(const InetAddress &bind, tpport_t port);

	/// Associate this socket with a specified peer host.  The port number
	/// from the constructor will be used.  All UDP packets will be sent
	/// received from the specified host.
	/// @return 0 on success, -1 on error.
	/// @param host address to connect socket to.
	Error connect(const InetHostAddress &host, tpport_t port);

	/// Disassociate this socket from any host connection.  No data should
	/// be read or written until a connection is established.
	Error disconnect(void);

	/// Receive a data packet from the connected peer host.
	/// @return num of bytes actually received.
	/// @param addr of data receive buffer.
	/// @param size of data receive buffer.
	int receive(void *buf, size_t len);

	/// See if input queue has data packets available.
	/// @return true if data packets available.
	/// @param timeout in milliseconds.
	bool isInputReady(timeout_t timeout = TIMEOUT_INF);

	/// Check for pending data.
	/// @return true if data is waiting.
	/// @param timeout in milliseconds.
	bool isPendingReceive(timeout_t timeout);

	/// End receiver.
	inline void endReceiver(void)
		{ Socket::endSocket(); }

	inline SOCKET getReceiver(void)
		{ return _so; }

	inline Error setRouting(bool enable)
		{ return Socket::setRouting(enable); }

	inline Error setMulticast(bool enable)
		{ return Socket::setMulticast(enable); }

	inline Error join(const InetMcastAddress &ia)
		{ return Socket::join(ia); }

	inline Error drop(const InetMcastAddress &ia)
		{ return Socket::drop(ia); }

};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COM_UDPSOCKET_H__
