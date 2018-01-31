#include "RTSPServer.h"

#include "SystemUtils.h"
#include <boost/regex.hpp>

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
class RTSPPassiveConn : public RTSPConnection
{
public:
	RTSPPassiveConn(ZQ::common::Log& log, TCPServer* tcpServer):RTSPConnection(log, tcpServer){}
	~RTSPPassiveConn(){}

	virtual void onError( int error,const char* errorDescription );

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp);
	virtual void OnRequest(RTSPMessage::Ptr req);

private:
	RTSPHandler::Ptr		_rtspHandler;

private:
	static void simpleResponse(int code,uint32 cseq,RTSPConnection* conn);
};

// ---------------------------------------
// class RTSPHandler
// ---------------------------------------
void RTSPHandler::onOptions(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	resp->code(200);
	
	resp->header("Server",_server._Config.serverName);
	resp->header("Public","OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, PAUSE");
}

void RTSPHandler::onDescribe(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3012
	std::string url = req->url(); 
	int midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);

	// SDP
	std::string sdp = mediaSDP(mid);

	resp->code(200);

	resp->header("Server", _server._Config.serverName);

	resp->header("Content-base", url);
	resp->header("Content-type", "application/sdp");
	resp->header("Content-length", sdp.size());
	resp->appendBody(sdp.c_str(),sdp.size());
}

void RTSPHandler::onAnnounce(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp) {}

void RTSPHandler::onSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3201/(rtx/audio/video)
	std::string url = req->url(); 

	// mid and stream name
	int urlEndPos = url.rfind("/");
	std::string streamName = url.substr(urlEndPos + 1);
	url = url.erase(urlEndPos);
	int midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);


	RTSPSession::Ptr pSess = createSession(_server.generateSessionID());
	if (pSess == NULL)
	{
		resp->code(500);
		return;
	}
	_server.addSession(pSess);

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
	resp->header("Session", pSess->id());
	resp->header("Transport", strTran);

}

void RTSPHandler::onPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
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
	 resp->header("Session", pSess->id());
	 resp->header("RTP-Info", pSess->streamsInfo());
}

void RTSPHandler::onPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
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

void RTSPHandler::onTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	std::string sid =  req->header("Session");
	RTSPSession::Ptr pSess = _server.findSession(sid);
	if (pSess == NULL)
	{
		resp->code(454);
		return;
	}

	pSess->teardown();
}


// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
void RTSPPassiveConn::onError( int error,const char* errorDescription )
{
}

void RTSPPassiveConn::OnResponse(RTSPMessage::Ptr resp)
{
}

void RTSPPassiveConn::OnRequest(RTSPMessage::Ptr req)
{
	RTSPMessage::Ptr resp = new RTSPMessage(RTSPMessage::RTSP_MSG_RESPONSE);
	resp->cSeq(req->cSeq());
	std::string OnDemandSessionId = req->header("OnDemandSessionId");
	if (!OnDemandSessionId.empty())
		resp->header("OnDemandSessionId", OnDemandSessionId);

	do {
		RTSPServer* pSev = dynamic_cast<RTSPServer*>(_tcpServer);
		if (pSev == NULL)
		{
			resp->code(503);
			break;
		}

		// 	if (0 == req->method().compare("OPTIONS"))		pSev->onOptions(req, resp);
		// 	else if (0 == req->method().compare("ANNOUNCE")) pSev->onAnnounce(req, resp);
		// 	else if (0 == req->method().compare("DESCRIBE")) pSev->onDescribe(req, resp);
		// 	else if (0 == req->method().compare("SETUP"))	pSev->onSetup(req, resp);
		// 	else if (0 == req->method().compare("PLAY"))		pSev->onPlay(req, resp);
		// 	else if (0 == req->method().compare("PAUSE"))	pSev->onPause(req, resp);
		// 	else if (0 == req->method().compare("TEARDOWN"))	pSev->onTeardown(req, resp);
		// 	else
		// 	{
		// 		resp->code(405);
		// 		goto sendResp;
		// 	}

		// 	if (!_rtspHandler)
		// 		_rtspHandler = pSev->createHandler( req->url(), *this);

		_rtspHandler = pSev->createHandler(req->url(), *this);

		if(!_rtspHandler)
		{
			//should make a 404 response
			_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn,"OnRequests failed to find a suitable handle to process url: %s"), req->url().c_str() );
			resp->code(404);
			break;
		}

		if (0 == req->method().compare("OPTIONS"))	{ _rtspHandler->onOptions(req, resp);  break; }
		if (0 == req->method().compare("ANNOUNCE")) { _rtspHandler->onAnnounce(req, resp); break; }
		if (0 == req->method().compare("DESCRIBE")) { _rtspHandler->onDescribe(req, resp); break; }
		if (0 == req->method().compare("SETUP"))    { _rtspHandler->onSetup(req, resp);    break; }
		if (0 == req->method().compare("PLAY"))     { _rtspHandler->onPlay(req, resp);     break; }
		if (0 == req->method().compare("PAUSE"))    { _rtspHandler->onPause(req, resp);    break; }
		if (0 == req->method().compare("TEARDOWN")) { _rtspHandler->onTeardown(req, resp); break; }

		resp->code(405);

	} while(0);

	std::string respMsg = resp->toRaw();
	_Logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), respMsg.size(), hint().c_str(),true);
	printf("send resp[%s]\n",respMsg.c_str());
	write(respMsg.c_str(), respMsg.size());
}

void RTSPPassiveConn::simpleResponse(int code,uint32 cseq,RTSPConnection* conn)
{
	RTSPMessage::Ptr resp = new RTSPMessage(RTSPMessage::RTSP_MSG_RESPONSE);
	resp->code(code);
	resp->cSeq(cseq);
	resp->status(RTSPMessage::code2status(code));

	std::string response = resp->toRaw();
	conn->write(response.c_str(), response.size());
}

// ---------------------------------------
// class RTSPServer
// ---------------------------------------
TCPConnection* RTSPServer::createPassiveConn()
{
	return new RTSPPassiveConn(_Logger,this);
}

bool RTSPServer::mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;
	try {
		dir.re.assign(uriEx);
	}
	catch( const boost::regex_error& )
	{
		_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), vsite.c_str(), uriEx.c_str());
		return false;
	}

	dir.uriEx = uriEx;
	dir.app = app;
	dir.props = props;

	// address the virtual site
	VSites::iterator itSite = _vsites.find(vsite);
	if (_vsites.end() == itSite)
	{
		_vsites.insert(VSites::value_type(vsite, MountDirs()));
		itSite = _vsites.find(vsite);
	}

	itSite->second.push_back(dir);
	return true;
}

RTSPHandler::Ptr RTSPServer::createHandler( const std::string& uri, RTSPPassiveConn& conn, const std::string& virtualSite)
{
	RTSPHandler::AppPtr app = NULL;

	// cut off the paramesters
	std::string uriWithnoParams = uri;
	size_t pos = uriWithnoParams.find_first_of("?#");
	if (std::string::npos != pos)
		uriWithnoParams = uriWithnoParams.substr(0, pos);

	// address the virtual site
	VSites::const_iterator itSite = _vsites.find(virtualSite);
	if (_vsites.end() == itSite)
		itSite = _vsites.find(DEFAULT_SITE); // the default site

	RTSPHandler::Ptr handler;
	MountDirs::const_iterator it = itSite->second.begin();

	for( ; it != itSite->second.end(); it++)
	{
		if (boost::regex_match(uriWithnoParams, it->re))
		{
			if (it->app)
				handler = it->app->create(*this, it->props);
			break;
		}
	}

	return handler;
}

} }//namespace ZQ::eloop