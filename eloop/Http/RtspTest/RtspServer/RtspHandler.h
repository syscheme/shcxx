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

	virtual void	onSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);
	virtual void	onTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp);

};

}}
#endif