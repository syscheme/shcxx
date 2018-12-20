#include <stdio.h>
#include <xthread.h>

using namespace ZQ;

#include "_sleep.h"

class TestThread : public XThread {
public:
	virtual Exit_Status Run()
	{
		sleep(1);
		printf("testing\n");
		
		return (0);
	}
};

int main(void)
{
	TestThread thread;
	thread.Start();
	//thread.Terminate(0);

	sleep(5);
	return 0;
}
