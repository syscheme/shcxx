
#include "tcpServer.h"

int main()
{
	Loop loop(true);
	tcpServer server;


	server.init(loop);
	server.bind4("127.0.0.1",9978);
	server.listen();


	loop.run(Loop::Default);
	return 0;
}