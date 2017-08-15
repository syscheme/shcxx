#include "LIPC.h"

# define TEST_PIPENAME "\\\\?\\pipe\\uv-test"

void OnTest(const ZQ::LIPC::Arbitrary& param)
{
	std::cout << "Receive OnTest: " << param << std::endl;
}


class TestClient:public ZQ::LIPC::JsonRpcClient
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
		std::string seqId = "client-1";
		ZQ::LIPC::Arbitrary req;
		req[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
		req[JSON_RPC_METHOD] = "print";
		req[JSON_RPC_ID] = seqId;

		Addcb(seqId,OnTest);

		sendRequest(req);
	}
};

int main()
{
	ZQ::eloop::Loop loop(false);
	TestClient client;

	client.init(loop,0);
	client.connect(TEST_PIPENAME);



	loop.run(ZQ::eloop::Loop::Default);	
	printf("1111111111111\n");
	getchar();
}