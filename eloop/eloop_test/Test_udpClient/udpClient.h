#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class udpClient:public UDP
{
public:
	udpClient();
	~udpClient();

	virtual void OnSend_cb(UDP *self, int status);
	virtual void OnRead_cb(UDP *self, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
};

#endif