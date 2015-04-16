#include "../lwsocket.h"
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32")

int main(int argc, char* argv[])
{
	if(argc<3)
	{
		printf("usage : receiver ip port\r\n");
		return 1;
	}

	in_addr6 grpia6;
	if(!LwSocket::inet_pton6(argv[1],grpia6))
	{
		printf("parse ipaddr error!!");
		return 1;
	}
	
	unsigned short grpport=atoi(argv[2]);
	if(grpport==0)
	{
		printf("port error");
		return 1;
	}

	LwSocket recv_socket;
	recv_socket.create_udp_socket6();

	if(!recv_socket.bind6("::1",grpport))
	{
		printf("bind group error,error code is %d\r\n",::WSAGetLastError());
		return 1;
	}

	if(!recv_socket.mcast_join6(grpia6))
	{
		printf("join group error,error code is %d\r\n",::WSAGetLastError());
		return 1;
	}

	if(!recv_socket.mcast_set_loop6(true))
		printf("set loop error, code is %d",::WSAGetLastError());

	while(true)
	{
		static char buf[1300];
		static int XXX=sprintf(buf,"ffff, you ssss");

		sockaddr_in6 sa6;
		memset(&sa6,0,sizeof(sa6));

		int ret=recv_socket.recv_from6(buf,1300,&sa6);

		if(ret!=SOCKET_ERROR)
			printf("receive %d\r\n",ret);
		else
			printf("last error is %d\r\n",::WSAGetLastError());
	}
	return 0;
}

