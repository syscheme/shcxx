#include "..\nativethread.h"
#include <iostream>

class testthread: public ZQ::common::NativeThread
{

	int run()
	{
		DWORD dwTest = 12;

		int intv = 100*(id() % 5 +5);
		intv = 100*5;

		while (true)
		{
			printf("thread %04x heart beat    \n",id());
			Sleep(intv);
		}

		return 0;
	}
};

int test_thread(int thread_count)
{
	testthread *pthreads=new testthread[thread_count];
	int i;
	for(i=0;i<thread_count;i++)
	{
		bool isok=pthreads[i].start();
		if(!isok)
		{
			std::cout<<"error"<<std::endl;
		}
	}
	Sleep(2000);
	for(i=0;i<thread_count;i++)
	{
/*
		pthreads[i].wait();
		pthreads[i].close_handle();
*/
//		pthreads[i].terminate();
	}

	delete[] pthreads;

	return 0;
}

int test_thread2(int thread_count)
{
	testthread threads[20];

	thread_count = (thread_count<20) ? thread_count : 20;
	int i;
	for(i=0;i<thread_count;i++)
	{
		bool isok=threads[i].start();
		if(!isok)
		{
			std::cout<<"error"<<std::endl;
		}
	}
	Sleep(2000);
	printf("suspend %04x and %04x\n", threads[0].id(), threads[1].id());
	threads[0].suspend();
	threads[1].suspend();

	Sleep(2000);
	printf("resume %04x\n", threads[0].id());
	threads[0].resume();

	Sleep(2000);
	return 0;
}


int main()
{
	test_thread2(3);
	return 0;
}
