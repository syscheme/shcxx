#include "worker.h"



int main(int argc,char* argv[])
{
	int id = 0;
	if (argc >= 2)
		int id = atoi(argv[1]);
	Loop loop(false);
	Transport pipe(id);

	pipe.init(loop);
	pipe.open(0);
	pipe.read_start();

	loop.run(Loop::Default);
	return 0;
}