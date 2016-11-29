#include "eloop_file.h"


using namespace ZQ::eloop;
class pipeServer:public Pipe
{
public:
	pipeServer(){}
	~pipeServer(){}

	virtual void OnConnection_cb(ElpeError status)
	{
		if (status != ElpeError::elpeSuccess) {
			fprintf(stderr, "New connection error %s\n", Error(status).str());
			return;
		}

		pipeServer *client = new pipeServer();
		client->init(get_loop());

		if (accept((Stream *)client) == 0) {

			client->read_start();
		}
		else {
			client->close();
		}
	}

	virtual void OnRead(ssize_t nread, const char *buf)
	{
		printf("recv data:%s,len = %d\n", buf,nread);

		write(buf,nread);
	}

};

# define TEST_PIPENAME "\\\\?\\pipe\\uv-test"


int main()
{
	Loop loop(true);
	pipeServer server;

	server.init(loop);

	int r = -1;
	r = server.bind(TEST_PIPENAME);
	if (r != 0)
	{
		fprintf(stderr, "Bind error %s\n", Error(r).str());
		return 1;
	}

	r = server.listen();
	if (r != 0)
	{
		fprintf(stderr, "Listen error %s\n", Error(r).str());
		return 1;
	}

	loop.run(Loop::Default);



	return 0;
}