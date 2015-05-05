#include <stdio.h>
#include <xthread.h>
#include <xevent.h>

using namespace ZQ;
#include "_sleep.h"

XEvent event(true);

class TestThread : public XThread {

protected:

	virtual exit_status_t run()
	{
		event.Lock();
		printf("new thrad locked\n");
		return (0);
	}

};



class TestThread2 : public XThread {
protected:
	virtual exit_status_t run()
	{
		event.Lock();
		printf("new thrad2 locked\n");
		return (0);
	}
};


TestThread thread;
TestThread2 thread2;

int main(void)
{
	thread.start();
	thread2.start();
	sleep(1);
	event.SetEvent();
	sleep(2);
	printf("main locked\n");
	return 0;
}



