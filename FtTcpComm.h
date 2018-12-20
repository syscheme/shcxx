// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: FtTcpComm.h,v 1.12 2004/07/27 08:01:35 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : define wrapper to FtTcpComm APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/FtTcpComm.h $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     05-01-06 18:57 Jie.zhang
// remove the global gdwServiceType
// 
// 7     04-12-03 17:09 Daniel.wang
// 
// 6     04-09-14 13:55 Kaliven.lee
// 
// 5     04-08-27 15:47 Kaliven.lee
// add TYPEINST extern conference
// 
// 4     04-08-20 11:23 Hui.shao
// added operator const char*() for FtAddress
// Revision 1.12  2004/07/27 08:01:35  shao
// added active connection info
//
// Revision 1.11  2004/07/21 12:05:22  shao
// added comments
//
// Revision 1.10  2004/07/20 13:28:00  shao
// FtHostAddress filter if local address
//
// Revision 1.9  2004/07/20 08:26:38  shao
// replaced FtAddressPair with FtHostAddress to fix the leak
//
// Revision 1.8  2004/07/12 09:07:44  shao
// notice the user to delete server connection
//
// Revision 1.7  2004/07/10 13:10:15  shao
// fixed listener handle
//
// Revision 1.6  2004/07/10 12:54:21  shao
// fixed connection bug
//
// Revision 1.5  2004/07/09 11:20:55  shao
// fixed some callback impl
//
// Revision 1.4  2004/07/08 10:35:13  shao
// redefine the classes
//
// ===========================================================================
#ifndef __ZQ_COMMON_FTTCPCOMM_H__
#define __ZQ_COMMON_FTTCPCOMM_H__
//#include "MCastFwdConf.h"
#include "ZQ_common_conf.h"
#include "MtTcpcomm.h"
#include "log.h"
#include "Locks.h"
#include "InetAddr.h"

#include <vector>

namespace ZQ{
namespace common{

class ZQ_COMMON_API FtAddressPair;
class ZQ_COMMON_API FtTcpConnection;
class ZQ_COMMON_API FtTcpListener;
class ZQ_COMMON_API FtHostAddress;

#define DEFAULT_STATE_THREADS 3
#define DEFAULT_TOS      100

#ifndef DEFAULT_SERVICETYPE
#define DEFAULT_SERVICETYPE 0xffffffff
#endif //DEFAULT_SERVICETYPE

#ifndef DEFAULT_SERVICEINST
#define DEFAULT_SERVICEINST 0x0
#endif //DEFAULT_SERVICEINST

//extern const DWORD gdwServiceType;	
// @example testcli.cpp


// -----------------------------
// class FtHostAddress
// -----------------------------
/// FtHostAddress is designed to work with FtTcpComm API wrapper
/// It usually contains multiple address for the same machine
class FtHostAddress : protected ZQ::common::InetHostAddress
{
public:

	/// constructor
	/// @param filterLocal   true if filter the non-local address out
	///                      when addAddress() is called
	FtHostAddress(bool filterLocal=false)
		: ZQ::common::InetHostAddress(), _bfilterLocal(filterLocal)
		{ clear(); }

	/// copier
	FtHostAddress(const FtHostAddress& other)
		: ZQ::common::InetHostAddress(other), _bfilterLocal(other._bfilterLocal) {};

	/// destructor
	virtual ~FtHostAddress() {};

	/// @return     true if the address contained has been already filtered for 
	///             the local machine
	bool filteredLocal(void) const { return _bfilterLocal; }

	// export some InetAddress methods

	/// Provide a low level system usable struct in_addr object from
	/// the contents of InetAddress.  This is needed for services such
	/// as bind() and connect().
	/// @param i for InetAddresses with multiple addresses, returns the
	///	address at this index.  User should call getAddressCount() 
	///	to determine the number of address the object contains.
	/// @return system binary coded internet address.  If parameter i is
	///	out of range, the first address is returned.
	inetaddr_t getAddress(size_t i =0) const
	{ return InetHostAddress::getAddress(i); }

	/// Returns the number of internet addresses that an InetAddress object
	/// contains.  This usually only happens with InetHostAddress objects
	/// where multiple IP addresses are returned for a DNS lookup
	size_t getAddressCount() const
	{ return InetHostAddress::getAddressCount(); }

	const uint16 family(size_t i =0) const
	{ return InetHostAddress::family(i); }

	/// Provide a numeric string representation of the value (Internet Address)
	/// held in the InetAddress object.
	const char *getHostAddress(size_t i =0) const
	{ return InetHostAddress::getHostAddress(i); }


	/// add additional address
	/// @param address    numeric IP address to add in
	FtHostAddress &operator+=(const char *address);

	/// add additional address
	/// @param address    numeric IP address to add in
	/// @return     true if succ
	bool addAddress(const char* address);

	/// get the display address string
	/// @return     a list of address delimited by ';', for example "192.168.1.10;192.168.10.10"
	operator const char* ();

	/// clear all addresses
	void clear();

private:

	std::string _addrstr;

	bool _bfilterLocal;
};

// -----------------------------
// class FtTcpConnection
// -----------------------------
/// A dual rail Tcp Connection
class FtTcpConnection
{
	friend class FtTcpListener;
	friend class FtTcpState;

protected:

	FtTcpConnection(TCP_CHANDLE handle, FtTcpListener* pListener);
	TCP_CHANDLE handle() { return _handle;}

public:

	/// Default constructor
	FtTcpConnection(DWORD type=0, DWORD inst=0, ConnectOptions_t* pOptions=NULL);

	/// Default destructor
	virtual ~FtTcpConnection();

	bool isConnected() {return (_handle!=NULL);}

	TYPEINST* typeinst() { return &_localTypeInst; }

	bool isPassive() { return _pListener!=NULL; }

	/// Initiate a new fault tolerant connection with a remote node. 
	/// @note now only support 2 addresses.
	/// @param wPort			the port number on which to initiate the connection (remote port)
	/// @param LocalAddrs		List of local IP ports to use to establish a connection. The first address in
	///                         the array is the PRIMARY address. Note: Only supplying one address is a WARNING
	///                         case and will be logged in the NT Event Log as such.
	/// @param RemoteAddrs		The list of remote addrs to which this conection can be made.
	///                         The first address is the Primary address. Note: Only supplying one address is
	///                         a WARNING case and will be logged in the NT Event Log as such. 
	/// @param pUserData		The user data that the user wants to send over the wire to the remote
	///                         node This is different from context data. !!This must be allocated with
	///                         TcpState_Malloc!! Once passed to this function, this memory belongs	to TcpState.
	/// @param dwDataLen		Length of UserData
	/// @param TOS				The quality of connection structure with TOS or QOS information for this
	///                         FtConnection, this may be NULL for normal/default QOS.
	bool connect(WORD wPort, const FtHostAddress& LocalAddress, const FtHostAddress& RemoteAddress, int TOS=0, BYTE* pUserData=NULL, DWORD dwDataLen=0);

	/// drop this connection
	/// @param reason			A value to indicate to the OnClosed() callback how the connection was dropped.
	virtual void close(DWORD reason=0);

	/// Send the message pointed to in msg; msg does NOT include the TCPState header. 
	/// The big concern here is memory handling:@n
	/// 1. Memory used in this call must be allocated with TCPState_Malloc. TCPState is responsible for the
	/// memory once it is passed to the TcpStateSend call. That is, TCPState will free it, under the covers,
	/// without your help. Exception: When TcpStateSend returns an error, the CALLER is reponsible for
	/// freeing the memory.@n
	/// 2. The user may specify a callback routine that will let the user manage his/her own memory, see
	/// InitializeTcpStateSubsystem().@n
	/// There is no notification when the Send completes. When this call returns success, All sends are
	/// guaranteed to at least be queued within the system if not immediately sent to TCP.
	///	@param msg				The message to send over the wire. The user doesn't worry about the header.
	/// @param len		Number of bytes in msg 
	/// @return					if this call returns false, the caller is responsible for the memory. 
	bool send(void* msg, int len);

	/// Send an array of buffers. 
	/// The buffers will be concatenated by WINSOCK (MtTcpComm will NOT	do a single memcpy) into a single
	/// message to send to the other side of the connection. The order in which the buffers arrive on the 
	/// remote side is the same as they appear in the array, by index. 0 is first followed by 1, then 2, etc.
	/// This is especicially useful for creating a single message that has multiple parts while avoiding 
	/// unneccesary memcpys.@n
	/// Memory handling is the same as TcpStateSend. All buffers sent must be allocated with TcpState_Malloc
	/// and MtTcpComm is responsible for freeing every buffer.@n
	/// There is no notification when the Send completes. When this call returns success, All sends are
	/// guaranteed to at least be queued within the system if not immediately sent via TCP.
	/// @param pIOBufs			an array of MtTcpSend buffers (see struct _MtTcpIOBuffer above) 
	/// @param dwNumBufs		number of buffers in the array. 
	/// @return					if this call returns false, the caller is responsible for the memory. 
	bool send(PIOBUF pIOBufs, DWORD dwNumBufs);

	/// This allows the user to get the connection options associated with the given connection. 
	/// If options have not been set, the defaults are returned.
	/// @param pOptions			This structure receives the connection options. 
	bool getOptions(pConnectOptions_t pOptions);

	/// This allows the user to set the connection options associated with the given connection.
	/// @param pOptions		The new connection options for the given connection or listener.
	/// Use the dwFlags member of pConnectOptions to indicate which structure
	/// members are valid.
	bool setOptions(pConnectOptions_t pOptions);

	/// During a pNotifyAccept callback, this can be called by a Responder to tell the remote connection
	/// Intiator what is the precise TYPEINST for this side of the connection. @n
	/// This information is passed back to the initiator when pNotifyAccept returns and the connection
	/// handshake is completed. Calling this function at any other time does nothing (but don't go trying it).
	bool setLocalIdent(TYPEINST &LocalIdent);

	/// Retrieve the TYPEINST identity of this connection.
	TYPEINST getLocalIdent() { return _localTypeInst; }

	/// Retrieve the TYPEINST identity of the connected remote node.
	TYPEINST getRemoteIdent() { return _remoteTypeInst; }

	/// Retrieve the IP addr (.notation or hostname) of the connected remote node
	const char* getRemoteAddr() { return _remoteAddr.c_str(); }

	/// Retrieve the IP addr (.notation or hostname) of this node to make the connection
	const char* getLocalAddr() { return _localAddr.c_str(); }

	/// Retrieve the IP port number of the remote node
	const WORD getRemotePort();

protected:

	/// Event when a NewConnection call or a FtNewConnection call has completed and that connection has been
	/// accepted or denied by the remote node.
	/// Errors will be reported in dwError, including why a connection was denied. If dwError == TCPSTATE_WSA_ERROR
	/// then iError has the WSA error code. If the connection was originally started with FtNewConnection, and
	/// dwError == TCPSTATE_SUCCESS, iError indicates how the connection was made. Either FTSTATE_CONNECTED_NORMAL
	/// or FTSTATE_CONNECTED_FAILOVER. If iError == FTSTATE_CONNECTED_FAILOVER, you can expect a call to
	/// pFtNotifyStateChange later when the P-P connection has been re-established. 
	/// @param dwError			The TCPSTATE_ error associated with this callback. 
	/// @param iError			The WSA network error when dwError is TCPSTATE_WSA_ERROR or the FtConnection
	///                         type (see comments above).
	virtual void OnConnected(DWORD dwError, int iError)
	{
	}

	/// Event when an overlapped message is received
	/// @param message			The received message buffer. Note: This memory must be freed by the user, with
	///                         a call to TcpState_Free(). 
	/// @param len				The length of the message received 
	virtual void OnDataArrive(void *message, int len)
	{

#ifdef _DEBUG
		// dump the message to screen
		char *msg= (char*) message;
		for (int i =0; i< len; i++, msg++)
			printf("%c", isprint(*msg) ? *msg : '.');
		printf("\n");
#endif // _DEBUG
	}

	/// Event when a connection is closed.
	/// When the connection is a fault tolerant connection, this callback indicates that the user wanted to
	/// drop the connection or a failover was in progress and every possible address pair failed to connect.
	/// The dwError code lets you know the reason. 
	/// @param dwError			The error assciated with this notification. 
	/// @param iWsaError		The WSA network error associated with this notification.
	virtual void OnClosed(DWORD dwError, int iWsaError){}

	/// Event when the underlying TCP connection has been lost. FtComm is attempting to reconnect. Discontinue IO
	/// and wait for state update, or pNotifyConnectClose if things go south. Your app is expected to re-start
	/// communication once the connection is re-established. FtComm can not guarantee which messages were picked up
	/// by the other side before the failure ocurred.
	/// @param pLocalAddr	The FtComm address for the local port that has failed, or reconnected
	/// @param pRemoteAddr	The FtComm address for the remote port that has failed, or reconnected 
	/// @param dwError		The TCPSTATUS indicating why the failure occured
	/// @param iWsaError	The WSA specific error when dwError is TCPSTATE_WSA_ERROR 
	virtual void OnFalingOver(const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError){}

	// StateChanage events
	/// Event fired when the underlying TCP connection is moving from a FAILOVER address pair back to the
	/// PRIMARY-PRIMARY connection. This is goodness and will not disrupt comms in any way. This is just a
	/// notification that this is taking place.
	/// @param pLocalAddr	The FtComm address for the local port that has failed, or reconnected
	/// @param pRemoteAddr	The FtComm address for the remote port that has failed, or reconnected 
	/// @param dwError		The TCPSTATUS indicating why the failure occured
	/// @param iWsaError	The WSA specific error when dwError is TCPSTATE_WSA_ERROR 
	virtual void OnFallingBack(const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError){}

	/// Event whe the underlying TCP connection has been re-established on one or more backup ports
	/// @param pLocalAddr	The FtComm address for the local port that has failed, or reconnected
	/// @param pRemoteAddr	The FtComm address for the remote port that has failed, or reconnected 
	/// @param dwError		The TCPSTATUS indicating why the failure occured
	/// @param iWsaError	The WSA specific error when dwError is TCPSTATE_WSA_ERROR 
	virtual void OnConnectedFailover(const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError){}

	/// Event when the underlying TCP connection has been re-established on the two primary addresses. 
	/// Also, called after
	/// @param pLocalAddr	The FtComm address for the local port that has failed, or reconnected
	/// @param pRemoteAddr	The FtComm address for the remote port that has failed, or reconnected 
	/// @param dwError		The TCPSTATUS indicating why the failure occured
	/// @param iWsaError	The WSA specific error when dwError is TCPSTATE_WSA_ERROR 
	virtual void OnConnectedNormal(const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError){}

protected:

	FtTcpListener* _pListener;
	TCP_CHANDLE _handle;
	ConnectOptions_t _opts;

	HANDLE    _hConnectEvent;

	// the working address on both end of the connection
	std::string _localAddr, _remoteAddr;

	// the working port on both ends of the connection
	DWORD _localPort, _remotePort;

	static std::string _establishTimeStr;


	TYPEINST _remoteTypeInst;
	TYPEINST _localTypeInst;

private:

	// FtTcpComm API callbacks for connection
	static TCPSTATUS  cbNotifyReceived ( TCP_IOHANDLE hIORequest, BYTE* Message, DWORD len, DWORD dwError);
	static TCPSTATUS  cbNotifyConnect( TCP_CHANDLE hConnect, DWORD dwError, int iError);
	static TCPSTATUS  cbNotifyConnectClose( TCP_CHANDLE hConnect, DWORD dwError, int iWsaError);
	static TCPSTATUS  cbNotifyStateChange(TCP_CHANDLE hConnect, FTSTATE eState, const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError);

};

// -----------------------------
// class FtTcpListener
// -----------------------------
/// Simple wrap about a LHandle, just like a listen socket
class FtTcpListener
{
	friend class FtTcpState;

public:
	/// Default constructor
	FtTcpListener(DWORD type = DEFAULT_SERVICETYPE, DWORD inst = DEFAULT_SERVICEINST);

	/// Default destructor
	virtual ~FtTcpListener() {}

	TCP_LHANDLE handle(){return _handle;}
	TYPEINST* typeinst() { return &_localTypeInst; }

	/// Start a listener on the given port. 
	/// (IN) portnum -- The Port number to listen on, or ANY_PORT to select a random port.
	/// (IN) LocalAddrs  You may use ANY_ADDR for nodename, this will 
	///                  result in a listener port across a NIC randomly
	/// (IN) pQOC -- The quality of connection structure with TOS or QOS information for this
	///              FtConnection, this may be NULL for normal/default QOS.
	bool listen(const WORD portnum, const FtHostAddress &LocalAddress, const int TOS=0);

	/// close the listener
	void close(DWORD reason=0);

	bool isAlive()
	{
		return (_handle!=NULL);
	}

	WORD getPort();

	const char* getBindAddrs() { return (const char*) _bindAddrs; }

protected:

	/// Event fired when a listener has accepted a new connection. 
	/// The user has access to the userdata that was sent over the wire, see GetUserData(). The user can accept
	/// this connection by returning TCPSTATE_SUCCESS. Otherwise the error code returned will be sent to the
	/// remote client and the connection will be denied (torn down). The user can also reset the options on
	/// this connection (see the ConnectionOptions structure above, and SetConnectOptions() below). Note: The
	/// underlying connection is not officially "CONNECTED" until shortly after this call is returned by the
	/// user. All calls to Drop, Send or Recv, will fail with TCPSTATE_NOT_CONNECTED 
	/// @param ppConn			the FtTcpConnection instance to be created, *pConn ==NULL will be trust as
	///                         reject the connection                     
	/// @param hConn			a handle to the new connection to be accepted (not currently CONNECTED) 
	virtual void OnAccept(FtTcpConnection **ppConn, const TCP_CHANDLE hConn)
	{
		if (ppConn==NULL)
			return;
		
		if(hConn == NULL)
		{
			*ppConn = NULL;
			return;
		}

		// NOTE: FtTcpConnection/FtTcpListener are not response for releasing the server-side FtTcpConnection instances.
		//       user must impl the releasing
		if((*ppConn = new FtTcpConnection(hConn, this)) ==NULL)
			return;

		(*ppConn)->OnConnected(TCPSTATE_SUCCESS, 0);
	}

	/// Called when a listener port is stopped.
	/// @param dwError			The error assciated with this notification. This will be TCPSTATE_SUCCESS,
	///                         TCPSTATE_DROPPED, or a WASError
	virtual void OnClosed(DWORD dwError) {}

private:

	TCP_LHANDLE _handle;

	TYPEINST    _localTypeInst;
	ConnectOptions_t _opts;

	FtHostAddress _bindAddrs;
	// FtTcpComm API callbacks for listener
	static TCPSTATUS  cbNotifyAccept( TCP_LHANDLE hListener,  TCP_CHANDLE hConnect, int iError);
	static TCPSTATUS  cbNotifyListenerClose( TCP_LHANDLE hListener, DWORD dwError);

};

} // namespace common
} // namespace ZQ

#endif __ZQ_COMMON_FTTCPCOMM_H__