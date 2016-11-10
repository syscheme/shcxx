#include "tcpClient.h"



int main()
{
	Loop loop(false);

	tcpClient client;
	client.init(loop);
	client.connect4("127.0.0.1",9978);




	loop.run(Loop::Default);
	return 0;
}