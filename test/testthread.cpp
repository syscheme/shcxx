#include "..\thread.h"
#include <iostream>
class testthread: public ZQ::common::Thread
{
	int run()
	{
		DWORD dwTest = 12;

		static int i=0;
		i++;
		printf("thread XXX....,%d\r\n",i);
		Sleep(100);
		printf("Thread after Sleep %d\r\n",i);
		return 0;
	}
};

int test_thread(int thread_count)
{
	testthread *pthreads=new testthread[thread_count];
	for(int i=0;i<thread_count;i++)
	{
		bool isok=pthreads[i].start();
		if(!isok)
		{
			std::cout<<"error"<<std::endl;
		}
	}
	Sleep(2000);
	for(int i=0;i<thread_count;i++)
	{
		pthreads[i].wait();
		pthreads[i].close_handle();
	}

	return 0;
}

int main()
{
	test_thread(10);
	return 0;
}