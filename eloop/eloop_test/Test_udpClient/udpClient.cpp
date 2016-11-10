#include "udpClient.h"



udpClient::udpClient()
{

}

udpClient::~udpClient()
{

}

void udpClient::OnSend_cb(UDP *self, int status)
{
	if (status) {
		fprintf(stderr, "Send error %s\n", eloop_strerror(status));
		return;
	}
}

void udpClient::OnRead_cb(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
	char sender[17] = { 0 };
	get_ip4_name((const struct sockaddr_in*)addr, sender, 16);
	printf("Recv from %s\n",sender);

/*	char* recvdata = (char*)malloc(nread);
	memset(recvdata,0,nread);
	memcpy(recvdata,buf->base,nread);
	*/
	printf("recv data:%s len = %d\n", buf->base,nread);

	char sendbuf[1024];
	memset(sendbuf,0,1024);
	scanf("%s",sendbuf);

	send(sendbuf,strlen(sendbuf),addr);
}