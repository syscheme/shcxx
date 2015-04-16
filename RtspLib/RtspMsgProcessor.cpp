// FileName : RtspMsgProcessor.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspMsgProcessor.h"
#include "RtspInterface.h"

namespace ZQRtspCommon
{

RtspMsgProcessThread::RtspMsgProcessThread(ZQ::common::NativeThreadPool& pool, 
										   IHandler* handler, 
										   IRtspReceiveMsg* request,
										   IRtspSendMsg* response,
										   ZQ::common::Log& log)
: ThreadRequest(pool), _handler(handler), _request(request), _response(response), _log(log)
{

}

RtspMsgProcessThread::~RtspMsgProcessThread()
{
	_handler = NULL;
	_request = NULL;
	_response = NULL;
}

int RtspMsgProcessThread::run()
{
	bool isSync = _handler->HandleMsg(_request, _response);
	if (isSync)
	{
		_request->release();
		_response->release();
		_request = NULL;
	}
	return 0;
}

void RtspMsgProcessThread::final(int retcode /* = 0 */, bool bCancelled /* = false */)
{
	delete this;
}

} // end for ZQRtspCommon
