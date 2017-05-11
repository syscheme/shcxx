#include "worker.h"



int main()
{
	Loop loop(false);
	Transport pipe;

	pipe.init(loop);
	pipe.open(0);
	pipe.read_start();

	loop.run(Loop::Default);
	return 0;
}