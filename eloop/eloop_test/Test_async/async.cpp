#include <stdlib.h>

#include "eloop.h"

using namespace ZQ::eloop;

class MyAsync : public Async
{
public:
	MyAsync() {}
	~MyAsync() {}
	virtual void OnAsync()
	{
		printf("Async test\n");
	}

protected:
	asdsfasf
};

int main()
{
	Loop loop(true);

	MyAsync async;
	async.init(loop);
	async.send();

	loop.run(Loop::Default);
	return 0;
}
