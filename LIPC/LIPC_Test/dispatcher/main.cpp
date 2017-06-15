#include "LIPC.h"


int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::Dispatcher dsp;


	dsp.init(loop);
	dsp.bind4("10.15.10.50",9978);
	dsp.listen();

	loop.run(ZQ::eloop::Loop::Default);	
}