#include "TestHttp.h"

int main(int argc, char **argv)
{
	const char* ip = "10.15.10.73";
	const unsigned short port = 12000;
	const std::string uri	= "/scs/getfile?file=cdntest1234567892010xor.com.0X0000&ic=10000000000&rate=375000&range=0-1048577";
	const unsigned int maxHttpClient = 400;

	ZQ::common::FileLog log("testHttp.log", ZQ::common::Log::L_DEBUG);

	LibAsync::HttpMessagePtr sendMsgPtr = new LibAsync::HttpMessage(http_parser_type::HTTP_REQUEST);
	sendMsgPtr->method(http_method::HTTP_GET);
	sendMsgPtr->url(uri);
	sendMsgPtr->keepAlive(true);

	LibAsync::TestHttpClient::setup(1);
	std::vector<LibAsync::TestHttpClientPtr> httpMgr;
	httpMgr.resize(maxHttpClient);

	for (int i = 0; i < maxHttpClient; i++)
	{
		LibAsync::TestHttpClient* pHttpClient = new LibAsync::TestHttpClient(log, i+1);
		httpMgr.push_back(pHttpClient);

		if(!pHttpClient->beginRequest(sendMsgPtr, ip, port))
		{
			log(ZQ::common::Log::L_ERROR, CLOGFMT(TestHttpClient, "[Index = %5d, Cycle = %2d] failue to send request"), i + 1, 1);
		}
	}

	while (true)
	{
		Sleep(10);
	}
	return 0;
}