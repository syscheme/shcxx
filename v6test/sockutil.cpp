#include "sockutil.h"
#pragma comment(lib,"ws2_32")
#pragma warning(disable:4244)

Log glog;

class Winsock_Env
{
public:
	Winsock_Env(BYTE low=2,BYTE high=2)
	{
		WSADATA m_data;
		if(WSAStartup(MAKEWORD(low,high),&m_data)!=NULL)
			//i think it will never fail, so i don't throw exception
			glog(Log::L_CRIT,"%s:WSAStartup Error",__FUNCTION__);
	}
	~Winsock_Env(){	WSACleanup();}
};
//will this works???
static Winsock_Env gwinsock_env(2,2);


bool inet_pton6(const char *src,in_addr6 &dst)
{
	sockaddr_in6 sa;
	int salen=sizeof(sa);
	memset(&sa,0,sizeof(sa));
	char *dupsrc=strdup(src);
	if(!dupsrc) return false;
	int ret=WSAStringToAddress(dupsrc,AF_INET6,NULL,(sockaddr*)&sa,&salen);
	delete dupsrc;
	if(ret==0)
	{
		dst=sa.sin6_addr;
		return true;
	}
	else
		return false;
}

//=====================================================================
SOCKET create_udp_socket4(int protocol)
{
	return WSASocket(AF_INET,SOCK_DGRAM,protocol,NULL, 0, WSA_FLAG_OVERLAPPED);
}

SOCKET create_udp_socket6(int protocol)
{
	return WSASocket(AF_INET6,SOCK_DGRAM,protocol,NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool sock_bind4(SOCKET fd,const char *ipaddr,unsigned short port)
{
	return sock_bind4(fd,inet_addr(ipaddr),port);
}

bool sock_bind4(SOCKET fd,unsigned long ipaddr,unsigned short port)
{	
	sockaddr_in sa;
	memset(&sa,0,sizeof(sa));
	sa.sin_family=AF_INET;
	sa.sin_port=htons(port);
	sa.sin_addr.S_un.S_addr=ipaddr;
	return sock_bind4(fd,sa);
}

bool sock_bind4(SOCKET fd,sockaddr_in &sa)
{
	int ret=bind(fd,(sockaddr*)&sa,sizeof(sa));
	if(ret!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:bind error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool sock_bind4(SOCKET fd,in_addr ipaddr,unsigned short port)
{
	sockaddr_in sa;
	memset(&sa,0,sizeof(sa));
	sa.sin_addr=ipaddr;
	sa.sin_family=AF_INET;
	sa.sin_port=htons(port);
	return sock_bind4(fd,sa);
}

int inet_pton4(const char *src,in_addr &dst)
{
	unsigned long ret=inet_addr(src);
	dst.S_un.S_addr=ret;
	if(ret==INADDR_NONE)
		return 0;
	else
		return 1;
}


bool sock_bind6(SOCKET fd,const char *ipaddr6,unsigned short port)
{
	sockaddr_in6 sa6;
	memset(&sa6,0,sizeof(sa6));
	sa6.sin6_family=AF_INET6;
	sa6.sin6_port=htons(port);
	if(!inet_pton6(ipaddr6,sa6.sin6_addr))
	{
		glog(Log::L_ERROR,"%s:address error",__FUNCTION__);
		return false;
	}
	return sock_bind6(fd,sa6);
}

bool sock_bind6(SOCKET fd,in_addr6 &ia6,unsigned short port)
{
	sockaddr_in6 sa6;
	memset(&sa6,0,sizeof(sa6));
	sa6.sin6_family=AF_INET6;
	sa6.sin6_port=htons(port);
	sa6.sin6_addr=ia6;
	return sock_bind6(fd,sa6);
}

bool sock_bind6(SOCKET fd,sockaddr_in6 &sa6)
{
	int ret=bind(fd,(sockaddr*)&sa6,sizeof(sa6));
	if(ret!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:bind error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool mcast_join4(SOCKET sockfd,unsigned long grpaddr,unsigned long srcaddr)
{
	ip_mreq im;
	im.imr_interface.S_un.S_addr=srcaddr;
	im.imr_multiaddr.S_un.S_addr=grpaddr;
	int ret=setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&im, sizeof(im));
	if(ret!=SOCKET_ERROR)
		return true;
	glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
	return false;
}

bool mcast_leave4(SOCKET sockfd,unsigned long grpaddr,unsigned long srcaddr=INADDR_ANY)
{
	ip_mreq im;
	im.imr_interface.S_un.S_addr=srcaddr;
	im.imr_multiaddr.S_un.S_addr=grpaddr;
	int ret=setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&im, sizeof(im));
	if(ret!=SOCKET_ERROR)
		return true;
	glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
	return false;
}

bool mcast_join6(SOCKET sockfd,in6_addr &ia,unsigned long localif)
{
	ipv6_mreq im;
	im.ipv6mr_interface=localif;
	im.ipv6mr_multiaddr=ia;
	int ret=setsockopt(sockfd,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP, (char*)&im, sizeof(im));
	if(ret!=SOCKET_ERROR)
		return true;
	glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
	return false;
}

bool mcast_leave6(SOCKET sockfd,in6_addr &ia,unsigned long localif)
{
	ipv6_mreq im;
	im.ipv6mr_interface=localif;
	im.ipv6mr_multiaddr=ia;
	int ret=setsockopt(sockfd,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP, (char*)&im, sizeof(im));
	if(ret!=SOCKET_ERROR)
		return true;
	glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
	return false;
}

bool mcast_set_loop4(SOCKET sockfd,bool onoff)
{
	BOOL loop=onoff ? TRUE : FALSE;
	if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_LOOP,(char*)&loop,sizeof(loop))!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool mcast_set_loop6(SOCKET sockfd,bool onoff)
{
	BOOL loop=onoff ? TRUE : FALSE;
	if(setsockopt(sockfd,IPPROTO_IPV6,IPV6_MULTICAST_LOOP,(char*)&loop,sizeof(loop))!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool set_linger(SOCKET fd)
{
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;

	if (SOCKET_ERROR == ::setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct)))
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
	else
		return true;
}

bool set_block(SOCKET fd)
{
	unsigned long zero=0;
	if (ioctlsocket(fd, FIONBIO, &zero) == SOCKET_ERROR)
	{
		glog(Log::L_ERROR,"%s:ioctlsocket error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
	else
		return true;
}

bool set_noblock(SOCKET fd)
{
	unsigned long zero=1;
	if (ioctlsocket(fd, FIONBIO, &zero) == SOCKET_ERROR)
	{
		glog(Log::L_ERROR,"%s:ioctlsocket error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
	else
		return true;
}

bool set_sndbuf(SOCKET fd,unsigned int buflen)
{
	if(setsockopt(fd,SOL_SOCKET, SO_SNDBUF,(char*)&buflen,sizeof(unsigned int))==SOCKET_ERROR)
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
	else
		return true;
}

bool set_rcvbuf(SOCKET fd,unsigned int buflen)
{
	if(setsockopt(fd,SOL_SOCKET, SO_RCVBUF,(char*)&buflen,sizeof(unsigned int))==SOCKET_ERROR)
	{
		glog(Log::L_ERROR,"%s:bind error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
	else
		return true;
}


bool mcast_set_ttl4(SOCKET sockfd,int ttl)
{
	DWORD newttl=ttl;
	if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,(char*)&newttl,sizeof(newttl))!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool mcast_set_ttl6(SOCKET sockfd,int ttl)
{
	DWORD newttl=ttl;
	if(setsockopt(sockfd,IPPROTO_IPV6,IPV6_MULTICAST_HOPS,(char*)&newttl,sizeof(newttl))!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}

bool set_reuseaddr(SOCKET sockfd)
{
	BOOL opt=TRUE;
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt))!=SOCKET_ERROR)
		return true;
	else
	{
		glog(Log::L_ERROR,"%s:setsockopt error, error code is %u",__FUNCTION__,WSAGetLastError());
		return false;
	}
}
