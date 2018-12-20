#include "NativeThreadPool.h"

static int count =0;
class MyReq : public ZQ::common::ThreadRequest
{
public:
	MyReq(ZQ::common::NativeThreadPool& Pool, int id) : _id(id), ThreadRequest(Pool) {}

protected:

	int run(void)
	{
		int c = (int) ((float)_id*12345.739) % 9 +1;
		for (int i =0; i< 99999; i++);
//		for (int i =0; i< c; i++)
//		{
//			printf("req(%d) thrd(%08x) %i/%i\r", _id, threadId(), i, c);
//			::Sleep(1);
//		};
//		printf("req(%06d) thrd(%04x) finished      \r", _id, threadId());
		count++;

		return 0;
	}

	void final(int retcode, bool bCancelled)
	{
		delete this;
	}

	int _id;
};

int main(int argc, char* argv[])
{
//	::Sleep(5000);
	ZQ::common::NativeThreadPool pool(50);
//	::Sleep(5000);

	int s= pool.size();

	for (int j=0; j< 10000; j++)
	{
		for (int i=0; i< 5; i++)
		{
			MyReq* req = new MyReq(pool, (5 * j) +i);
			req->start();
		}
		
//		::Sleep(1);
	}
	printf("start\n");
	int c = count;
	::Sleep(10000);
	printf("\ncount=%d; %.2freq/sec\n",count-c, (float)(count-c)/10);
	return 0;
}
