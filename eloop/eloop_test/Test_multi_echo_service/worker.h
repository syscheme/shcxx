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
	Transport(int id):_workerid(id){}
	~Transport(){}

	virtual void OnRead(ssize_t nread, const char *buf);
private:
	int _workerid;
};

class worker:public TCP
{
public:
	worker(int id):_workerid(id){}
	virtual void OnRead(ssize_t nread, const char *buf);
private:
	int _workerid;
};

#endif