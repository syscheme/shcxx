#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "eloop_net.h"
using namespace ZQ::eloop;


class tcpServer:public TCP
{
public:
	tcpServer(){}
	~tcpServer(){}



	virtual void OnConnection_cb(ElpeError status);

	virtual void OnRead(ssize_t nread, const char *buf);
};

#endif