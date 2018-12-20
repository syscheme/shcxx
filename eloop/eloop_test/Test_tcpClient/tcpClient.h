#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "eloop_net.h"
#include "Pointer.h"
using namespace ZQ::eloop;


class tcpClient:public TCP
{
public:	
	typedef ZQ::common::Pointer<tcpClient> Ptr;
public:
	tcpClient(){}
	~tcpClient(){}

	virtual void OnConnected(ElpeError status);
	virtual void OnRead(ssize_t nread, const char *buf);

};

#endif