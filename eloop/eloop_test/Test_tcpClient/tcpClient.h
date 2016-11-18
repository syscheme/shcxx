#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class tcpClient:public TCP
{
public:
	tcpClient(){}
	~tcpClient(){}

	virtual void OnConnected(int status);
	virtual void OnRead(ssize_t nread, const uv_buf_t *buf);

};

#endif