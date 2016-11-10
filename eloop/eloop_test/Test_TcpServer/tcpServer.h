#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class tcpServer:public TCP
{
public:
	tcpServer(){}
	~tcpServer(){}



	virtual void OnConnection_cb(int status);

	virtual void OnRead_cb(ssize_t nread, const uv_buf_t *buf);
};

#endif