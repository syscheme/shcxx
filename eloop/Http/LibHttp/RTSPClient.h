#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__

#include "RTSPConnection.h"

namespace ZQ {
namespace eloop {

class ZQ_HTTP_API RTSPClient;
// ---------------------------------------
// class RTSPClient
// ---------------------------------------
class RTSPClient : public RTSPConnection 
{
public:
	RTSPClient(ZQ::common::Log& logger)
		:RTSPConnection(logger){}

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp){}
	virtual void OnRequest(RTSPMessage::Ptr req){}

	virtual void OnRequestPrepared(RTSPMessage::Ptr req) {}
	virtual void OnRequestDone(int cseq, int ret) {}
};

} }//namespace ZQ::eloop
#endif