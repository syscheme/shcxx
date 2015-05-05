#include "TestSimpleHttp.h"

int main(int argc, char **argv)
{
	LibAsync::HttpClient::setup(5);
	ZQ::common::FileLog log("C:\\testSimpleHttp.log", ZQ::common::Log::L_DEBUG);

	LibAsync::AttrMap reqMap, respMap;
	LibAsync::TestSimpleHttpPtr simpleHttpPtr = new LibAsync::TestSimpleHttp(log, "10.15.10.74");
	simpleHttpPtr->sendLocateRequest("/cacheserver", "cdntest1234567891005xor.com", "index", reqMap, respMap);

	while(true)
	{
		Sleep(10);
	}
	return 0;
}