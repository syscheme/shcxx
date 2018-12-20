#ifndef __RTSP_HANDLER_H__
#define __RTSP_HANDLER_H__

#include "RTSPServer.h"

namespace ZQ {
	namespace eloop {
// ---------------------------------------
// interface RTSPHandler
// ---------------------------------------
class RTSPTestHandler: public RTSPHandler
{
public:
	typedef RTSPApplication<RTSPTestHandler> RTSPTestHandleFactory;

	RTSPTestHandler(IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps = RTSPHandler::Properties())
		:RTSPHandler(app,server,dirProps)
	{
	}
	~RTSPTestHandler(){}
/*
	virtual void	onSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
*/

	// session-based requests
	//@return RTSP status code
	virtual RTSPMessage::ExtendedErrCode procSessionSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionAnnounce(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionDescribe(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);
	virtual RTSPMessage::ExtendedErrCode procSessionGetParameter(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess);

};

}}
#endif