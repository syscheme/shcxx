#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class udpClient:public UDP
{
public:
	udpClient();
	~udpClient();

	virtual void OnSent(UDP *self, int status);
	virtual void OnRead(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
};

#endif