#include "RtspHandler.h"

namespace ZQ {
	namespace eloop {

void RTSPTestHandler::onSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	RTSPSession::Ptr pSess = createSession(_server.generateSessionID());
	if (pSess == NULL)
	{
		resp->code(500);
		return;
	}
	_server.addSession(pSess);

//	SessionGroup = req->header("SessionGroup");

	pSess->setSessionGroup(req->header("SessionGroup"));

	std::string strTran = req->header( "Transport" );

	if(strTran.find("TCP") != std::string::npos)
	{

	}
	else if(strTran.find("multicast") != std::string::npos)
	{

	}
	else		//UDP
	{

	}

	pSess->setup();

	resp->code(200);
	resp->header("Server", _server._Config.serverName);
	resp->header("Method-Code", "SETUP");
	resp->header("Session", pSess->id());
	resp->header("Transport", "MP2T/DVBC/UDP;unicast;destination=127.0.0.1;client_port=514;bandwidth=3750000;sop_group=NGODTEST;sop_name=NGODTEST;source=127.0.0.1;server_port=0");
	std::string body = "v=0\r\n\
					   o=- 496455117 3722643886 IN IP4 172.16.20.28\r\n\
					   s=\r\n\
					   c=IN IP4 172.16.20.28\r\n\
					   t=0 0\r\n\
					   a=control:rtsp://172.16.20.28:554/496455117\r\n";
	resp->contentLength(body.size());
	resp->appendBody(body.c_str(), body.size());
}

void RTSPTestHandler::onPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	std::string sid =  req->header("Session");
	RTSPSession::Ptr pSess = _server.findSession(sid);
	if (pSess == NULL)
	{
		resp->code(454);
		return;
	}
	std::string strTime = req->header("Range");

	pSess->play();


	resp->code(200);
	resp->header("Server", _server._Config.serverName);
	resp->header("Method-Code", "PLAY");
	resp->header("Session", pSess->id());

	resp->header("SessionGroup", pSess->getSessionGroup());
	resp->header("User-Agent", "SeaChangeRTSPClient");
}

void RTSPTestHandler::onPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	std::string sid =  req->header("Session");
	RTSPSession::Ptr pSess = _server.findSession(sid);
	if (pSess == NULL)
	{
		resp->code(454);
		return;
	}

	pSess->pause();
}

void RTSPTestHandler::onTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	std::string sid =  req->header("Session");
	RTSPSession::Ptr pSess = _server.findSession(sid);
	if (pSess == NULL)
	{
		resp->code(454);
		return;
	}

	pSess->teardown();

	resp->code(200);
	resp->header("Server", _server._Config.serverName);
	resp->header("Method-Code", "TEARDOWN");

	resp->header("Range", "npt=60.202-3600.000");
	resp->header("Session", pSess->id());
	resp->header("Content-Type", "text/xml");
	std::string body = "<ResponseData>\r\n		\
		<ODRMSessionHistory>\r\n				\
		</ODRMSessionHistory>\r\n				\
		</ResponseData>\r\n";

	resp->appendBody(body.c_str(),body.size());
}

}}