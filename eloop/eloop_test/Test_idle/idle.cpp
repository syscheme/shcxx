#include "eloop.h"

using namespace ZQ::eloop;

class MyIdle : public Idle
{
public:
	MyIdle() {}
	~MyIdle() {}
	virtual void OnIdle()
	{
		printf("Idle test\n");
		Sleep(1000);
	}
};

int main()
{
	Loop loop(true);

	MyIdle idle;
	idle.init(loop);
	idle.start();

	loop.run(Loop::Default);

	return 0;
}
