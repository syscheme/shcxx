// ===========================================================================
// Copyright (c) 2004 by
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
// Ident : $Id: FtTcpComm.cpp,v 1.13 2004/07/27 08:01:35 shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : impl wrapper to FtTcpComm APIs
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/FtTcpComm.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 8     04-12-02 16:25 Daniel.wang
// 
// 7     04-09-30 14:14 Hui.shao
// 
// 6     04-09-14 13:55 Kaliven.lee
// 
// 5     04-08-20 11:23 Hui.shao
// added operator const char*() for FtAddress
// 
// 4     04-08-20 11:11 Kaliven.lee
// Revision 1.13  2004/07/27 08:01:35  shao
// added active connection info
//
// Revision 1.12  2004/07/22 09:41:50  shao
// IPV6_ENABLED range bug
//
// Revision 1.11  2004/07/20 13:28:00  shao
// FtHostAddress filter if local address
//
// Revision 1.10  2004/07/20 08:26:38  shao
// replaced FtAddressPair with FtHostAddress to fix the leak
//
// Revision 1.9  2004/07/12 09:06:52  shao
// test the connected condition
//
// Revision 1.8  2004/07/10 13:10:15  shao
// fixed listener handle
//
// Revision 1.7  2004/07/10 12:54:21  shao
// fixed connection bug
//
// Revision 1.6  2004/07/09 11:20:55  shao
// fixed some callback impl
//
// Revision 1.5  2004/07/08 10:35:13  shao
// redefine the classes
//
// ===========================================================================

#include "FtTcpComm.h"
#include "InetAddr.h"
#include "log.h"
#include <assert.h>

#pragma comment(lib, "MtTcpComm" VODLIBEXT)

namespace ZQ{
namespace common{
std::string FtTcpConnection::_establishTimeStr;

// -----------------------------
// class FtTcpState
// -----------------------------
/// Simple wrap of TCP_UHANDLE see mcftcomm.h for more.
/// This class is mostly used internal (by MtTcpConnection,MtTcpListener..)
class FtTcpState
{

public:

	/// constructor
	/// @param threads			Must be a value between 3 and 5, may be 0 to indicate the default.
	/// @param pmemcallback		Memory allocation user callback routines, or NULL.
	FtTcpState(DWORD threads=DEFAULT_STATE_THREADS)
		:_handle(NULL)
	{
/*
		memset(&_cbFunc, 0, sizeof(_cbFunc));
		_cbFunc.pNotifyReceived =       FtTcpConnection::cbNotifyReceived;
		_cbFunc.pNotifyConnect =        FtTcpConnection::cbNotifyConnect;
		_cbFunc.pNotifyAccept =         FtTcpListener::cbNotifyAccept;
		_cbFunc.pNotifyConnectClose =   FtTcpConnection::cbNotifyConnectClose;
		_cbFunc.pNotifyListenerClose =  FtTcpListener::cbNotifyListenerClose;
		_cbFunc.pFtNotifyStateChange =  FtTcpConnection::cbNotifyStateChange;
*/

		TcpStateMemCallBacks_t cbMem = {FtTcpState::_malloc, FtTcpState::_free};

		DWORD errorcode=0;
		_handle=InitializeTcpStateSubsystem(&cbMem, threads, &errorcode);

		//throw exception here?? will cause error later... so no throw here.
		if(_handle==NULL) 
			glog(Log::L_DEBUG, "%s:InitializeTcpStateSubsystem error, code=%u",__FUNCTION__,errorcode);
	}

	bool isValid() { return (_handle!=NULL); }

	TCP_UHANDLE handle() { return _handle; }

	/// default destructor, do nothing
	~FtTcpState()
	{ 
		if(!isValid())
			return;

		//very stange , why need a pointer ,?? it is a handle after all.will NULL it?
		TCPSTATUS result=ReleaseTcpStateSubsystem(&_handle);
		if(result==TCPSTATE_SUCCESS)
			_handle=NULL;
		else
			glog(Log::L_ERROR, "%s:ReleaseTcpStateSubsystem error",__FUNCTION__);
	}

	BYTE* malloc(DWORD size)
	{
		DWORD err;
		if (size <=0)
			return NULL;
		return TcpState_Malloc(_handle, size, &err);
	}

	DWORD free(BYTE* buf)
	{
		if (buf == NULL)
			return TCPSTATE_SUCCESS;

		return TcpState_Free(_handle, buf);
	}

private:

    // Called when TcpState_Malloc is called.
	static BYTE* _malloc( DWORD dwNumBytes, DWORD *pdwError =NULL)
	{
		BYTE* p = (BYTE*) new char[dwNumBytes];
		if (pdwError)
			*pdwError = (p==NULL) ? TCPSTATE_OUT_OF_MEMORY : TCPSTATE_SUCCESS;

		return p;
	}

	static TCPSTATUS _free( BYTE* pBuf )
	{
		char* p = (char*) pBuf;
		if (p==NULL)
			return TCPSTATE_BAD_PARAM;

		delete[] p;

		return TCPSTATE_SUCCESS;
	}
    // Called when TcpState_Free is called.
	FAR TCPSTATUS  ( __cdecl *pFreeMessage)( BYTE* pBuf );

	TCP_UHANDLE _handle;
};

FtTcpState gState;

// ---------------------------------
// FtTcpComm callback implementation
// ---------------------------------

/// Called when an overlapped message is received
/// @param hIORequest		the IORequest handle for the completed operation. This handle is valid only
///                         for the duration of this callback. 
/// @param Message			The received message buffer. Note: This memory must be freed by the user, with
///                         a call to TcpState_Free(). 
/// @param len				The length of the message received 
/// @param dwError			The WSA error associated with this message.
/// @return					The user should return TCPSTATE_SUCCESS, by returning any error the user can
///                         cancel future RECV_ALWAYS calls. Returning an error on a RECV_ONCE call does nothing.
TCPSTATUS  FtTcpConnection::cbNotifyReceived(TCP_IOHANDLE hIORequest, BYTE* Message, DWORD len, DWORD dwError)
{

	glog(Log::L_DEBUG, LOGFMT("cbNotifyReceived handle %x error: %d message:%x(%d)"), hIORequest, dwError, Message, len);
	if(dwError!=TCPSTATE_SUCCESS || Message == NULL)
		return TCPSTATE_SUCCESS;

	DWORD errorcode;
	FtTcpConnection *pconn=(FtTcpConnection *) GetIOContext(hIORequest, &errorcode);
	assert(pconn!=NULL);

	glog(Log::L_DEBUG, LOGFMT("conn=%08x len=%d"), pconn, len);

	try
	{
		pconn->OnDataArrive(Message,len);
	}
	catch(...) {}

	
    // clean up the memory we received
    gState.free(Message);

	return TCPSTATE_SUCCESS;
}

/// Called when a NewConnection call or a FtNewConnection call has completed and that connection has been
/// accepted or denied by the remote node.
/// Errors will be reported in dwError, including why a connection was denied. If dwError == TCPSTATE_WSA_ERROR
/// then iError has the WSA error code. If the connection was originally started with FtNewConnection, and
/// dwError == TCPSTATE_SUCCESS, iError indicates how the connection was made. Either FTSTATE_CONNECTED_NORMAL
/// or FTSTATE_CONNECTED_FAILOVER. If iError == FTSTATE_CONNECTED_FAILOVER, you can expect a call to
/// pFtNotifyStateChange later when the P-P connection has been re-established. 
/// @param hConnect			a valid handle to the new connection 
/// @param dwError			The TCPSTATE_ error associated with this callback. 
/// @param iError			The WSA network error when dwError is TCPSTATE_WSA_ERROR or the FtConnection
///                         type (see comments above).
/// @return					The user should return TCPSTATE_SUCCESS.
TCPSTATUS  FtTcpConnection::cbNotifyConnect(TCP_CHANDLE hConnect, DWORD dwError, int iError)
{
	glog(Log::L_DEBUG, LOGFMT("cbNotifyConnect conn %x error: %d:%d"), hConnect, dwError, iError);

	if (TCPSTATE_SUCCESS != dwError)
	{
		glog(Log::L_DEBUG, LOGFMT("failed to establish connection"));
		return TCPSTATE_SUCCESS;
	}

	if (FTSTATE_CONNECTED_FAILOVER == iError)
		glog(Log::L_DEBUG, LOGFMT("successfully connected in failover state."));
	else
		glog(Log::L_DEBUG, LOGFMT("successfully connected in normal state."));

	DWORD errorcode;
	FtTcpConnection *pconn=(FtTcpConnection *) GetConnectContext(hConnect,&errorcode);
	assert(pconn!=NULL);

	try
	{
		pconn->_handle = hConnect;

		// refresh the addresses that the connection on
		FtCommAddr_t FtAddr;

		if(FtGetLocalAddr( hConnect, &FtAddr) == TCPSTATE_SUCCESS)
		{
			pconn->_localAddr = FtAddr.szAddr;
		}
		else 
			pconn->_localAddr = "0.0.0.0";

		if(FtGetRemoteAddr(hConnect, &FtAddr )== TCPSTATE_SUCCESS)
		{
			pconn->_remoteAddr = FtAddr.szAddr;
		}
		else 
			pconn->_remoteAddr = "0.0.0.0";

		// refresh the remote typeinst
		if(TCPSTATE_SUCCESS != ::GetRemoteIdent(hConnect, &(pconn->_remoteTypeInst)))
		{
			pconn->_remoteTypeInst.s.dwType =0;
			pconn->_remoteTypeInst.s.dwInst =0;
		}

		pconn->_remotePort = ::GetRemotePort(hConnect, &errorcode);
		
		{
			char buf[64];
			ZQ::common::Log::getTimeStampStr(buf, sizeof(buf),false);
			_establishTimeStr = buf;
		}

		// fire the event
		pconn->OnConnected(dwError,iError);
	}
	catch(...) {}

	return TCPSTATE_SUCCESS;
}

/// Called when a connection is closed.
/// When the connection is a fault tolerant connection, this callback indicates that the user wanted to
/// drop the connection or a failover was in progress and every possible address pair failed to connect.
/// The dwError code lets you know the reason. 
/// @param hConnect			a semi-valid handle to the connection that has closed Only certain functions
///                         are available to this connection once this notification is received by the user.
///                         The user may call the following, anything else returns TCPSTATE_BAD_HANDLE: 
///                         GetContextData GetRemoteIdent, etc. The user may not call Drop, Send, Recv, etc. 
/// @param dwError			The error assciated with this notification. 
/// @param iWsaError		The WSA network error associated with this notification.
/// @return					The function should return TCPSTATE_SUCCESS
TCPSTATUS FtTcpConnection::cbNotifyConnectClose( TCP_CHANDLE hConnect, DWORD dwError, int iWsaError)
{
	glog(Log::L_DEBUG, LOGFMT("cbNotifyConnectClose conn %x error: %d:%d"), hConnect, dwError, iWsaError);
	assert(hConnect!=NULL);

	DWORD errorcode;
	FtTcpConnection *pconn=(FtTcpConnection *)GetConnectContext(hConnect,&errorcode);
	assert(pconn!=NULL);

	try
	{
		pconn->OnClosed(dwError,iWsaError);
//		pconn->_handle = NULL;
	}
	catch(...) {}

	return TCPSTATE_SUCCESS;
}

/// Called when the state of the Fault Tolerant connection has changed. 
/// This call must return relatively quickly. Since this is the thread that is cycling through failover
/// connection attempts.
/// @param hConnect		The handle to this Fault Tolerant connection, it is valid in every way although
///                     TcpStateSend should not be called, and will be returning TCPSTATE_FAILOVER_IN_PROGRESS
///                     when the FtConnection is failing from a failed connection to a new connection. 
/// @param eState		Is the new state, one of the following:@n
///                     FTSTATE_FAILING_OVER -- The underlying TCP connection has been lost. FtComm
///                     is attempting to reconnect. Discontinue IO and wait for state update, or
///                     pNotifyConnectClose if things go south.	Your app is expected to re-start
///                     communication once the connection is re-established. FtComm can not guarantee
///                     which messages were picked up by the other side before the failure ocurred.@n
///                     FTSTATE_FAILING_BACK -- The underlying TCP connection is moving from a FAILOVER
///                     address pair back to the PRIMARY-PRIMARY connection. This is goodness and will not
///                     disrupt comms in any way. This is just a notification that this is taking place.@n
///                     FTSTATE_CONNECTED_FAILOVER -- The underlying TCP connection has been re-established
///                     on one or more backup ports.@n
///                     FTSTATE_CONNECTED_NORMAL -- The underlying TCP connection has been re-established on
///                     the two primary addresses. Also, called after@n
/// @param pLocalAddr	The FtComm address for the local port that has failed, or reconnected
/// @param pRemoteAddr	The FtComm address for the remote port that has failed, or reconnected 
/// @param dwError		The TCPSTATUS indicating why the failure occured
/// @param iWsaError	The WSA specific error when dwError is TCPSTATE_WSA_ERROR 
/// @return				this should be TCPSTATE_SUCCESS.
TCPSTATUS FtTcpConnection::cbNotifyStateChange(TCP_CHANDLE hConnect, FTSTATE eState, const FtCommAddr_t* pLocalAddr, const FtCommAddr_t* pRemoteAddr, DWORD dwError, int iWsaError)
{
	glog(Log::L_DEBUG, LOGFMT("cbNotifyStateChange conn %x error: %d, state: %d"), hConnect, dwError, eState);
	DWORD errorcode;
	FtTcpConnection *pconn=(FtTcpConnection *)GetConnectContext(hConnect, &errorcode);
	assert(pconn!=NULL);
	
	try
	{
		switch(eState)
		{
		case FTSTATE_FAILING_OVER:
			pconn->OnFalingOver(pLocalAddr,pRemoteAddr,dwError,iWsaError);
			break;
		case FTSTATE_FAILING_BACK:
			pconn->OnFallingBack(pLocalAddr,pRemoteAddr,dwError,iWsaError);
			break;
		case FTSTATE_CONNECTED_FAILOVER:

			pconn->OnConnectedFailover(pLocalAddr,pRemoteAddr,dwError,iWsaError);
			break;
		case FTSTATE_CONNECTED_NORMAL:
			pconn->OnConnectedNormal(pLocalAddr,pRemoteAddr,dwError,iWsaError);
			break;
		}
	}
	catch(...) {
		int i = 0;
	}

	return TCPSTATE_SUCCESS;
}

/// Called when a listener has accepted a new connection. 
/// The user has access to the userdata that was sent over the wire, see GetUserData(). The user can accept
/// this connection by returning TCPSTATE_SUCCESS. Otherwise the error code returned will be sent to the
/// remote client and the connection will be denied (torn down). The user can also reset the options on
/// this connection (see the ConnectionOptions structure above, and SetConnectOptions() below). Note: The
/// underlying connection is not officially "CONNECTED" until shortly after this call is returned by the
/// user. All calls to Drop, Send or Recv, will fail with TCPSTATE_NOT_CONNECTED 
/// @param hListener		Handle to the Listener that accepted the connection 
/// @param hConnect			a handle to the new connection to be accepted (not currently CONNECTED) 
/// @param iError			The WSA error associated with this notification. if hConnect is NULL, iError
///                         has the WSA Error code 
/// @return					TCPSTATE_SUCCESS to accept the connection, otherwise connection s denied. 
///                         The socket is valid and good, but any RECV_ALWAYS calls will not begin until
///                         this call returns (because we're not yet CONNECTED). The user should try to
////                        return as quickly as possible. 
TCPSTATUS  FtTcpListener::cbNotifyAccept( TCP_LHANDLE hListener, TCP_CHANDLE hConnect, int iError)
{
	glog(Log::L_DEBUG, LOGFMT("error is %d"),iError);
	assert(hConnect!=NULL);
	assert(hListener!=NULL);

	DWORD errorcode=0;
	FtTcpListener *plistener=(FtTcpListener *)GetListenerContext(hListener,&errorcode);
	assert(plistener!=NULL);

	FtTcpConnection *pConn =NULL;
	plistener->OnAccept(&pConn, hConnect);

	if(pConn == NULL)
	{
		glog(Log::L_DEBUG, "%s:fail to created one",__FUNCTION__);
		return TCPSTATE_USER_REJECT;
	}

	// set the connection handle as the context handle so we get it with 
	// every IO handle later
	SetConnectContext(hConnect, pConn);
	// Set the options so we know when it really gets connectec
	ConnectOptions_t opts;
	memset(&opts, 0, sizeof(ConnectOptions_t) );
	opts.dwFlags = SSCONNECT_EVENT | OPEN_RECV;
	opts.hSSConnectEvent = pConn->_hConnectEvent;
	opts.bOpenRecv = TRUE;
	SetConnectOptions(hConnect, &opts);

	// refresh the addresses that the connection on
	FtCommAddr_t FtAddr;

	if(FtGetLocalAddr( hConnect, &FtAddr) == TCPSTATE_SUCCESS)
	{
		pConn->_localAddr = FtAddr.szAddr;
	}
	else 
		pConn->_localAddr = "0.0.0.0";

	if(FtGetRemoteAddr(hConnect, &FtAddr )== TCPSTATE_SUCCESS)
	{
		pConn->_remoteAddr = FtAddr.szAddr;
	}
	else 
		pConn->_remoteAddr = "0.0.0.0";

	// refresh the remote typeinst
	if(TCPSTATE_SUCCESS != ::GetRemoteIdent(hConnect, &(pConn->_remoteTypeInst)))
	{
		pConn->_remoteTypeInst.s.dwType =0;
		pConn->_remoteTypeInst.s.dwInst =0;
	}

	pConn->_remotePort = ::GetRemotePort(hConnect, &errorcode);

	{
		char buf[64];
		ZQ::common::Log::getTimeStampStr(buf, sizeof(buf),false);
		pConn->_establishTimeStr = buf;
	}

	return TCPSTATE_SUCCESS;
}

/// Called when a listener port is stopped.
/// @param hListener		a semi-valid handle to the listener that has closed Only certain functions are
///                         available to this connection once this notification is received by the user.
///                         The user may call the following, anything else returns 
///                         TCPSTATE_BAD_HANDLE: GetContextData GetRemoteIdent, etc. 
/// @param dwError			The error assciated with this notification. This will be TCPSTATE_SUCCESS,
///                         TCPSTATE_DROPPED, or a WASError
/// @return					The function should return TCPSTATE_SUCCESS
TCPSTATUS  FtTcpListener::cbNotifyListenerClose( TCP_LHANDLE hListener, DWORD dwError)
{
	glog(Log::L_DEBUG, LOGFMT("error is %d"), dwError);
	DWORD errorcode;
	FtTcpListener *plistener=(FtTcpListener *)GetListenerContext(hListener,&errorcode);
	assert(plistener!=NULL);

	plistener->OnClosed(dwError);
//	plistener->_handle = NULL;
	return TCPSTATE_SUCCESS;
}

// -----------------------------
// class FtHostAddress
// -----------------------------
FtHostAddress& FtHostAddress::operator+=(const char *address)
{
	addAddress(address);
	return *this;
}

bool FtHostAddress::addAddress(const char* address)
{
	ZQ::common::InetHostAddress tmpaddr;
	tmpaddr.setAddress(address);
	if (!tmpaddr.isInetAddress())
		return false;

	// append the int _ipaddr with the new address(es) in tmpaddr
	int newAddrsCount = tmpaddr.getAddressCount();
	inetaddr_t* newAddrs = new inetaddr_t[newAddrsCount+_addr_count];
	memcpy(newAddrs, _ipaddr, sizeof(inetaddr_t)*_addr_count);
	int c =0;
	for (int i=0; i< newAddrsCount; i++)
	{
		inetaddr_t addrIterator = tmpaddr.getAddress(i);

		if ((addrIterator.family ==PF_INET && (*this == addrIterator.addr.a)))
			continue;

#ifdef IPV6_ENABLED
		if ((addrIterator.family ==PF_INET6 && (*this == addrIterator.addr.a6)))
			continue;
#endif
		
		if (_bfilterLocal)
		{
			// check if it is a local address
			static InetHostAddress localAddrs = InetHostAddress::getLocalAddress();
			if (localAddrs != addrIterator.addr.a)
				continue; // skip if it is not a local address
		}

		memcpy(&newAddrs[_addr_count+ (c++)], &addrIterator, sizeof(inetaddr_t));
	}

	if (c>0)
	{
		_addr_count += c;
		if (_ipaddr!=NULL)
			delete [] _ipaddr;
		_ipaddr = new inetaddr_t[_addr_count];
		memcpy(_ipaddr, newAddrs, sizeof(inetaddr_t)*_addr_count);
	}

	delete[] newAddrs;

	return (c>0);
}

FtHostAddress::operator const char* ()
{
	_addrstr = getHostAddress();
	for (size_t i=1; i <_addr_count; i++)
		_addrstr += std::string(";") + getHostAddress(i);
	return _addrstr.c_str();
}

void FtHostAddress::clear()
{
	if (_ipaddr!=NULL)
		delete [] _ipaddr;

	_ipaddr = NULL;
	_addr_count =0;
}


// -----------------------------
// class FtTcpConnection
// -----------------------------

FtTcpConnection::FtTcpConnection(TCP_CHANDLE handle, FtTcpListener* pListener)
:_handle(handle), _pListener(pListener),_hConnectEvent(NULL), _remotePort(0)
{
	if (_pListener!=NULL)
		memcpy(&_localTypeInst, _pListener->typeinst(), sizeof(_localTypeInst));

	_hConnectEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	_localPort = pListener->getPort();

	getOptions(&_opts);
}

FtTcpConnection::FtTcpConnection(DWORD type/*=0*/, DWORD inst/*=0*/, ConnectOptions_t* pOptions /*=NULL*/)
:_handle(NULL), _pListener(NULL),_hConnectEvent(NULL), _remotePort(0), _localPort(0)
{
	if (type!=0)
	{
		_localTypeInst.s.dwType = type;
		_localTypeInst.s.dwInst = inst;
	}
	else
		memset(&_localTypeInst, 0, sizeof(_localTypeInst));

	_hConnectEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pOptions !=NULL)
		memcpy(&_opts, pOptions, sizeof(_opts));
	else
	{
		memset(&_opts, 0, sizeof(_opts));
		_opts.dwFlags = (MESSAGE_LENGTH_SIZE /*| FT_FAILOVER_WAIT | FO_RETRY_INTERVAL | FB_RETRY_INTERVAL
						| FT_KEEP_ALIVE_INTERVAL | FT_KEEP_ALIVE_RESPONSE_INTERVAL*/| OPEN_RECV);

		_opts.byteMessageLengthSize = 4;
		_opts.bOpenRecv = TRUE;
	}
}

FtTcpConnection::~FtTcpConnection()
{
	if (NULL ==_hConnectEvent)
		::CloseHandle(_hConnectEvent);
}


bool FtTcpConnection::connect(WORD wPort,const FtHostAddress& LocalAddress, const FtHostAddress& RemoteAddress, int TOS/*=0*/, BYTE* pUserData/*=NULL*/, DWORD dwDataLen/*=0*/)
{
	if (!gState.isValid())
	{
		glog(Log::L_DEBUG, "State not initialized");
		return false;
	}

	if (isConnected())
	{
		glog(Log::L_DEBUG, LOGFMT("connect() is called after the connection has been established"));
		return false;
	}

    FTQOC qoc;
    memset(&qoc, 0, sizeof(FTQOC));
    qoc.eType = QOCTYPE_TOS;
	qoc.ftTOS.tos = (TOS <=0 || TOS>255) ? DEFAULT_TOS : TOS;

	TcpStateCallBacks_t cbFunc;
	memset(&cbFunc, 0, sizeof(cbFunc));
	cbFunc.pNotifyReceived =       FtTcpConnection::cbNotifyReceived;
	cbFunc.pNotifyConnect =        FtTcpConnection::cbNotifyConnect;
	cbFunc.pNotifyConnectClose =   FtTcpConnection::cbNotifyConnectClose;
	cbFunc.pFtNotifyStateChange =  FtTcpConnection::cbNotifyStateChange;


	// make up the local FtCommAddrList
	FtCommAddrList_t LocalAddrs;
    LocalAddrs.dwAddrCount = 0;
    LocalAddrs.pAddrs = new FtCommAddr_t[2];
    memset(LocalAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);

	int addrcount = LocalAddress.getAddressCount();
	ZQ::common::InetHostAddress allLocalAddrs = ZQ::common::InetHostAddress::getLocalAddress();
	for(int i=0; i< addrcount && LocalAddrs.dwAddrCount<2; i++)
	{
		if (LocalAddress.family(i) !=PF_INET)
			continue; // MtTCPCommon only support IPv4 currently

		// check if it is a local address
		ZQ::common::InetAddress::inetaddr_t tmp = LocalAddress.getAddress(i);
		if (allLocalAddrs == tmp.addr.a)
			strcpy(LocalAddrs.pAddrs[LocalAddrs.dwAddrCount++].szAddr, LocalAddress.getHostAddress(i));
	}

	if(LocalAddrs.dwAddrCount<=0)
	{
		glog(Log::L_ERROR, LOGFMT("illegal local address to connect"));
		delete [] LocalAddrs.pAddrs;
		return false;
	}

	// make up the remote FtCommAddrList
	FtCommAddrList_t RemoteAddrs;
    RemoteAddrs.dwAddrCount = 0;
    RemoteAddrs.pAddrs = new FtCommAddr_t[2];
    memset(RemoteAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);

	addrcount = RemoteAddress.getAddressCount();

	for(int j=0; j< addrcount && RemoteAddrs.dwAddrCount<2; j++)//for vc 6
	{
		if (RemoteAddress.family(j) !=PF_INET)
			continue; // MtTCPCommon only support IPv4 currently

		strcpy(RemoteAddrs.pAddrs[RemoteAddrs.dwAddrCount++].szAddr, RemoteAddress.getHostAddress(j));
	}

	if(RemoteAddrs.dwAddrCount<=0)
	{
		glog(Log::L_ERROR, LOGFMT("illegal remote address to connect"));
		delete [] LocalAddrs.pAddrs;
		delete [] RemoteAddrs.pAddrs;
		return false;
	}

	memset(&_opts, 0, sizeof(_opts) );
	_opts.dwFlags =  OPEN_RECV|MESSAGE_LENGTH_SIZE;
	_opts.bOpenRecv = TRUE;
	_opts.byteMessageLengthSize=4;

	_opts.dwFlags |= // FT_FAILOVER_WAIT | FO_RETRY_INTERVAL | FB_RETRY_INTERVAL |
                 FT_KEEP_ALIVE_INTERVAL | FT_KEEP_ALIVE_RESPONSE_INTERVAL;

    _opts.dwFtFailoverWait =                 2000;    //2s
    _opts.dwFoRetryInterval =                10000;    //10s
    _opts.dwFbRetryInterval =                20000;    //20s
    _opts.dwFtKeepAliveInterval =            5000;   //45s
    _opts.dwFtKeepAliveResponseInterval =    2000 ;  //15s

	glog(Log::L_DEBUG, LOGFMT("connect %s,%s to %s,%s:%d"), LocalAddrs.pAddrs[0].szAddr, LocalAddrs.pAddrs[1].szAddr, RemoteAddrs.pAddrs[0].szAddr, RemoteAddrs.pAddrs[1].szAddr, wPort);

	TCPSTATUS result = FtNewConnection(gState.handle(), wPort, &LocalAddrs, &RemoteAddrs,
	                       _localTypeInst, pUserData, dwDataLen, this, &_opts,
						   &cbFunc,&qoc);

    delete [] LocalAddrs.pAddrs;
    delete [] RemoteAddrs.pAddrs;

	if (result == TCPSTATE_SUCCESS)
		return true;

	if (result == TCPSTATE_BIND_FAIL)
		glog(Log::L_ERROR, LOGFMT("bind failed"));
	else
		glog(Log::L_ERROR, LOGFMT("failed with decimal code %d"), result);

	return false;
}

void FtTcpConnection::close(DWORD reason)
{
	if(!isConnected())
		return;

	TCPSTATUS result = DropConnection(_handle, reason);

	if(result!=TCPSTATE_SUCCESS)
		glog(Log::L_DEBUG, "%s:DropConnection error, TCPSTATUS=%u",__FUNCTION__,result);
//  else _handle=NULL; // leave for cbClosed() to put NULL
}

bool FtTcpConnection::send(void* msg, int len)
{
	if(!isConnected() || msg==NULL || len<=0)
		return false;

	BYTE* sendbuf = gState.malloc(len+1);
	memset(sendbuf, 0, len+1);

	if (sendbuf == NULL)
		return false;

	memcpy(sendbuf, msg, len);

	TCPSTATUS result=TcpStateSend(_handle, sendbuf, len, 0, this);

	return (result ==TCPSTATE_SUCCESS);
}

bool FtTcpConnection::send(PIOBUF pIOBufs, DWORD dwNumBufs)
{
	if(!isConnected())
		return false;

	TCPSTATUS result=TcpStateSendMb(_handle,pIOBufs,dwNumBufs,0,this);
	if(result!=TCPSTATE_SUCCESS)
	{
		glog(Log::L_DEBUG, "%s:TcpStateSendMb error, TCPSTATUS=%u",__FUNCTION__,result);
		return false;
	}
	else
		return true;
}

bool FtTcpConnection::getOptions(pConnectOptions_t pOptions)
{
	if(!isConnected())
		return false;

	TCPSTATUS result=GetConnectOptions(_handle,pOptions);
	if(result!=TCPSTATE_SUCCESS)
	{
		glog(Log::L_DEBUG, "%s:GetConnectOptions error, return %u",__FUNCTION__,result);
		return false;
	}
	else
		return true;
}

bool FtTcpConnection::setOptions(pConnectOptions_t pOptions)
{
	if(!isConnected())
		return false;

	TCPSTATUS result= SetConnectOptions(_handle,pOptions);
	if(result!=TCPSTATE_SUCCESS)
	{
		glog(Log::L_DEBUG, "%s:SetConnectOptions error, return %u",__FUNCTION__,result);
		return false;
	}
	else
		return true;
}

bool FtTcpConnection::setLocalIdent(TYPEINST &LocalIdent)
{
	if(!isConnected())
	{
		memcpy(&_localTypeInst, &LocalIdent, sizeof(_localTypeInst));
		return true;
	}

	TCPSTATUS result= SetLocalIdent(_handle,&LocalIdent);
	if(result!=TCPSTATE_SUCCESS)
	{
		glog(Log::L_DEBUG, "%s:SetLocalIdent error, return %u",__FUNCTION__,result);
		return false;
	}

	memcpy(&_localTypeInst, &LocalIdent, sizeof(_localTypeInst));

	return true;
}

const WORD FtTcpConnection::getRemotePort()
{
	if(!isConnected())
		return false;

	DWORD errorcode=0;
	return ::GetRemotePort(_handle,&errorcode);
}

// -----------------------------
// class FtTcpListener
// -----------------------------

/// Default constructor
FtTcpListener::FtTcpListener(DWORD type, DWORD inst)
:_handle(NULL)
{
	_localTypeInst.s.dwType = type;
	_localTypeInst.s.dwInst = inst;

	memset(&_opts, 0, sizeof(ConnectOptions_t) );
	_opts.dwFlags =  OPEN_RECV|MESSAGE_LENGTH_SIZE;
	_opts.bOpenRecv = TRUE;
	_opts.byteMessageLengthSize=6;
}


bool FtTcpListener::listen(const WORD portnum, const FtHostAddress &LocalAddress, const int TOS /*=0*/)
{
	if (!gState.isValid())
	{
		glog(Log::L_DEBUG, "State not initialized");
		return false;
	}

	// make up the local FtCommAddrList
	FtCommAddrList_t LocalAddrs;
    LocalAddrs.dwAddrCount = 0;
    LocalAddrs.pAddrs = new FtCommAddr_t[2];
    memset(LocalAddrs.pAddrs, 0, sizeof(FtCommAddr_t)*2);

	_bindAddrs = LocalAddress;

	int addrcount = LocalAddress.getAddressCount();
	ZQ::common::InetHostAddress allLocalAddrs = ZQ::common::InetHostAddress::getLocalAddress();
	for(int i=0; i< addrcount && LocalAddrs.dwAddrCount<2; i++)
	{
		if (LocalAddress.family(i) !=PF_INET)
			continue; // MtTCPCommon only support IPv4 currently

		// check if it is a local address
		if (LocalAddress.filteredLocal() || (allLocalAddrs == LocalAddress.getAddress(i).addr.a))
			strcpy(LocalAddrs.pAddrs[LocalAddrs.dwAddrCount++].szAddr, LocalAddress.getHostAddress(i));
	}

	if (LocalAddrs.dwAddrCount<=0)
	{
		glog(Log::L_ERROR, LOGFMT("illegal local address to listen"));
		delete [] LocalAddrs.pAddrs;
		return false;
	}

	FTQOC qoc;
    memset(&qoc, 0, sizeof(FTQOC));
    qoc.eType = QOCTYPE_TOS;
	qoc.ftTOS.tos = (TOS <=0 || TOS>255) ? DEFAULT_TOS : TOS;

	DWORD dwError=0;
	TcpStateCallBacks_t cbFunc;
	memset(&cbFunc, 0, sizeof(cbFunc));
	cbFunc.pNotifyAccept =         FtTcpListener::cbNotifyAccept;
	cbFunc.pNotifyListenerClose =  FtTcpListener::cbNotifyListenerClose;

	cbFunc.pNotifyReceived =       FtTcpConnection::cbNotifyReceived;
	cbFunc.pNotifyConnect =        FtTcpConnection::cbNotifyConnect;
	cbFunc.pNotifyConnectClose =   FtTcpConnection::cbNotifyConnectClose;
	cbFunc.pFtNotifyStateChange =  FtTcpConnection::cbNotifyStateChange;

	_handle = FtStartListener(gState.handle(), portnum, &LocalAddrs,
		               _localTypeInst, this, &_opts, &cbFunc,
					   &qoc, &dwError);

	delete [] LocalAddrs.pAddrs;

	if(_handle==NULL)
		glog(Log::L_ERROR, LOGFMT("errorcode=%u"),dwError);

	return isAlive();
}

void FtTcpListener::close(DWORD reason)
{
	if(!isAlive())
		return;

	TCPSTATUS result;
	__try{
		result =DropListener(_handle,reason);
	}
	__finally {}
	
	if(result!=TCPSTATE_SUCCESS)
		glog(Log::L_DEBUG, "%s:DropListener error, TCPSTATUS=%u",__FUNCTION__,result);
//	else _handle=NULL; // leave for cbClosed() to put NULL
}


WORD FtTcpListener::getPort()
{
	if(!isAlive())
		return 0;

	DWORD errorcode;
	return GetListenerPort(_handle,&errorcode);
}


} // namespace common
} // namespace ZQ

