// FileName : RtspMsgProcessor.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#ifndef __ZQ_RTSP_COMMON_MSG_PROCESSOR_H__
#define __ZQ_RTSP_COMMON_MSG_PROCESSOR_H__

#include "NativeThreadPool.h"

namespace ZQRtspCommon
{

class IHandler;
class IRtspReceiveMsg;
class IRtspSendMsg;

class RtspMsgProcessThread : public ZQ::common::ThreadRequest
{
public:
	RtspMsgProcessThread(ZQ::common::NativeThreadPool& pool, IHandler* handler, 
		IRtspReceiveMsg* request, IRtspSendMsg* response, ZQ::common::Log& log);
	~RtspMsgProcessThread();
protected:
	virtual int run(void);
	virtual void final(int retcode = 0, bool bCancelled = false);
private:
	IHandler* _handler;
	IRtspReceiveMsg* _request;
	IRtspSendMsg* _response;
	ZQ::common::Log& _log;
};

}// end for ZQRtspCommon

#endif // end for #ifndef __ZQ_RTSP_ENGINE_MSG_PROCESSOR_H__
