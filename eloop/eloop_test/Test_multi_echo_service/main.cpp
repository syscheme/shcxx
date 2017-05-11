#include "MultiEchoService.h"

int main()
{
	Loop loop(false);
	MultiEchoServer server;
	server.init(loop);
	server.bind4("192.168.81.71",9978);

	server.SetupWorker();
	server.listen();

	loop.run(Loop::Default);
	getchar();
	return 0;
}