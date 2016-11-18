#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class udpServer:public UDP
{
public:
	udpServer();
	~udpServer();

	virtual void OnSent(UDP *self, int status);
	virtual void OnRead(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

};

#endif