
#include "tcpClient.h"


void tcpClient::OnRead(ssize_t nread, const char *buf)
{
	printf("recv data:%s,len = %d\n", buf,nread);

	char sendbuf[1024];
	memset(sendbuf,0,1024);
	scanf("%s",sendbuf);

	write(sendbuf,strlen(sendbuf)+1);
}

void tcpClient::OnConnected(ElpeError status)
{
	if (status != ElpeError::elpeSuccess) {
		fprintf(stderr, "on_connect error %s\n", Error(status).str());
		return;
	}
	read_start();

	char buf[1024];
	memset(buf,0,1024);
	scanf("%s",buf);

	write(buf,strlen(buf)+1);

}