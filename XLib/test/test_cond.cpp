#include <xcond.h>



#include <stdio.h>

#include <xthread.h>

#include "_sleep.h"



using namespace ZQ;



XCond cond;



class TestThread : public XThread {

public:

	virtual exit_status_t run()

	{

		cond.Lock();

		printf("new thread locked\n");

		cond.Unlock();

		return (0);

	}

};



class TestThread2 : public XThread {

public:

	virtual exit_status_t run()

	{

		cond.Lock();

		printf("new thread2 locked\n");

		cond.Unlock();

		return (0);

	}

};



TestThread thread;

TestThread2 thread2;



int main(void)

{

	thread.start();

	thread2.start();

	//thread.terminate(0);

	sleep(2);

	printf("main Signal\n");

	cond.Broadcast();

	sleep(1);

	//_exit(0);

	//sleep(1);

	return 0;

}



