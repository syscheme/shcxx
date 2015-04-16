#include "../lwsocket.h"
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32")
int main(int argc, char* argv[])
{
	if(argc<3)
	{
		printf("usage : sender ip port\r\n");
		return 1;
	}

	in_addr6 grpia6;
	if(!LwSocket::inet_pton6(argv[1],grpia6)) 
	{
		printf("parse addr error!!\r\n");
		return 1;
	}

	unsigned short grpport=atoi(argv[2]);
	if(grpport==0)
	{
		printf("port error!!");
		return 1;
	}

	LwSocket send_socket;
	send_socket.create_udp_socket6();

	sockaddr_in6 sa6;
	memset(&sa6,0,sizeof(sa6));
	sa6.sin6_family=AF_INET6;
	sa6.sin6_port=htons(grpport);
	sa6.sin6_addr=grpia6;

	for(int i=0;i<1000;i++)
	{
		static char buf[1300];
		static int XXX=sprintf(buf,"faint, you stupid");
		
		int ret=send_socket.send_to6(buf,1300,&sa6);
		if(ret==SOCKET_ERROR)
			printf("last error is %d\r\n",::WSAGetLastError());
		else
			printf("Send ok\r\n");
		Sleep(20);
	}
	return 0;
}

