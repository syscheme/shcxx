#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__

#include "RTSPConnection.h"

namespace ZQ {
namespace eloop {

class ZQ_ELOOP_HTTP_API RTSPClient;
// ---------------------------------------
// class RTSPClient
// ---------------------------------------
class RTSPClient : public RTSPConnection 
{
public:
	RTSPClient(InterruptibleLoop& loop, ZQ::common::Log& logger)
		:RTSPConnection(loop, logger){}

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp){}
	virtual void OnRequest(RTSPMessage::Ptr req){}

	virtual void OnRequestPrepared(RTSPMessage::Ptr req) {}
	virtual void OnRequestDone(int cseq, int ret) {}
};

} }//namespace ZQ::eloop
#endif