#include <iostream>
#include "GetMainFile.h"

ZQ::common::FileLog g_log("testMainFile.log", ZQ::common::Log::L_DEBUG);

int main(int argc, char** argv)
{
#ifdef ZQ_OS_MSWIN
    std::string bindIP = "192.168.86.140";
    std::string clientTransfer = "192.168.86.140";
#else
    std::string bindIP = "10.15.10.50";
    std::string clientTransfer = "10.15.10.50";
#endif

    std::string httpcrgAddr = "10.15.10.74";
    const uint  httpcrgPort = 10080;
    std::string sendUrl = "/cacheserver";
    std::string contentName = "cdntest1234567891003xor.com";
    std::string subType = "index";

    LibAsync::HttpClient::setup(5);
    ZQ::StreamService::AttrMap reqProps;

    ZQ::StreamService::C2ClientAsyncPtr c2cltPtr = new ZQ::StreamService::C2ClientAsync(g_log, bindIP, clientTransfer,httpcrgAddr, httpcrgPort);
    
    if (!c2cltPtr->sendLocateRequest(sendUrl, contentName, subType, reqProps))
    {
        g_log(ZQ::common::Log::L_ERROR, CLOGFMT(Main, "send locate request failure"));
        std::cout<<"send locate request failure"<<std::endl;
    }
    
    while(true)
    {
        SYS::sleep(10);
    }
    return 0;
}