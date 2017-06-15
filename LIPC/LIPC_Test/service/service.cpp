#include "LIPC.h"
#include "json.h"

class TestRpc
{
  public:
    bool Print(const Json::Value& root, Json::Value& response)
	{
		  std::cout << "Receive query: " << root << std::endl;
		  response["jsonrpc"] = "2.0";
		  response["id"] = root["id"];
		  response["result"] = "success";
		  return true;
	}

    bool Notify(const Json::Value& root, Json::Value& response)
	{
		  std::cout << "Notification: " << root << std::endl;
		  response = Json::Value::null;
	}

    Json::Value GetDescription()
	{
		Json::FastWriter writer;
		Json::Value root;
		Json::Value parameters;
		Json::Value param1;

		root["description"] = "Print";

		/* type of parameter named arg1 */
		param1["type"] = "integer";
		param1["description"] = "argument 1";

		/* push it into the parameters list */
		parameters["arg1"] = param1;
		root["parameters"] = parameters;

		/* no value returned */
		root["returns"] = Json::Value::null;
	}
};

int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::JsonRpcService service;

	server.AddMethod(new Json::Rpc::RpcMethod<TestRpc>(a, &TestRpc::Print,std::string("print")));
	server.AddMethod(new Json::Rpc::RpcMethod<TestRpc>(a, &TestRpc::Notify,std::string("notify")));
		
	service.init(loop);
	service.bind4("/home/zhixiang.zhu/var/run/service_test");
	service.listen();

	loop.run(ZQ::eloop::Loop::Default);	
}