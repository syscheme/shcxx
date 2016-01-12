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
// $Log: /ZQProjs/Common/TCPSocket.cpp $
// 
// 41    11/26/15 2:49p Hui.shao
// 
// 40    3/19/15 9:55a Hui.shao
// 
// 39    1/22/15 5:06p Hui.shao
// tested 1million multibulks
// 
// 38    1/13/15 11:36a Hui.shao
// reduct the select() error log printing
// 
// 37    12/26/14 10:56a Hui.shao
// 
// 34    12/31/13 5:36p Hui.shao
// 
// 33    12/31/13 5:31p Hui.shao
// 
// 32    9/22/11 3:16p Hui.shao
// merged from V1.15
// 
// 33    9/13/11 4:48p Hui.shao
// 
// 32    8/18/11 5:12p Hui.shao
// 
// 31    3/02/11 7:06p Hui.shao
// 
// 30    3/02/11 3:54p Hui.shao
// 
// 28    2/14/11 12:59p Fei.huang
// 
// 27    2/14/11 11:44a Haoyuan.lu
// 
// 26    2/12/11 11:10a Haoyuan.lu
// add lock for flag
// 
// 25    1/28/11 11:08a Hui.shao
// 
// 24    1/28/11 10:41a Haoyuan.lu
// 
// 23    1/27/11 7:35p Hui.shao
// 
// 22    1/27/11 6:58p Hui.shao
// debug the dummy connection
// 
// 21    1/27/11 3:13p Hui.shao
// 
// 20    1/26/11 11:34a Hui.shao
// 
// 19    1/25/11 5:38p Hui.shao
// query for tcp thread pool status
// 
// 18    1/25/11 11:31a Haoyuan.lu
// 
// 17    1/24/11 6:39p Fei.huang
// 
// 16    1/19/11 1:03p Haoyuan.lu
// 
// 15    1/14/11 7:24p Hui.shao
// moved Ponter into cpp
// 
// 14    1/14/11 7:05p Haoyuan.lu
// 
// 13    1/14/11 11:32a Hui.shao
// defined TCPServer, replaced the dummy socket from UDP to TCP conn at
// loopback interface
// 
// 12    1/11/11 5:04p Haoyuan.lu
// 
// 11    1/11/11 11:29a Haoyuan.lu
// fix handle leak, socket
// 
// 10    1/06/11 5:54p Fei.huang
// 
// 9     12/31/10 6:41p Fei.huang
// + merged to linux
// 
// 8     10-12-06 17:00 Xiaohui.chai
// 
// 7     10-11-24 18:16 Hui.shao
// made the watchdog instance per need
// 
// 6     10-11-24 17:06 Hui.shao
// 
// 5     10-11-24 17:06 Hui.shao
// 
// 4     10-11-24 17:02 Hui.shao
// 
// 3     10-11-24 15:55 Hui.shao
// for the stuck on TcpWatchDog's quit
// 
// 2     10-11-24 15:46 Haoyuan.lu
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 17    10-11-10 10:24 Haoyuan.lu
// 
// 16    10-11-02 17:18 Hui.shao
// moved the pending flags into TcpSocket
// 
// 15    10-10-29 13:16 Haoyuan.lu
// 
// 14    10-10-28 12:04 Hui.shao
// start the TcpSockWatchdog by default
// 
// 13    10-10-22 13:46 Hui.shao
// take _timeout for connection idle check after the connected
// 
// 12    10-10-21 15:34 Hui.shao
// simplize with internal background threadpool, initial with size=3
// 
// 11    10-10-20 15:42 Hui.shao
// 
// 10    10-10-20 14:31 Hui.shao
// added TCPSocket, incompleted
// 
// 9     10-10-18 20:34 Hui.shao
// impl the TCP connection
// ===========================================================================

#include "TCPSocket.h"
#include "NativeThreadPool.h"
#include "TimeUtil.h"
#include "SystemUtils.h"
#include "Pointer.h"

#ifdef ZQ_OS_MSWIN
#undef max
#endif

namespace ZQ {
namespace common {

#ifndef MAPSET
#  define MAPSET(_MAPTYPE, _MAP, _KEY, _VAL) if (_MAP.end() ==_MAP.find(_KEY)) _MAP.insert(_MAPTYPE::value_type(_KEY, _VAL)); else _MAP[_KEY] = _VAL
#endif // MAPSET

// -----------------------------
// class SocketEventDispatcher
// -----------------------------
class TcpSocketWatchDog;

class SocketEventDispatcher : public ThreadRequest
{
public:
	SocketEventDispatcher(TcpSocketWatchDog& wd, TCPSocket& so);

protected:
	TCPSocket& _so;
	TcpSocketWatchDog& _wd;

	virtual int run();
	void final(int retcode =0, bool bCancelled =false)
	{
		delete this;
	}
};

// -----------------------------
// class DummyTCPSvrConn
// -----------------------------
class DummyTCPSvrConn : public TCPSocket, virtual public SharedObject
{
public:
	typedef Pointer < DummyTCPSvrConn > Ptr;

	DummyTCPSvrConn(TCPServer& server, TCPSocket& source): TCPSocket(source), _server(server) { setTimeout(0); }
	DummyTCPSvrConn(TCPServer& server): TCPSocket(), _server(server) { setTimeout(0); }

protected:
	virtual void OnConnected()
	{
		setTimeout(0);
	}

	virtual void OnError() { disconnect(); };
	virtual void OnDataArrived()
	{
		char buf[128];
		struct sockaddr_in fromAddress;
		socklen_t addressSize = sizeof(fromAddress);
		int ret = recv(_so, (char*) buf, sizeof(buf),	0);

		if (ret <= 0)
			disconnect();
	}

	virtual void OnTimeout() {};

	TCPServer& _server;
};

class DummyServer : public TCPServer, virtual public SharedObject
{
public:
	typedef Pointer < DummyServer > Ptr;
	DummyServer(const InetAddress &bind, tpport_t port) : TCPServer(bind, port) {}

	virtual TCPSocket* OnAccepted(TCPSocket& preacceptedConn)
	{
		_conn = new DummyTCPSvrConn(*this, preacceptedConn);
		return _conn.get();
	};

	DummyTCPSvrConn::Ptr _conn;
};

class DummyClient : public TCPClient, virtual public SharedObject
{
public:
	typedef Pointer < DummyClient > Ptr;
	DummyClient(const InetAddress &bind, tpport_t port) : TCPClient(bind, port) {}
	void OnDataArrived()
	{
		char buf[128];
		struct sockaddr_in fromAddress;
		socklen_t addressSize = sizeof(fromAddress);
		recvfrom(_so, (char*) buf, sizeof(buf), 0, (struct sockaddr*)&fromAddress, &addressSize);
	}

	virtual void OnConnected()
	{
		setTimeout(0);
	}

	void setConnected() {
		Socket::_state = stConnected;
	}
};

// -----------------------------
// class TcpSocketWatchDog
// -----------------------------
class TcpSocketWatchDog : public ThreadRequest, virtual public SharedObject
{
	friend class TCPSocket;
	friend class SocketEventDispatcher;
public:
	typedef Pointer < TcpSocketWatchDog > Ptr;

	TcpSocketWatchDog()
		: ThreadRequest(_pool), _pool(3), _bQuit(false), _tcpServer(NULL), _dummyClient(NULL), _pDummyConn(NULL)
	{
        SharedObject::__setNoDelete(true); // manual delete the object
//		_dummySo = socket(AF_INET, SOCK_DGRAM, 0);
//		start(); // start the watchdog thread instantly
	}

	bool dummyConnect()
	{
		if(!_tcpServer)
			_tcpServer = new DummyServer(InetAddress("localhost"), 0);
		if(!_dummyClient)
			_dummyClient = new DummyClient(InetAddress("localhost"), 0);
		if (_tcpServer->listen(2))
		{
			tpport_t svrport;
			_tcpServer->getLocal(&svrport);
			_dummyClient->setPeer(InetAddress("localhost"), svrport);
			_dummyClient->connect(1000);
		}

		return _dummyClient->isConnected();
	}

	virtual ~TcpSocketWatchDog()
	{
		_pool.clearRequests();
		quit();
		SYS::sleep(1);
		_pool.stop();
	}

	SOCKET	getDummyClientSO()
	{
		if (_dummyClient)
		{
			return _dummyClient->get();
		}
		else
			return INVALID_SOCKET;
	}

	SOCKET	getDummyConnSO()
	{
		if (_pDummyConn)
		{
			return _pDummyConn->get();
		}
		else
			return INVALID_SOCKET;
	}


protected:
	NativeThreadPool _pool;
	bool            _bQuit;

	typedef std::map<SOCKET, TCPSocket* > SocketMap;
	SocketMap		_socketMap;
	Mutex		    _lock;

	DummyServer::Ptr  _tcpServer;
	DummyClient::Ptr  _dummyClient;
	DummyTCPSvrConn::Ptr _pDummyConn;


public:

	void watch(TCPSocket& tso, bool bShoudlWakeup = true)
	{
		MutexGuard g(_lock);
		if (INVALID_SOCKET == tso.get())
			return;

		MAPSET(SocketMap, _socketMap, tso.get(), &tso);
		if(bShoudlWakeup) {
			wakeup();
		}
	}

	void unwatch(TCPSocket& tso)
	{
		{
			MutexGuard g(_lock);

			SOCKET so = tso.get();
			_socketMap.erase(so);
			//		FD_CLR((uint)so, &_fdSet);

			std::vector<SOCKET> socketToRemove;
			for (SocketMap::iterator it = _socketMap.begin(); it != _socketMap.end(); it++)
			{
				if (it->second == &tso)
					socketToRemove.push_back(it->first);
			}

			for (size_t i=0; i < socketToRemove.size(); i++)
			{
				_socketMap.erase(socketToRemove[i]);
			}
		}

		wakeup();

/*
// wait till the this watchdog is truly quit
		if (_bQuit && threadId() >0)
			::Sleep(1);

		if (_bQuit)
		{
			_pool.clearRequests();
			_pool.stop();
		}
*/
	}

	void wakeup()
	{
		if(_dummyClient && _dummyClient->isConnected() )
			_dummyClient->send("Hi", 2);
	}

	void quit()
	{
		_bQuit = true;
		_dummyClient->disconnect();
/*
	if(_dummySo != INVALID_SOCKET) {
#ifdef ZQ_OS_MSWIN
			closesocket(_dummySo);
#else
			close(_dummySo);
#endif
		}
		_dummySo = INVALID_SOCKET;
*/
	}

	TCPSocket* find(SOCKET so)
	{
		MutexGuard g(_lock);
		SocketMap::iterator result = _socketMap.find(so);
		if (result == _socketMap.end())
			return NULL;

		return  result->second;
	}

	void OnError()
	{
		//TODO: find another port of localhost to listen again, then re-connect the dummy client
	}

	TCPSocket* OnAccepted(TCPSocket& preacceptedConn)
	{
		_pDummyConn = new DummyTCPSvrConn(*_tcpServer, preacceptedConn);
		//_tcpServer->stopListening();
		return NULL;//_pDummyConn.get();
	};

protected:

	int run()
	{
		dummyConnect();
		int32 nextSleep = 100; // initial with a small number for the first round
		int64  stampNow = TimeUtil::now();

//		char buf[128];
//		struct sockaddr_in fromAddress;
//		socklen_t addressSize = sizeof(fromAddress);

		fd_set fdread, fdwrite, fderr; // make a copy
		FD_ZERO(&fdread);  	FD_ZERO(&fdwrite); 	FD_ZERO(&fderr);
		int lastErrcode =0;
		int cContinuousErr =0;

		while(!_bQuit)
		{
			if (!_dummyClient || (!_dummyClient->isConnected() && Socket::stConnecting != _dummyClient->state()))
				dummyConnect();

			struct timeval timeout;
			timeout.tv_sec	= nextSleep / 1000;
			timeout.tv_usec	= (nextSleep % 1000) *1000;

			SOCKET maxFd = 1; // _dummySo +1;
			std::string strfdr, strfdw, strfde;
			{
				char buf[16];
				MutexGuard g(_lock);
				for (SocketMap::iterator it = _socketMap.begin(); it != _socketMap.end(); it++)
				{
					if (INVALID_SOCKET == it->first)
						continue;

					snprintf(buf, sizeof(buf)-2, "%d,", it->first);

					if (0 == (it->second->_flagsPending & TCPSocket::FLG_READ))
					{
						FD_SET(it->first, &fdread);
						strfdr+=buf;
					}

					if (0 == (it->second->_flagsPending & TCPSocket::FLG_WRITE))
					{
						FD_SET(it->first, &fdwrite);
						strfdw+=buf;
					}

					if (0 == (it->second->_flagsPending & TCPSocket::FLG_ERROR))
					{
						FD_SET(it->first, &fderr);
						strfde+=buf;
					}

					maxFd = std::max(maxFd, it->first+1);
				}
			}

			// force to include the dummy connection in the select list
			if (_dummyClient && _dummyClient->isConnected())
			{
				//FD_SET(_dummyClient->get(), &fdread);

				//if (_pDummyConn)
				//	FD_SET(_pDummyConn->get(), &fdread);
			}
			else timeout.tv_sec =1;

			if (_bQuit)
				break;

//			char buf[64];
//			printf("Before select [%d %d %d] %s\n", fdread.fd_count, fdwrite.fd_count, fderr.fd_count, TimeUtil::TimeToUTC(now(), buf, sizeof(buf)-2, true));
			int rc = select(maxFd, &fdread, &fdwrite, &fderr, &timeout);
//			printf("After select [%d %d %d] %s\n", fdread.fd_count, fdwrite.fd_count, fderr.fd_count, TimeUtil::TimeToUTC(now(), buf, sizeof(buf)-2, true));
			if (_bQuit)
				break;

			stampNow = TimeUtil::now();
			nextSleep = 10000; // start taking a fair number 10sec
			if (rc < 0)
			{
				try {
					int errcode = SYS::getLastErr(SYS::SOCK);
					if (errcode != lastErrcode || cContinuousErr > (10*24*3600*6))
						cContinuousErr =0;
					lastErrcode = errcode;
					if (cContinuousErr++ < 3)
						glog(Log::L_ERROR, CLOGFMT(TcpSocketWatchDog, "max[%d]/FD_SETSIZE[%d] select() %dms error[%d]: fdread[%s], fdwrite[%s], fderr[%s]"), maxFd, FD_SETSIZE, nextSleep, errcode, strfdr.c_str(), strfdw.c_str(), strfde.c_str());
				}
				catch(...) {}

				// rebuild fds
				FD_ZERO(&fdread);  	FD_ZERO(&fdwrite); 	FD_ZERO(&fderr);

				continue;
			}
			
			cContinuousErr =0; // reset the err counting

			// rc >=0 here

			// read the dummy connection directly in this thread
//			if (_dummyClient && FD_ISSET(_dummyClient->get(), &fdread))
//				recvfrom(_dummyClient->get(), (char*) buf, sizeof(buf), 0, (struct sockaddr*)&fromAddress, &addressSize);

//			if (_pDummyConn && FD_ISSET(_pDummyConn->get(), &fdread))
//				recvfrom(_pDummyConn->get(), (char*) buf, sizeof(buf), 0, (struct sockaddr*)&fromAddress, &addressSize);

			{
				MutexGuard g(_lock);
				for (SocketMap::iterator it = _socketMap.begin(); !_bQuit && it != _socketMap.end(); it++)
				{
					try {
						// if (it->first == _dummySo || NULL == it->second || it->first <0)
						if (NULL == it->second || it->first <0)
							continue;

						int flags =0;
						int32 timeLeft = 24*60*60*1000; // initialize with a big number
						if (it->second->_timeout >0)
						{
							timeLeft = (int32) (it->second->_lastUpdated + it->second->_timeout - stampNow);
							if (nextSleep > timeLeft && timeLeft >0)
								nextSleep = timeLeft;
						}

						if (timeLeft<=0)
							flags |= TCPSocket::FLG_TIMEOUT; // timed out

						if (rc >0)
						{
							if (!(it->second->_flagsPending & TCPSocket::FLG_READ) && FD_ISSET(it->first, &fdread))
								flags |= TCPSocket::FLG_READ;

							if (FD_ISSET(it->first, &fdwrite))
								flags |= TCPSocket::FLG_WRITE;

							if (FD_ISSET(it->first, &fderr))
								flags |= TCPSocket::FLG_ERROR;
						}

						it->second->_flagsPending |= flags;
						if (it->second->_flagsPending)
							(new SocketEventDispatcher(*this, *it->second))->start();
					}
					catch(...) 
					{
						glog(Log::L_ERROR, CLOGFMT(TcpSocketWatchDog, "SocketEventDispatcher SO[%d] FLAG[%d] error occured, [%d, %d]"), it->first, it->second->_flagsPending, _dummyClient->get(), _pDummyConn->get());
						// reset flags
						it->second->_flagsPending = 0;
					}
				}
			}

			SYS::sleep(1); // yield the CPU

			// rebuild fds
			FD_ZERO(&fdread);  	FD_ZERO(&fdwrite); 	FD_ZERO(&fderr);
		}

		try {
			glog(Log::L_INFO, CLOGFMT(TcpSocketWatchDog, "quits"));
		}
		catch(...) {}

		return 0;
	}
};

// -----------------------------
// class SocketEventDispatcher
// -----------------------------
SocketEventDispatcher::SocketEventDispatcher(TcpSocketWatchDog& wd, TCPSocket& so)
: ThreadRequest(wd._pool), _so(so), _wd(wd)
{
	ThreadRequest::setPriority(DEFAULT_REQUEST_PRIO >>1); // make the priority of this ThreadRequest higher than normal
}

#define RESET_SOFLAG(FLAGS)  {	ZQ::common::MutexGuard g(TCPSocket::_gWatchDog->_lock); _so._flagsPending &= ~FLAGS; _so._lastUpdated = stampNow; }

int SocketEventDispatcher::run()
{
	bool bNeedWakeup = false;
	int64 stampNow = TimeUtil::now();

	if (!_wd._bQuit && (_so._flagsPending & TCPSocket::FLG_WRITE))
	{
		RESET_SOFLAG(TCPSocket::FLG_WRITE);

		// writeable
		try {
			if (Socket::stConnecting == _so._state)
			{
				// double check if the connection is good
				// The connection succeeded.  If the connection came about from an attempt to set up RTSP-over-HTTP, finish this now:
				//		int err = 0;
				//		int len = sizeof(err);
				if (_so.checkSoErr() != 0)
					_so._flagsPending |= TCPSocket::FLG_ERROR; // error occurred
				else
				{
					_so.setKeepAlive(true);
					_so.setBlock(_so._bBlockEnable ? TCPSocket::BLOCK : TCPSocket::NONBLOCK); // restore the old BLOCK setting
					_so._state = Socket::stConnected;
					_so.updateConnDesc();
					_so.OnConnected();

					bNeedWakeup = true;
				}
			}
		}
		catch(...) {}
	}

	if (!_wd._bQuit && (_so._flagsPending & TCPSocket::FLG_READ))
	{
		RESET_SOFLAG(TCPSocket::FLG_READ);

		// readable
		try {
			_so.OnDataArrived();

		}
		catch(...) {}

		if (_so.get() != TCPSocket::_gWatchDog->getDummyClientSO() && _so.get() != TCPSocket::_gWatchDog->getDummyConnSO())
			bNeedWakeup = true;
	}

	if (!_wd._bQuit && (_so._flagsPending & TCPSocket::FLG_ERROR))
	{
		RESET_SOFLAG(TCPSocket::FLG_ERROR);

		try {
			_so.OnError();
		}
		catch(...) {}

		if (_so.get() != TCPSocket::_gWatchDog->getDummyClientSO() && _so.get() != TCPSocket::_gWatchDog->getDummyConnSO())
			bNeedWakeup = true;
	}

	if (!_wd._bQuit && _so._flagsPending & TCPSocket::FLG_TIMEOUT) // connection timeout
	{
		RESET_SOFLAG(TCPSocket::FLG_TIMEOUT);

		try {
			_so.OnTimeout();
		}
		catch(...) {}

		bNeedWakeup = true;
	}

	if (bNeedWakeup && TCPSocket::_gWatchDog)
		TCPSocket::_gWatchDog->wakeup();

	return 0;
}

#undef RESET_SOFLAG

// -----------------------------
// class TCPSocket
// -----------------------------
// static members
TcpSocketWatchDog* TCPSocket::_gWatchDog = NULL;
Mutex TCPSocket::_lockWatchDog;
uint8 TCPSocket::_poolSize =3;

void TCPSocket::refWatchDog()
{
	{
		MutexGuard g(_lockWatchDog);
		if (NULL == _gWatchDog) 
		{
			_gWatchDog = new TcpSocketWatchDog();
			_gWatchDog->start();
		}

		if (NULL != _gWatchDog)
			_gWatchDog->__incRef();
	}

	resizeThreadPool(_poolSize);
}

void TCPSocket::unrefWatchDog()
{
	MutexGuard g(_lockWatchDog);
	if (NULL == _gWatchDog) 
		return;

	_gWatchDog->__decRef();
	if (_gWatchDog->__getRef() <=0)
	{
		delete _gWatchDog;
		_gWatchDog = NULL;
	}
}

bool TCPSocket::getThreadPoolStatus(int& poolSize, int& activeCount, int& pendingSize)
{
	MutexGuard g(_lockWatchDog);
	if (NULL == _gWatchDog) 
	{
		poolSize = 0;
		return false;
	}

	poolSize = _gWatchDog->_pool.size();
	pendingSize = _gWatchDog->_pool.pendingRequestSize();
	activeCount = _gWatchDog->_pool.activeCount();
	return true;
}

void TCPSocket::resizeThreadPool(int newSize)
{
	MutexGuard g(_lockWatchDog);
	_poolSize = newSize >3 ? newSize:3;
	if (NULL == _gWatchDog)
		return;

	_gWatchDog->_pool.resize(_poolSize);
}

TCPSocket::TCPSocket(void)
: Socket(PF_INET, SOCK_STREAM, 0), _bBlockEnable(true), _timeout(0), _flagsPending(0)// , _bRecvdDataInPending(false)
{
	_lastUpdated = TimeUtil::now();
	setBlock(NONBLOCK);
	refWatchDog();
}

TCPSocket::TCPSocket(const InetAddress &host, tpport_t port)
: Socket(PF_INET, SOCK_STREAM, 0), _bBlockEnable(true),  _timeout(0), _flagsPending(0)
{
	_lastUpdated = TimeUtil::now();
	setBlock(NONBLOCK);
	setPeer(host, port);
	refWatchDog();
	bind();
}

// the copier
TCPSocket::TCPSocket(const TCPSocket &source)
: Socket(source), _bBlockEnable(source._bBlockEnable), _timeout(source._timeout), _lastUpdated(source._lastUpdated), _flagsPending(source._flagsPending) 
{
	memcpy(&_peer, &source._peer, sizeof(_peer));
	updateConnDesc();
	refWatchDog();
}

TCPSocket::TCPSocket(const Socket &source)
: Socket(source), _bBlockEnable(true), _timeout(0), _flagsPending(0)
{
	_lastUpdated = TimeUtil::now();
	updateConnDesc();
	refWatchDog();
}

TCPSocket::TCPSocket(int so )
:Socket(so),_bBlockEnable(true),_timeout(0),_flagsPending(0){
}

void TCPSocket::bind()
{
#ifdef	SOCK_REUSEADDR
	int opt = 1;
	setsockopt(_so, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, (socklen_t)sizeof(opt));
#endif

	if(_peer.family == PF_INET)
	{
		if (::bind(_so, (struct sockaddr *)&(_peer.sa.a), sizeof(_peer.sa.a))!=0)
		{
			endSocket();
			error(errBindingFailed);
			return;
		}
	}

#ifdef IPV6_ENABLED
	if(_peer.family == PF_INET6)
	{
		int ret = ::bind(_so, (struct sockaddr *)&(_peer.sa.a6), sizeof(_peer.sa.a6));
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

/// Destructor
TCPSocket::~TCPSocket()
{
	if (NULL != _gWatchDog)
		_gWatchDog->unwatch(*this);
	endSocket();
	unrefWatchDog();
}

int TCPSocket::checkSoErr()
{
	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(_so, SOL_SOCKET, SO_ERROR, (char*)&err, &len)< 0 )
		return -1;
	return err;
}

void TCPSocket::setPeer(const InetAddress &ia, tpport_t port)
{
	disconnect();

	memset(&_peer, 0, sizeof(_peer));
	InetAddress::inetaddr_t addr = getaddress(ia); 

	_peer.family = addr.family;
	if (_peer.family == PF_INET)
	{
		_peer.sa.a.sin_family = _peer.family;
		_peer.sa.a.sin_addr = addr.addr.a;
		_peer.sa.a.sin_port = htons(port);

		return;
	}

#ifdef IPV6_ENABLED
	if (_peer.family == PF_INET6)
	{
		_peer.sa.a6.sin6_family = _peer.family;
		_peer.sa.a6.sin6_addr = addr.addr.a6;
		_peer.sa.a6.sin6_port = htons(port);
		_peer.sa.a6.sin6_scope_id = addr.scopeid;

		return;
	}
#endif // IPV6_ENABLED
}

void TCPSocket::createSocket()
{
	endSocket();
/*
	if (_so != INVALID_SOCKET)
	{
#ifdef ZQ_OS_MSWIN
		closesocket(_so);
#else
		close(_so);
#endif
	}
*/
	_so = socket(PF_INET, SOCK_STREAM, 0);
	if(_so == INVALID_SOCKET)
	{
		error(errCreateFailed);
		return;
	}

	Socket::_state = stAvailable;
}

InetHostAddress TCPSocket::getPeer(tpport_t *port)
{
	// FIXME: insufficient buffer
	//        how to retrieve _peer ??
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

		memset((void*) &_peer, 0, sizeof(_peer));

		return dummyAddr;
	}
#else
	if(rtn < 1)
	{
		if(port)
			*port = 0;

		memset((void*) &_peer, 0, sizeof(_peer));
		return dummyAddr;
	}
#endif

	if (source.sa.a.sin_family == PF_INET)
	{
		_peer.family = source.sa.a.sin_family;
		memcpy(&(_peer.sa.a), &(source.sa.a), sizeof(_peer.sa.a));
		if(port)
			*port = ntohs(_peer.sa.a.sin_port);

		return InetHostAddress(_peer.sa.a.sin_addr);
	}

#ifdef IPV6_ENABLED
	if (source.sa.a6.sin6_family == PF_INET6)
	{
		_peer.family = source.sa.a6.sin6_family;
		memcpy(&(_peer.sa.a6), &(source.sa.a6), sizeof(_peer.sa.a6));
		if(port)
			*port = ntohs(_peer.sa.a6.sin6_port);
		return InetHostAddress(_peer.sa.a6.sin6_addr);
	}
#endif // IPV6_ENABLED

	return dummyAddr;
}


int TCPSocket::send(const void *buf, size_t len)
{
//	socklen_t slen = sizeof(_peer.sa.a);

	int ret = ::send(_so, (const char*)buf, (int) len, 0);
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
		perror("TCPSocket::send");
#endif
	}
#endif
	return ret;
}

int TCPSocket::receive(void *buf, size_t len, RECVMODEL model, int32 timeOut /*= 0*/)
{
	if (!isConnected())
		return -1;
	int ret = 0;
	if (timeOut == -1)
	{
		ret = ::recv(_so, (char *)buf, (int)len, 0);
	}
	else
	{
		struct timeval timeout;
		fd_set fd;
		FD_ZERO(&fd);   
		FD_SET(_so, &fd);
		timeout.tv_sec	= timeOut;
		timeout.tv_usec	= 0;
		if (timeOut == 0)
			timeout.tv_usec = 300;
		int rc = select(0, &fd, NULL, NULL, &timeout);

		if (rc > 0)
		{
			ret = ::recv(_so, (char *)buf, (int)len, 0);
		}
		else 
			return rc;
	}

#ifdef ZQ_OS_MSWIN
	if (socket_errno == WSAECONNRESET || ret == 0) 
#else
	if (socket_errno == ECONNRESET || ret == 0) 
#endif
		return -1;
	if (model == NONBLOCK && ret == -1)
	{
		int err = socket_errno;
#ifdef ZQ_OS_MSWIN
		if (err == WSAEWOULDBLOCK)
#else
		if (err == EWOULDBLOCK)
#endif
			return 0;
		else
			return -1;
	}		
	return ret;
}

int TCPSocket::peek(void *buf, size_t len)
{
	return ::recv(_so, (char *)buf, (int)len, MSG_PEEK);
}

#ifdef ZQ_OS_MSWIN
bool TCPSocket::privateSetBlock(bool enable)
{
	int iMode = 0;
	int ret = 0;

	if (enable)
	{
		ret = ioctlsocket(_so, FIONBIO, (u_long FAR*) &iMode);
		if (0 == ret)
			return true;
	}
	else
	{
		iMode = 1;
		ret = ioctlsocket(_so, FIONBIO, (u_long FAR*) &iMode);
		if (0 == ret)
			return false;
	}

	return false;
}
#endif

bool TCPSocket::setBlock(RECVMODEL model)
{
//	int iMode = 0;
//	int ret = 0;

	//already the specified recv type
	if (_bBlockEnable == (model == BLOCK))
		return true;

#ifdef ZQ_OS_MSWIN
	return privateSetBlock(_bBlockEnable);
#else
	setCompletion(_bBlockEnable);
	return true;
#endif
}

bool TCPSocket::disconnect()
{
	Socket::_state = stBound;
	_flagsPending = 0;
//	_bRecvdDataInPending = false;
	if (_gWatchDog)
		_gWatchDog->unwatch(*this);

	endSocket();
	return true;
}

void TCPSocket::updateConnDesc()
{
	tpport_t bport=0, sport=0;
	std::string bindAddr=Socket::getLocal(&bport).getHostAddress();
	std::string peerAddr=Socket::getPeer(&sport).getHostAddress();

	snprintf(_connDesc, sizeof(_connDesc) -2, "%08x|%s/%d->%s/%d", _so, bindAddr.c_str(), bport, peerAddr.c_str(), sport);
}

const char* TCPSocket::connDescription()
{
	if (Socket::_state <= stConnecting)
		_connDesc[0] =0x00;

	return _connDesc;
}

// -----------------------------
// class TCPClient
// -----------------------------
bool TCPClient::connect(int32 sTimeOut)
{
	createSocket();

	//set the default time out
	Socket::_state = stConnecting;
	if (sTimeOut < 0)
#ifdef ZQ_OS_MSWIN
		privateSetBlock(true);
#else
		setCompletion(true);
#endif
	else
#ifdef ZQ_OS_MSWIN
		privateSetBlock(false);
#else
		setCompletion(false);
#endif

	int rc;

	//connect to server
	struct sockaddr *serverAddr = (struct sockaddr *) &(_peer.sa.a);
	socklen_t serverLen = sizeof(_peer.sa.a);

#ifdef IPV6_ENABLED
	if (_peer.family == PF_INET6)
	{
		saddr = (struct sockaddr *) &(_peer.sa.a6);
		serverLen = sizeof(_peer.sa.a6);
	}
#endif // IPV6_ENABLED

	_lastUpdated = TimeUtil::now();
	_timeout = sTimeOut;
	_flagsPending = 0;
//	_bRecvdDataInPending = false;
	rc = ::connect(_so, serverAddr, serverLen);

	//fail to connect server
#ifdef ZQ_OS_MSWIN
	if (rc == SOCKET_ERROR)
#else
	if (rc == (-1))
#endif
	{
		Socket::Error err = Socket::connectError();
#ifdef ZQ_OS_MSWIN
		if (sTimeOut < 0 || errConnectBusy == err)
#else
		if (sTimeOut < 0 || errConnectBusy == err)
#endif
		{
			// if it is busy at connecting, let the watchdog wake up later thru entry OnConnect()
			if (_gWatchDog)
				_gWatchDog->watch(*this);
			return true;
		}
	}

	Socket::setKeepAlive(true);

	setBlock(_bBlockEnable ? BLOCK : NONBLOCK); // restore the old BLOCK setting
	Socket::_state = stConnected;
	updateConnDesc();

	return true;
}

// -----------------------------
// class TCPServer
// -----------------------------
bool TCPServer::listen(int backlog)
{
	if (_state != stBound)
	{
		createSocket();
		bind();

		if (_state != stBound)
		{
			error(errBindingFailed, "Could not bind socket to listen()", socket_errno);
			return false;
		}
	}

	_flagsPending = 0;
	if (::listen(_so, backlog))
	{
		endSocket();
		error(errBindingFailed, "Could not listen on socket", socket_errno);
		return false;
	}

	_state = stStream;

	if (_gWatchDog)
		_gWatchDog->watch(*this);

	return true;
}

void TCPServer::stopListening()
{
	_flagsPending = 0;
	if (_gWatchDog)
		_gWatchDog->unwatch(*this);

	endSocket();
}

void TCPServer::OnDataArrived()
{
	SOCKET newConn = ::accept(_so, NULL, NULL);
	if (INVALID_SOCKET == newConn)
		return;

	TCPSocket *acceptedConn = new TCPSocket(newConn);

	TCPSocket* pCustomizedServerConn;
	
	if (_gWatchDog != NULL)
	{
		pCustomizedServerConn = _gWatchDog->OnAccepted(*acceptedConn);
	}
	else pCustomizedServerConn = OnAccepted(*acceptedConn);

	if (NULL != pCustomizedServerConn)
	{
		_gWatchDog->watch(*pCustomizedServerConn);
		return;
	}
	else
	{
		// the user's impl ask to reject the connection
		// the connection would be closed per the destructor of acceptedConn
	}
}


}//namespace common
}//namespace ZQ

// vim: ts=4 sw=4 bg=dark nu
