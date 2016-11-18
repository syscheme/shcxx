#include "tcpServer.h"



void tcpServer::OnConnection_cb(int status)
{
	if (status < 0) {
//		fprintf(stderr, "New connection error %s\n", uv_strerror(status));
		// error!
		return;
	}

	tcpServer *client = new tcpServer();
	client->init(get_loop());

	if (accept((Stream *)client) == 0) {

	//	read_start();
		client->read_start();
	}
	else {
		client->close();
	}

}


void tcpServer::OnRead(ssize_t nread, const uv_buf_t *buf)
{
	printf("recv data:%s,len = %d\n", buf->base,nread);

	write(buf->base,nread);
}