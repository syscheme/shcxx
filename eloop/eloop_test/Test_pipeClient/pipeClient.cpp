#include "eloop_file.h"


using namespace ZQ::eloop;
class pipeClient:public Pipe
{
public:
	pipeClient(){}
	~pipeClient(){}


	virtual void OnConnected(ElpeError status)
	{
		if (status != ElpeError::elpeSuccess) {
			fprintf(stderr, "on_connect error %s\n", Handle::errDesc(status));
			return;
		}
		read_start();

		char buf[1024];
		memset(buf,0,1024);
		scanf("%s",buf);
		

		write(buf,strlen(buf)+1);
	}

	virtual void OnRead(ssize_t nread, const char *buf)
	{
		printf("recv data:%s,len = %d\n", buf,nread);

		char sendbuf[1024];
		memset(sendbuf,0,1024);
		scanf("%s",sendbuf);

		write(sendbuf,strlen(sendbuf)+1);
	}

};

# define TEST_PIPENAME "\\\\?\\pipe\\uv-test"


int main()
{
	Loop loop(true);
	pipeClient client;

	client.init(loop);


	client.connect(TEST_PIPENAME);

	loop.run(Loop::Default);
	return 0;
}