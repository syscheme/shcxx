#include "udpServer.h"



udpServer::udpServer()
{

}

udpServer::~udpServer()
{

}

void udpServer::OnSent(UDP *self, int status)
{
	if (status) {
		fprintf(stderr, "Send error %s\n", eloop_strerror(status));
		return;
	}
}

void udpServer::OnRead(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
	char sender[17] = { 0 };
	get_ip4_name((const struct sockaddr_in*)addr, sender, 16);
	printf("Recv from %s\n",sender);

/*	char* recvdata = (char*)malloc(nread);
	memset(recvdata,0,nread);
	memcpy(recvdata,buf->base,nread);*/
	printf("recv data:%s,len = %d\n", buf->base,nread);

/*
	uv_udp_send_t* sendreq;
	sendreq = (uv_udp_send_t*)malloc(sizeof(*sendreq));
	uv_buf_t sendbuf = uv_buf_init(recvdata, nread);
*/
//	printf("send buf:%s,len=%d\n",recvdata, nread);

	send(buf->base,nread,addr);
}