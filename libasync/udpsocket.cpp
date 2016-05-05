#include "udpsocket.h"

namespace LibAsync {
	UDPSocket::UDPSocket(EventLoop& loop):Socket(loop)
	{
		mMulticast = false;
		mBroadcast = false;
	}

	UDPSocket::UDPSocket(EventLoop& loop, SOCKET sock):Socket(loop, sock)
	{
		mMulticast = false;
		mBroadcast = false;
	}
	
	UDPSocket::~UDPSocket()
	{
	
	}

	UDPSocket::Ptr UDPSocket::create(EventLoop& loop)
	{
		return new UDPSocket(loop);
	}

	bool UDPSocket::bind(const std::string& ip, unsigned short port)
	{
		SocketAddrHelper helper(false);
		if (!helper.parse(ip, port))
			return false;
		mLocalAddr = ip;
		mLocalPort = port;
		const struct addrinfo* info = helper.info();
		assert(info != NULL && "should never be false");
		if (!isOpened()) {
			if (!open(info->ai_family, info->ai_socktype, info->ai_protocol))
				return false;
		}
		if (alive()) {
			assert(false);// abort in DEBUG mode
			return false;//can't bind a already connected socket
		}
#ifdef ZQ_OS_MSWIN
#define socklen_t int
#endif
		setReuseAddr(true);
		if(::bind(mSocket, info->ai_addr, (socklen_t)info->ai_addrlen) != 0)
			return false;
		mbAlive = true;
		return true;

	}

	bool UDPSocket::openudp()
	{
		if (!isOpened())
		{
			if(open(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
			{
				mbAlive = true;
				return true;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	void    UDPSocket::peer(const std::string& ip, unsigned short port)
	{
		setPeer(ip, port);
	}

	void UDPSocket::local(const std::string& ip, unsigned short port)
	{
		if( ip.empty())
			mLocalAddr = "0.0.0.0";
		else
			mLocalAddr = ip;
		mLocalPort = port;
		bind(mLocalAddr, port);
	}

	bool UDPSocket::setbroadcast(bool enable/*=true*/)
	{
		int opt = enable ? 1 : 0;
		int ret = setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (char *)&opt, (socklen_t)sizeof(opt));
		if (ret == 0)
		{
			mBroadcast = enable;
			return true;
		}
		mBroadcast = false;
		return false;
	}

	
	bool UDPSocket::recvfrom(const std::string& ip, unsigned short port, AsyncBuffer buf)
	{
		mPeerAddr = ip;
		mPeerPort = port;
		return recvfrom(buf);
	}

	bool UDPSocket::recvfrom(AsyncBuffer buf)
	{
		if (!openudp())
			return false;
		if (buf.len <= 0)
			return false;
		AsyncBufferS bufs;
		bufs.push_back(buf);
		return recv(bufs);
	}

	bool UDPSocket::setPeer( const std::string& ip, unsigned short port) {
		mPeerAddr = ip;
		mPeerPort = port;
		if(vaildatemulticast(ip)) {
			return setgroup(ip, port);
		} else {
			return mPeerInfo.parse(mPeerAddr, mPeerPort);
		}
	}

	bool UDPSocket::sendto(const std::string& ip, unsigned short port, AsyncBuffer buf)
	{
		if (buf.len <= 0)
			return false;
		setPeer(ip, port);
		return sendto(buf);
	}

	bool UDPSocket::sendto(AsyncBuffer buf)
	{
		if ( !openudp() || mPeerAddr.empty() || mPeerPort == 0 )
			return false;
		if (buf.len <= 0)
			return false;
		AsyncBufferS bufs;
		bufs.push_back(buf);
		if (vaildatemulticast(mPeerAddr))
			setgroup();
		return send(bufs);
	}

	bool UDPSocket::setgroup(const std::string& ip, unsigned short port)
	{
		mLocalAddr = ip;
		mLocalPort = port;
		return setgroup();
	}

	bool UDPSocket::setgroup()
	{
		/*
		SocketAddrHelper helper(false);
		if (!helper.parse(mLocalAddr, mLocalPort))
			return false;
		const struct addrinfo* info = helper.info();
		assert(info != NULL && "should never be false");
		if (!isOpened()) {
			if (!open(info->ai_family, info->ai_socktype, info->ai_protocol))
				return false;
		}*/
        int err = 0;
        char buf[1024];
        struct hostent ret, *hostEnt;
        if (gethostbyname_r(mPeerAddr.c_str(), &ret, buf, sizeof(buf), &hostEnt, &err)!=0){
            return false;
        }
		if (NULL == hostEnt)
			return false;
		struct in_addr ia;
		memcpy((void*)&ia, (void*)hostEnt->h_addr, sizeof(ia));
		struct sockaddr_in myaddr;
		memset((void*)&myaddr, '0', sizeof(struct sockaddr_in));
		socklen_t addr_len = sizeof(myaddr);
		getsockname(mSocket, (struct sockaddr*)&myaddr, &addr_len);
		int loop_key = 1;
		setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop_key, 1);
		unsigned char ttl=254;  
		setsockopt(mSocket, IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)); 
		struct ip_mreq group;
		memcpy((void*)&group.imr_multiaddr.s_addr, (void*)&ia, sizeof(struct in_addr));
		memcpy(&group.imr_interface, &(myaddr.sin_addr), sizeof(group.imr_interface));
		return ( 0 == setsockopt(mSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group) ) );
	}

	bool UDPSocket::vaildatemulticast(const std::string& ip)
	{
		struct in_addr address;
		bzero(&address, sizeof(address));
		inet_pton(AF_INET, ip.c_str(), &address);
		if ((address.s_addr != INADDR_ANY) && (address.s_addr & MCAST_VALID_MASK) != MCAST_VALID_VALUE)
			return false;
		return true;
	}
}
