#include "LIPC.h"

class TestRpc
{
  public:
    bool Print(const ZQ::LIPC::Arbitrary& root, ZQ::LIPC::Arbitrary& response)
	{
		  std::cout << "Receive query: " << root << std::endl;
		  response["jsonrpc"] = "2.0";
		  response["id"] = root["id"];
		  response["result"] = "success";
		  return true;
	}

    bool Notify(const ZQ::LIPC::Arbitrary& root, ZQ::LIPC::Arbitrary& response)
	{
		  std::cout << "Notification: " << root << std::endl;
		  response = ZQ::LIPC::Arbitrary::null;
	}
};

int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::JsonRpcService service;

	TestRpc test;
	service.AddMethod(new ZQ::LIPC::RpcMethod<TestRpc>(test, &TestRpc::Print,std::string("print")));
	service.AddMethod(new ZQ::LIPC::RpcMethod<TestRpc>(test, &TestRpc::Notify,std::string("notify")));
		
	service.init(loop);
	service.bind("/home/zhixiang.zhu/var/run/service_test");
	service.listen();

	loop.run(ZQ::eloop::Loop::Default);	
}