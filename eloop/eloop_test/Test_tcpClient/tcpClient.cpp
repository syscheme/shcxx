
#include "tcpClient.h"


void tcpClient::OnRead(ssize_t nread, const uv_buf_t *buf)
{
	printf("recv data:%s,len = %d\n", buf->base,nread);

	char sendbuf[1024];
	memset(sendbuf,0,1024);
	scanf("%s",sendbuf);

	write(sendbuf,strlen(sendbuf));
}

void tcpClient::OnConnected(int status)
{
	if (status < 0) {
		//fprintf(stderr, "on_connect error %s\n", uv_strerror(status));
		return;
	}
	read_start();

	char buf[1024];
	memset(buf,0,1024);
	scanf("%s",buf);

	write(buf,strlen(buf));

}