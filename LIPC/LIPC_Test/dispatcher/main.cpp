#include "LIPC.h"


int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC:: Dispatcher dsp;


	dsp.init(loop);
	server.bind4("10.15.10.50",9978);
	server.listen();

	loop.run(Loop::Default);	
}