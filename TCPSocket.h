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
// Ident : $Id: Socket.cpp,v 1.7 2010/10/18 06:25:44 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define TCPSocket class
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/TCPSocket.h $
// 
// 7     1/25/11 5:38p Hui.shao
// query for tcp thread pool status
// 
// 6     1/14/11 7:24p Hui.shao
// moved Ponter into cpp
// 
// 5     1/14/11 7:05p Haoyuan.lu
// 
// 4     1/14/11 11:32a Hui.shao
// defined TCPServer, replaced the dummy socket from UDP to TCP conn at
// loopback interface
// 
// 3     12/31/10 6:41p Fei.huang
// + merged to linux
// 
// 2     10-11-24 18:16 Hui.shao
// made the watchdog instance per need
// 
// 7     10-11-02 17:18 Hui.shao
// moved the pending flags into TcpSocket
// 
// 6     10-10-22 13:46 Hui.shao
// take _timeout for connection idle check after the connected
// 
// 5     10-10-21 15:34 Hui.shao
// simplize with internal background threadpool, initial with size=3
// 
// 4     10-10-20 15:42 Hui.shao
// 
// 3     10-10-20 14:31 Hui.shao
// added TCPSocket, incompleted
// 
// 2     10-10-18 20:34 Hui.shao
// impl the TCP connection
// ===========================================================================

#ifndef __ZQ_COM_TCPSocket_H__
#define __ZQ_COM_TCPSocket_H__

#include "Socket.h"

namespace ZQ {

namespace common {

class TcpSocketWatchDog;

class ZQ_COMMON_API TCPSocket;
class ZQ_COMMON_API TCPServer;
class ZQ_COMMON_API TCPClient;

// -----------------------------
// class TCPSocket
// -----------------------------
class TCPSocket : public Socket
{
	friend class TcpSocketWatchDog;
	friend class SocketEventDispatcher;

protected:
	void createSocket();
	bool _bBlockEnable;
#ifdef ZQ_OS_MSWIN
	bool privateSetBlock(bool enable);
#endif

protected:

	enum {
		FLG_READ    = (1 <<pendingInput),
		FLG_WRITE   = (1 <<pendingOutput),
		FLG_ERROR   = (1 <<pendingError),
		FLG_TIMEOUT = (1 <<(pendingError+1))
	};

	saddr_t _peer;
	int32	_timeout;
	int64   _lastUpdated;
//	bool    _bRecvdDataInPending;

	char    _connDesc[128];
	uint8   _flagsPending;

	static TcpSocketWatchDog* _gWatchDog;
	static Mutex _lockWatchDog;
	static uint8 _poolSize;
	static void refWatchDog();
	static void unrefWatchDog();

	void   bind();

	virtual void OnConnected() {};
	virtual void OnError() {};
	virtual void OnDataArrived() {};
	virtual void OnTimeout() {};

	void updateConnDesc();

public:

	// about the backgroup thread pool
	static bool getThreadPoolStatus(int& poolSize, int& activeCount, int& pendingSize);
	static void resizeThreadPool(int newSize);

	typedef enum { NONBLOCK, BLOCK } RECVMODEL;

	/// Create a bound TCP socket, mostly for internal use.
	TCPSocket(void);

	// the copier
	TCPSocket(const TCPSocket &source);
	TCPSocket(const Socket &source);
	
	/// Create a TCP socket and bind it to a specific interface and port
	/// address so that other TCP sockets may find and send TCP messages
	/// to it. On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	TCPSocket(const InetAddress &bind, tpport_t port);

	/// Destructor
	virtual ~TCPSocket();

	/// set the peer address to send message packets to. This can be set
	/// before every Send() call if necessary.
	/// @param host address to send packets to.
	/// @param port number to deliver packets to.
	void setPeer(const InetAddress &host, tpport_t port);

	/// Examine address of sender of next waiting packet. This also sets
	/// "peer" address to the sender so that the next "send" message
	/// acts as a "reply".  This additional behavior overrides the
	/// standard socket getSender behavior.
	/// @param pointer to hold port number.
	InetHostAddress getPeer(tpport_t *port = NULL);

	/// Send a message packet to a peer host.
	/// @param pointer to packet buffer to send.
	/// @param len of packet buffer to send.
	/// @return number of bytes sent.
	int send(const void *buf, size_t len);

	/// Receive a message from any host.
	/// @param pointer to packet buffer to receive.
	/// @param len of packet buffer to receive.
	/// @param receive type
	/// @param receive operation timeout if use NOBLOCK type
	/// @return number of bytes received.
	int receive(void *buf, size_t len, RECVMODEL model = NONBLOCK, int32 timeOut = 0);

	/// Examine contents of next waiting packet.
	/// @param pointer to packet buffer for contents.
	/// @param len of packet buffer.
	/// @return number of bytes examined.
	int peek(void *buf, size_t len);

	/// Set TCP socket option
	/// @param true to set socket use block type to receive, false to set unblock type
	/// @return the operation result
	bool setBlock(RECVMODEL model = BLOCK);

	/// disconnect to TCP server
	/// @return success will return true, otherwise return false
	bool disconnect();

	int checkSoErr();
	const char* connDescription();
	int32 getTimeout() const { return _timeout; }

	/// if set prior to connect(), this setting would be overwritten by connect()
	int32 setTimeout(int32 newTimeout) { _timeout = newTimeout; return _timeout; }
};

// -----------------------------
// class TCPServer
// -----------------------------
class TCPServer : public TCPSocket
{
public:
	/// Create a TCP listen socket and bind it to a specific interface and port
	/// address so that other TCP sockets may find and send TCP messages
	/// to it. On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	TCPServer(const InetAddress &bind, tpport_t port) : TCPSocket(bind, port) { setTimeout(0); }
	virtual ~TCPServer() { stopListening(); }

	bool listen(int backlog =1000);
	void stopListening();

private:
	/// the OnDataArrived() becomes non-overwriteable since TCPServer, its implmentation
	/// would accept a connection from the client, then trigger the overwriteable
	/// new entry OnAccepted() instead
	void OnDataArrived(); // make OnDataArrived() non-overwriteable since TCPServer

	void OnTimeout() {}; // make OnTimeout() non-overwriteable since TCPServer

protected:
	// overwriteable declares
	virtual void OnError() {};

	virtual TCPSocket* OnAccepted(TCPSocket& preacceptedConn)
	{
//		return MyServerConn(acceptedConn);
		return NULL;
	};
};

// -----------------------------
// class TCPClient
// -----------------------------
class TCPClient : public TCPSocket
{
public:
	/// Create a TCP socket and bind it to a specific interface and port
	/// address so that other TCP sockets may find and send TCP messages
	/// to it. On failure to bind, an exception is thrown.
	/// @param bind address to bind this socket to.
	/// @param port number to bind this socket to.
	TCPClient(const InetAddress &bind, tpport_t port) : TCPSocket(bind, port) {}

	TCPClient(void) : TCPSocket() {}

	virtual ~TCPClient() {}

	/// Connect to a remote TCP server with specified port
	/// @param sTimeOut connect timeout in second, -1 means no timeout until server return, the
	///        class will take non-blocking mode to connect
	/// @return false if there are any problem occured
	/// @note with timeout greater than 0, the connect() would return instantly and the caller must
	///       take the OnConnected() callback to confirmed the connection established
	bool connect(int32 sTimeOut =-1);
};

#define TCPSocket_MAX_BACKLOG 4096

#define SB_DELTA 1024

}//namespace common

}//namespace ZQ
#endif 
