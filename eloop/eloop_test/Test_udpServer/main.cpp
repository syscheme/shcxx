#include "udpServer.h"



int main()
{
	Loop loop(true);

	udpServer server;
	server.init(loop);
	server.bind4("127.0.0.1",9978,UDP::Reuseaddr);
	server.recv_start();

	loop.run(Loop::Default);
	return 0;
}