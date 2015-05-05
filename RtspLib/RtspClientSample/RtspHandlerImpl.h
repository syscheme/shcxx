// FileName : RtspHandlerImpl.h
// Author   : Zheng Junming
// Date     : 2009-11
// Desc     : 


#ifndef __ZQ_RTSP_COMMON_INTERFACE_HANDLER_IMPL_SAMPLE_H__
#define __ZQ_RTSP_COMMON_INTERFACE_HANDLER_IMPL_SAMPLE_H__

#include "Log.h"
#include "RtspInterface.h"

namespace RtspSample
{

class RtspHandlerImpl : public ZQRtspCommon::IHandler
{
public:
	RtspHandlerImpl(ZQ::common::Log& log);
	virtual ~RtspHandlerImpl();
public:
	virtual bool HandleMsg(ZQRtspCommon::IRtspReceiveMsg* receiveMsg, ZQRtspCommon::IRtspSendMsg* sendMsg);
private:
	virtual void testRequest(ZQRtspCommon::IRtspReceiveMsg* receiveMsg, ZQRtspCommon::IRtspSendMsg* sendMsg);
	virtual void testResponse(ZQRtspCommon::IRtspReceiveMsg* receiveMsg, ZQRtspCommon::IRtspSendMsg* sendMsg);
private:
	ZQ::common::Log& _log;

};

} // end for RtspSample

#endif // __ZQ_RTSP_COMMON_INTERFACE_HANDLER_IMPL_SAMPLE_H__