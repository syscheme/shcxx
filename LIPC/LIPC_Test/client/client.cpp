#include "LIPC.h"


int main()
{
	ZQ::eloop::Loop loop(false);
	ZQ::LIPC::JsonRpcClient client;

	ZQ::LIPC::Request::Ptr req = new ZQ::LIPC::Request(1,"print");

	client.init(loop);
	client.beginRequest("10.15.10.50",9978,req);
//	client.connect("10.15.10.50",9978);

	loop.run(ZQ::eloop::Loop::Default);	
}