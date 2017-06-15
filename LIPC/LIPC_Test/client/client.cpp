#include "LIPC.h"
#include "json/json.h"


int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::JsonRpcClient client;

	Json::Value query;
	query["jsonrpc"] = "2.0";
	query["id"] = 1;
	query["method"] = "print";
	client.init(loop);
	client.beginRequest("10.15.10.50",9978,query);
//	client.connect("10.15.10.50",9978);

	loop.run(ZQ::eloop::Loop::Default);	
}