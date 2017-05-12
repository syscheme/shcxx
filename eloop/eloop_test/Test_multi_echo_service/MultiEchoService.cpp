#include "MultiEchoService.h"
#include "eloop.h"




MultiEchoServer::MultiEchoServer()
:_round_robin_counter(0)
{

}

MultiEchoServer::~MultiEchoServer()
{

}

void MultiEchoServer::SetupWorker()
{
	size_t path_size = 500;
	char worker_path[500];
	Handle::exepath(worker_path, &path_size);
	strcpy(worker_path + (strlen(worker_path) - strlen("Test_multi-echo-server.exe")-1), "worker.exe");
	fprintf(stderr, "Worker path: %s\n", worker_path);

	char* args[2];
	args[0] = worker_path;
	// ...
	// launch same number of workers as number of CPUs
	ZQ::eloop::CpuInfo cpu;
	int child_worker_count = cpu.getCpuCount();

	for(int i = 0;i<child_worker_count;i++)
	{
		CHILD_WORKER work;
		work.pipe = new Pipe();
		work.pipe->init(get_loop(),1);

		Process::eloop_stdio_container_t child_stdio[3];

		child_stdio[0].flags = (Process::eloop_stdio_flags)(Process::ELOOP_CREATE_PIPE | Process::ELOOP_READABLE_PIPE);
		child_stdio[0].data.stream = (uv_stream_t*)(work.pipe->context_ptr());
		child_stdio[1].flags = (Process::eloop_stdio_flags)Process::ELOOP_IGNORE;
		child_stdio[2].flags = (Process::eloop_stdio_flags)Process::ELOOP_INHERIT_FD;
		child_stdio[2].data.fd = 2;

		char id[4];
		sprintf(id,"%d",i+1);
		args[1] = id;

		work.proc = new ChildProcess();
		work.proc->init(get_loop());
		int r = work.proc->spawn(args[0],args,child_stdio,3); 
		if (r != elpeSuccess)
		{
			printf("spawn() error code[%d] desc[%s]\n",r,errDesc(r));
			return;
		}
		
		fprintf(stderr, "Started worker %d\n",work.proc->pid());
		_workers.push_back(work);
	}
}

void MultiEchoServer::doAccept(ElpeError status)
{
	if (status != elpeSuccess)
	{
		printf("doAccept() error code[%d] desc[%s]",status,errDesc(status));
		return;
	}

	TCP *client = new TCP();

	client->init(get_loop());

	if (accept(client) == 0) {

		char* dummy_buf = "a";
		_workers[_round_robin_counter].pipe->write(dummy_buf,1,client);
		_round_robin_counter = (_round_robin_counter + 1) % _workers.size();
	}
	else {
		client->close();
	}
}
