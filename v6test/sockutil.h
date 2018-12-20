#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>

class Log
{
public:
	typedef enum  //	level range: value 0~7
	{
		L_EMERG=0, L_ALERT,  L_CRIT, L_ERROR,
		L_WARNING, L_NOTICE, L_INFO, L_DEBUG
	};
	void operator ()(int x,const char *fmt...){}
};

extern Log glog;

SOCKET create_udp_socket4(int protocol=IPPROTO_UDP);
SOCKET create_udp_socket6(int protocol=IPPROTO_UDP);

bool sock_bind4(SOCKET fd,const char *ipaddr,unsigned short port);
bool sock_bind4(SOCKET fd,unsigned long ipaddr,unsigned short port);
bool sock_bind4(SOCKET fd,in_addr ipaddr,unsigned short port);
bool sock_bind4(SOCKET fd,sockaddr_in &sa);

bool sock_bind6(SOCKET fd,const char *ipaddr6,unsigned short port);
bool sock_bind6(SOCKET fd,in_addr6 &ia6,unsigned short port);
bool sock_bind6(SOCKET fd,sockaddr_in6 &sa6);

bool inet_pton6(const char *src,in_addr6 &dst);

//now we use IGMPV2, en, we'll support IGMPV3 soon!
bool mcast_join4(SOCKET sockfd,unsigned long grpaddr,unsigned long srcaddr);
bool mcast_leave4(SOCKET sockfd,unsigned long grpaddr,unsigned long srcaddr);

bool mcast_join6(SOCKET sockfd,in6_addr &ia,unsigned long localif=0);
bool mcast_leave6(SOCKET sockfd,in6_addr &ia,unsigned long locali=0);

bool mcast_set_loop4(SOCKET sockfd,bool onoff=true);
bool mcast_set_loop6(SOCKET sockfd,bool onoff=true);

bool set_linger(SOCKET fd);
bool set_block(SOCKET fd);
bool set_noblock(SOCKET fd);

bool set_sndbuf(SOCKET fd,unsigned int buflen);
bool set_rcvbuf(SOCKET fd,unsigned int buflen);

bool mcast_set_ttl4(SOCKET fd,int ttl);
bool mcast_set_ttl6(SOCKET fd,int ttl);

bool set_reuseaddr(SOCKET fd);

