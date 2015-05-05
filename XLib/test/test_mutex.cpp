#include <xmutex.h>



#include <stdio.h>

#include <xthread.h>



using namespace ZQ;



#include "_sleep.h"



XMutex mutex;



class TestThread : public XThread {

public:

	virtual exit_status_t run()

	{

		mutex.Lock(1000);

		printf("new thrad locked\n");

		//mutex.Unlock();

		

		return (0);

	}

};



int main(void)

{

	TestThread thread;

	mutex.Lock();

	printf("Lock 1\n");

	mutex.Unlock();

	mutex.Lock();

	printf("Lock 2\n");

	thread.start();

	sleep(2);

	mutex.Unlock();



	//thread.terminate(0);

	

	

	printf("main locked\n");

	return 0;

}



