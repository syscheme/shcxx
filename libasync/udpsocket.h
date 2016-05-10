#pragma once
#ifndef __UDP_SOCKET_H
#define __UDP_SOCKET_H

#include "socket.h"

namespace LibAsync {
	class ZQ_COMMON_API UDPSocket;
	class UDPSocket : protected virtual  LibAsync::Socket, public virtual ZQ::common::SharedObject{
	protected:
		UDPSocket(EventLoop& loop);
	private:
		//disallow copy construction
		UDPSocket(EventLoop& loop, SOCKET sock);
		UDPSocket(const UDPSocket&);
		UDPSocket& operator=(const UDPSocket&);
	public:
		typedef ZQ::common::Pointer<UDPSocket> Ptr;
		virtual ~UDPSocket(void);

	public:
		static UDPSocket::Ptr create(EventLoop& loop);
		bool    openudp();
		bool	bind(const std::string& ip, unsigned short port);
		bool 	peer(const std::string& ip, unsigned short port);
		bool	local(const std::string& ip, unsigned short port);
		EventLoop&  getLoop() const  { return Socket::getLoop(); }
		bool    sendto(const std::string& ip, unsigned short port, AsyncBuffer buf);
		bool    sendto(AsyncBuffer);

		bool    recvfrom(const std::string& ip, unsigned short port, AsyncBuffer buf);
		bool    recvfrom(AsyncBuffer buf);

		bool    setbroadcast(bool enable = true);
		bool    setgroup(const std::string& ip, unsigned short port);
		bool    setgroup();
		bool    setTTL(int ttl);
		bool    setSendBufSize(int size);
		bool	setPeer( const std::string& ip, unsigned short port );
		//fun to validates multicast addresses
		bool    vaildatemulticast(const std::string& ip);
#ifdef ZQ_OS_LINUX
		virtual bool    recvAction();
		virtual bool    sendAction(bool firstSend = false);
		virtual int     sendDirect(AsyncBuffer buf);
	//	virtual int     sendDirect(const AsyncBufferS& bufs);
	//	virtual void    errorAction(int err);
#endif
	protected:
		virtual	void	onSocketRecved(size_t size, std::string ip, unsigned short port) { }


	protected:
		SocketAddrHelper			mPeerInfo;
		std::string                 mLocalAddr;
		unsigned  short             mLocalPort;
		std::string                 mPeerAddr;
		unsigned  short             mPeerPort;
		bool                        mBroadcast;
		bool                        mMulticast;
	private:
#if __BYTE_ORDER == __BIG_ENDIAN
		enum { MCAST_VALID_MASK = 0xF0000000, MCAST_VALID_VALUE = 0xE0000000 };
#else
		enum { MCAST_VALID_MASK = 0x000000F0, MCAST_VALID_VALUE = 0x000000E0 };
#endif

	};
}


#endif
