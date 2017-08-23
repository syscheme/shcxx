#include "LIPC.h"

# define TEST_PIPENAME "\\\\?\\pipe\\uv-test"

class TestRpc
{
public:
	void Print(const ZQ::LIPC::Arbitrary& root,ZQ::LIPC::PipeConnection& conn)
	{
		ZQ::LIPC::Arbitrary response;
		std::cout << "Receive query: " << root << std::endl;
		response["jsonrpc"] = "2.0";
		response["id"] = root["id"];
		response["result"] = "success";
		conn.send(response);
	}

	void Notify(const ZQ::LIPC::Arbitrary& root,ZQ::LIPC::PipeConnection& conn)
	{
		std::cout << "Notification: " << root << std::endl;
		//response = ZQ::LIPC::Arbitrary::null;
	}
};

int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::Service service;

	TestRpc test;
	service.AddMethod(new ZQ::LIPC::RpcMethod<TestRpc>(test, &TestRpc::Print,std::string("print")));
	service.AddMethod(new ZQ::LIPC::RpcMethod<TestRpc>(test, &TestRpc::Notify,std::string("notify")));

	service.init(loop,0);
	service.bind(TEST_PIPENAME);
	service.listen();

	loop.run(ZQ::eloop::Loop::Default);	
}