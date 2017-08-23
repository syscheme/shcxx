#include "LIPC.h"

# define TEST_PIPENAME "\\\\?\\pipe\\uv-test"

void OnTest(const ZQ::LIPC::Arbitrary& param)
{
	std::cout << "Receive OnTest: " << param << std::endl;
}


class TestClient:public ZQ::LIPC::Client
{
public:
	virtual void OnConnected(ElpeError status)
	{
		if (status != ElpeError::elpeSuccess) {
			fprintf(stderr, "on_connect error %s\n", Handle::errDesc(status));
			return;
		}
		read_start();
		printf("OnConnected\n");

		sendRequest("print",ZQ::LIPC::Arbitrary::null,cbTest,this);
	}

	virtual void OnWrote(int status)
	{
		printf("OnWrote status = %d\n",status);
	}

	virtual void OnTest()
	{
		printf("OnTest\n");
		_test.push_back("OnTest");
	}

private:
	static void cbTest(const ZQ::LIPC::Arbitrary& param,void* data)
	{
		printf("cbTest\n");
		TestClient* self = static_cast<TestClient *>(data);
		if (self)
		{
			self->OnTest();
		}
	}
	std::vector<std::string> _test;
};

int main()
{
	ZQ::eloop::Loop loop(false);
	TestClient client;

	client.init(loop,0);
	client.connect(TEST_PIPENAME);



	loop.run(ZQ::eloop::Loop::Default);	
	getchar();
}