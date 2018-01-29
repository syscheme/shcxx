#ifndef __RTSP_CLIENT_TEST_H__
#define __RTSP_CLIENT_TEST_H__

#include "RTSPClient.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class RTSPClientTest
// ---------------------------------------
class RTSPClientTest : public RTSPClient
{
public:
	RTSPClientTest(ZQ::common::Log& logger,int64 timeout =500)
		:RTSPClient(logger,timeout){}

};



// ---------------------------------------
// class ClientSession
// ---------------------------------------
class ClientSession : public RTSPSession
{
public:
	ClientSession(){}


	int setup(RTSPClientTest& client);


};


} }//namespace ZQ::eloop
#endif