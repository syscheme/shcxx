#ifndef __WORKER_H__
#define __WORKER_H__

#include "eloop_net.h"
#include "eloop_file.h"
#include <vector>
using namespace ZQ::eloop;


class Transport:public Pipe
{
	//	friend class MultiEchoServer;
public:
	Transport(){}
	~Transport(){}

	virtual void OnRead(ssize_t nread, const char *buf);
};

class worker:public TCP
{
public:
	virtual void OnRead(ssize_t nread, const char *buf);
};

#endif