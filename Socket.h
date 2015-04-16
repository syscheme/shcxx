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
// Ident : $Id: Socket.h,v 1.5 2004/07/09 11:21:36 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define Socket class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/Socket.h $
// 
// 5     1/24/11 5:53p Fei.huang
// 
// 3     1/19/11 11:23a Hui.shao
// 
// 2     1/14/11 12:11p Hui.shao
// 
// 7     10-10-20 14:31 Hui.shao
// added TCPSocket, incompleted
// 
// 6     09-06-23 15:23 Fei.huang
// * define SOCKET as int on linux
// 
// 5     08-03-06 16:32 Hui.shao
// changed maro WIN32 to ZQ_OS_MSWIN
// 
// 2     8/30/04 10:43a Jie.zhang
// Revision 1.5  2004/07/09 11:21:36  shao
// no message
//
// Revision 1.4  2004/06/21 04:05:32  shao
// enabled IPv6 for UDPSocket/UDPBroadcast
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

#ifndef __ZQ_COM_SOCKET_H__
#define __ZQ_COM_SOCKET_H__

#include "ZQ_common_conf.h"
#include "Exception.h"
#include "InetAddr.h"

#ifdef ZQ_OS_MSWIN
#  include <winsock2.h>
#  define TIMEOUT_INF ~((timeout_t) 0)
   typedef int socklen_t;
#  ifndef ssize_t
#    define ssize_t int
#  endif
#  ifndef timeout_t
     typedef DWORD   timeout_t;
#  endif
#else
#  define SOCKET int
#  define  INVALID_SOCKET (-1)
#  define  SOCKET_ERROR	(-1)
#endif

#ifdef ZQ_OS_MSWIN
#  ifndef ssize_t
#    define ssize_t int
#  endif
#  define socket_errno WSAGetLastError()
#else
#define socket_errno errno
#endif


#ifndef	MSG_DONTWAIT
#  define	MSG_DONTWAIT	0
#endif

namespace ZQ {
namespace common {

class ZQ_COMMON_API Socket;

/// Transport Protocol Ports.
typedef unsigned short tpport_t;

// -----------------------------
// class Socket
// -----------------------------
/// The Socket is used as the base for all Internet protocol services.
/// A socket is a system resource (or winsock descriptor) that occupies
/// a specific port address (and may be bound to a specific network
/// interface) on the local machine.  The socket may also be directly
/// connected to a specific socket on a remote internet host.

/// This base class is not directly used, but is provided to offer
/// properties common to other socket classes, including the socket
/// exception model and the ability to set socket properties such as
/// QoS, "sockopts" properties like Dont-Route and Keep-Alive, etc.
class Socket
{
public:
	enum Error
	{
		errSuccess = 0,
		errCreateFailed,
		errCopyFailed,
		errInput,
		errInputInterrupt,
		errResourceFailure,
		errOutput,
		errOutputInterrupt,
		errNotConnected,
		errConnectRefused,
		errConnectRejected,
		errConnectTimeout,
		errConnectFailed,
		errConnectInvalid,
		errConnectBusy,
		errConnectNoRoute,
		errBindingFailed,
		errBroadcastDenied,
		errRoutingDenied,
		errKeepaliveDenied,
		errServiceDenied,
		errServiceUnavailable,
		errMulticastDisabled,
		errTimeout,
		errNoDelay,
		errExtended
	};

	typedef enum Error Error;
	
	enum Pending
	{
		pendingInput,
		pendingOutput,
		pendingError
	};
	typedef enum Pending Pending;

	typedef struct _saddr
	{
		short family;
		union
		{
			uint8 reserved[16];
			sockaddr_in  a;
#ifdef IPV6_ENABLED
			sockaddr_in6 a6;
#endif // IPV6_ENABLED
		} sa;
	} saddr_t;

	typedef enum _Tos
	{
		tosLowDelay = 0,
		tosThroughput,
		tosReliability,
		tosMinCost,
		tosInvalid
	} Tos;

public:
	typedef enum _State
	{
		stInitial,
		stAvailable,
		stBound,
		stConnecting,
		stConnected,
		stStream
	} State;

private:
	/// used by exception handlers....
	mutable Error _errid;
	mutable const char *_errstr;
	mutable int16 _errnum;
	
	friend SOCKET dupSocket(SOCKET s, Socket::State state);
	friend class TcpConnWatchDog;
	friend class SocketEventDispatcher;
	friend class TCPServer;

protected:
	mutable struct sf_t
	{
		bool thrown		: 1;
		bool broadcast	: 1;
		bool route		: 1;
		bool keepalive	: 1;
		bool loopback	: 1;
		bool multicast	: 1;
		bool completion	: 1;
		bool linger		: 1;
		unsigned ttl	: 8;
	} flags;

	void setSocket(void); /// init socket object

	/// the actual socket descriptor, in Windows, unlike posix it 
	/// *cannot* be used as an file descriptor that way madness lies
	SOCKET _so;
	State  _state;

	/// Constructors
	/// An unconnected socket may be created directly on the local machine.
	/// Sockets can occupy both the internet domain (PF_INET) and UNIX
	/// socket domain (AF_UNIX) under unix.  The socket type
	/// (SOCK_STREAM, SOCK_DGRAM) and protocol may also be specified.
	/// If the socket cannot be created, an exception is thrown.
	/// @param domain socket domain to use.
	/// @param type base type and protocol family of the socket.
	/// @param protocol specific protocol to apply.
	Socket(int domain, int type, int protocol = 0);

	/// A socket object may be created from a file descriptor when that
	/// descriptor was created either through a socket() or accept() call.
	/// This constructor is mostly for internal use.
	/// @param fd file descriptor of an already existing socket.
	Socket(SOCKET fd);

	/// Copier constructor, The socket file descriptor is dup()'d. This
	/// does not exist under win32.
	/// @param source of existing socket to clone.
	Socket(const Socket &source);

	/// This service is used to throw all socket errors which usually
	/// occur during the socket constructor.
	/// @param error defined socket error id.
	/// @param _errstr string or message to pass.
	Error error(const Error err, const char *errs=NULL, const uint16 errnum=0) const;

	/// This service is used to throw application defined socket errors
	/// where the application specific error code is a string.
	/// @param _errstr string or message to pass.
	void error(char *estr);
	
	/// This service is used to turn the error handler on or off for
	/// "throwing" exceptions by manipulating the thrown flag.
	/// @param true to enable handler.
	void setError(bool enable);

	/// Used as the default destructor for ending a socket.  This will
	/// cleanly terminate the socket connection.  It is provided
	/// for use in derived virtual destructors.
	void endSocket(void);

	/// Used as a common handler for connection failure processing.
	/// @return correct failure code to apply.
	Error connectError(void);

	/// Set the subnet broadcast flag for the socket.  This enables
	/// sending to a subnet and may require special image privileges
	/// depending on the operating system.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param enable when set to true.
	Error setBroadcast(bool enable);

	/// Setting multicast binds the multicast interface used for the
	/// socket to the interface the socket itself has been implicitly
	/// bound to.  It is also used as a check flag to make sure 
	/// multicast is enabled before multicast operations are used.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param enable when set to true.
	Error setMulticast(bool enable);

	/// Set the multicast loopback flag for the socket. Loopback enables
	/// a socket to hear what it is sending.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param enable when set to true.
	Error setLoopback(bool enable);

	/// Set the multicast time to live for a multicast socket.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param time to live.
	Error setTimeToLive(unsigned char ttl);

	/// Join a multicast group.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param ia address of multicast group to join.
	Error join(const InetMcastAddress &ia);

	/// Drop membership from a multicast group.
	/// @return 0 (SOCKET_SUCCESS) on success, else error code.
	/// @param address of multicast group to drop.
	Error drop(const InetMcastAddress &ia);

	/// Set the socket routing to indicate if outgoing messages should
	/// bypass normal routing (set false).
	/// @return 0 on success.
	/// @param enable normal routing when set to true.
	Error setRouting(bool enable);

	/// Enable/disable delaying packets (Nagle algorithm)
	/// @return 0 on success.
	/// @param disable Nagle algorithm when set to true.
	Error setNoDelay(bool enable);

	/// Process a logical input line from a socket descriptor directly.
	/// @param pointer to string.
	/// @param maximum length to read.
	/// @param timeout for pending data in milliseconds.
	/// @return number of bytes actually read.
	ssize_t readLine(char *buf, size_t len, timeout_t timeout = 0);

public:

	/// The socket base class may be "thrown" as a result of an error,
	/// and the "catcher" may then choose to destroy the object. By
	/// assuring the socket base class is a virtual destructor, we can
	/// assure the full object is properly terminated.
	virtual ~Socket();

	/// Sockets may also be duplicated by the assignment operator.
	Socket &operator=(const Socket &from);

	SOCKET get() const { return _so; }
	State state() const { return _state; }

	/// May be used to examine the origin of data waiting in the socket
	/// receive queue.  This can tell a TCP server where pending "connect"
	/// requests are coming from, or a UDP socket where it's next packet
	/// arrived from.
	/// @param ptr to port number of sender.
	/// @return host address, test with "isInetAddress()".
	InetHostAddress getSender(tpport_t *port = NULL) const;

	/// Get the host address and port of the socket this socket is
	/// connected to. If the socket is currently not in a connected
	/// state, then a host address of 0.0.0.0 is returned.
	/// @param ptr to port number of remote socket.
	/// @return host address of remote socket.
	InetHostAddress getPeer(tpport_t *port = NULL) const;

	/// Get the local address and port number this socket is currently
	/// bound to.
	/// @param ptr to port number on local host.
	/// @return host address of interface this socket is bound to.
	InetHostAddress getLocal(tpport_t *port = NULL) const;
	
	/// Used to specify blocking mode for the socket.  A socket can be
	/// made non-blocking by setting setCompletion(false) or set to block
	/// on all access with setCompletion(true). not sure if this form of
	/// non-blocking socket I/O is supported in winsock, though it
	/// provides an alternate asynchronous set of socket services.
	/// @param mode specify socket I/O call blocking mode.
	void setCompletion(bool immediate);

	/// Enable lingering sockets on close.
	/// @param specify linger enable.
	Error setLinger(bool linger);

	/// Set the keep-alive status of this socket and if keep-alive
	/// messages will be sent.
	/// @return 0 on success.
	/// @param enable keep alive messages.
	Error setKeepAlive(bool enable);

	/// Set packet scheduling on platforms which support ip quality of
	/// service conventions.  This effects how packets in the queue are
	/// scheduled through the interface.
	/// @param type of service enumerated type.
	/// @return 0 on success, error code on failure.
	Error setTypeOfService(Tos service);

	/// Can test to see if this socket is "connected", and hence whether
	/// a "catch" can safely call getPeer().  Of course, an unconnected
	/// socket will return a 0.0.0.0 address from getPeer() as well.
	/// @return true when socket is connected to a peer.
	bool isConnected(void) const;

	/// Test to see if the socket is at least operating or if it is
	/// mearly initialized.  "initialized" sockets may be the result of
	/// failed constructors.
	/// @return true if not in initial state.
	bool isActive(void) const;

	/// Operator based testing to see if a socket is currently active.
	bool operator!() const;

	/// Return if broadcast has been enabled for the specified socket.
	/// @return true if broadcast socket.
	inline bool isBroadcast(void) const
		{return flags.broadcast;};

	/// Return if socket routing is enabled.
	inline bool isRouted(void) const
		{return flags.route;};

	/// used by a "catch" to get the last error of a thrown socket.
	/// @return error number of Error error.
	inline Error getErrorNumber(void) const {return _errid;}
	
	/// used by a "catch" to fetch the user set error string of a
	/// thrown socket, but only if EXTENDED error codes are used.
	inline const char *getErrorString(void) const {return _errstr;}

	/// Get the status of pending operations.  This can be used to
	/// examine if input or output is waiting, or if an error has occured
	/// on the descriptor.
	/// @return true if ready, false on timeout.
	/// @param ready check to perform.
	/// @param timeout in milliseconds, inf. if not specified.
	virtual bool isPending(Pending pend, timeout_t timeout = TIMEOUT_INF);
};

inline InetAddress::inetaddr_t getaddress(const InetAddress &ia)
{ return ia.getAddress(); }

/// Dup util
#ifdef ZQ_OS_MSWIN
#  define DUP_SOCK(s,state) dupSocket(s,state)
#else
#  define DUP_SOCK(s,state) dup(s)
#endif 

class SockException : public IOException
{
public:
	SockException(std::string str) : IOException(str) {};
};

} // namespace common
} // namespace ZQ

#endif // __ZQ_COM_SOCKET_H__
