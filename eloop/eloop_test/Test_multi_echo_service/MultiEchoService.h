#ifndef __MULTI_ECHO_SERVER_H__
#define __MULTI_ECHO_SERVER_H__

#include "eloop_net.h"
#include "eloop_file.h"
#include <vector>
using namespace ZQ::eloop;

class ChildProcess:public Process
{
public:
	virtual void OnExit(int64_t exit_status,int term_signal) 
	{
		close();
		printf("------------------\n");
	}
};

class Transport;
class MultiEchoServer:public TCP
{
public:
	typedef struct child_worker {
		Pipe* pipe;
		ChildProcess* proc;
	}CHILD_WORKER;
	typedef std::vector<CHILD_WORKER> WORKERS;
public:
	MultiEchoServer();
	~MultiEchoServer();

	void SetupWorker();

	virtual void doAccept(ElpeError status);

private:
	WORKERS		_workers;
	int			_round_robin_counter;
};

#endif