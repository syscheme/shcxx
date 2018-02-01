#include "RTSPServer.h"

#include "Guid.h"
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
RTSPMessage::ExtendedErrCode RTSPHandler::onOptions(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	resp->header("Public","OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, PAUSE");
	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onDescribe(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3012
	std::string url = req->url(); 
	size_t midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);

	// SDP
	std::string sdp = mediaSDP(mid);

	resp->header("Content-base", url);
	resp->header("Content-type", "application/sdp");
	resp->header("Content-length", sdp.size());
	resp->appendBody(sdp.c_str(),sdp.size());

	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onAnnounce(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp)
{
	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetup(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// URL: rtsp://127.0.0.1:9960/3201/(rtx/audio/video)
	std::string url = req->url(); 

	// mid and stream name
	size_t urlEndPos = url.rfind("/");
	std::string streamName = url.substr(urlEndPos + 1);
	url = url.erase(urlEndPos);
	size_t midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);

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

	// TODO: the handler impl here: pSess->setup();

	resp->header("Transport", strTran);
	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPlay(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	std::string strTime = req->header("Range");

	// TODO: the handler impl here: pSess->play();

	 resp->header("RTP-Info", svrsess->streamsInfo());
	 return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPause(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;
	// TODO: the handler impl here: pSess->pause();

	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionTeardown(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here: pSess->teardown();
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionAnnounce(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionDescribe(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionGetParameter(const RTSPMessage::Ptr& req, RTSPMessage::Ptr& resp, RTSPSession::Ptr& sess)
{
	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
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
	std::string method = req->method();

	resp->cSeq(req->cSeq());
	
	//std::string OnDemandSessionId = req->header("OnDemandSessionId");
	//if (!OnDemandSessionId.empty())
	//	resp->header("OnDemandSessionId", OnDemandSessionId);
	int respCode =500;

	do {
		RTSPServer* pSev = dynamic_cast<RTSPServer*>(_tcpServer);
		if (pSev == NULL)
		{
			respCode =503;
			break;
		}

		resp->header("Server", pSev->_Config.serverName);

		// check if the request is session-based or not
		std::string sid =  req->header("Session");
		RTSPSession::Ptr pSess; // should be server-side session

		if (!sid.empty())
		{
			pSess = RTSPServer::Session::Ptr::dynamicCast(pSev->findSession(sid));
			if (NULL == pSess)
			{
				respCode =454;
				break;
			}
		}

		if (NULL == pSess)
		{
			// no-session specified
			_rtspHandler = pSev->createHandler(req->url(), *this);
			if(!_rtspHandler)
			{
				// should make a 404 response
				_Logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn, "OnRequests failed to find a suitable handle to process url: %s"), req->url().c_str() );
				respCode =404;
				break;
			}

			// handle if it is a SETUP
			if (0 == req->method().compare("SETUP")) 
			{ 
				RTSPServer::Session::Ptr sess = pSev->createSession(pSev->generateSessionID());
				pSess = RTSPSession::Ptr::dynamicCast(sess);
				if (NULL == pSess)
				{
					respCode =500;
					break;
				}

				respCode = _rtspHandler->procSessionSetup(req, resp, pSess);
				if (RTSP_RET_SUCC(respCode))
				{
					resp->header("Session",  pSess->id());
					pSev->addSession(sess);
				}

				break;
			}

			// non-session based requests
			if (0 == req->method().compare("OPTIONS"))	{ respCode = _rtspHandler->onOptions(req, resp);  break; }
			if (0 == req->method().compare("ANNOUNCE")) { respCode = _rtspHandler->onAnnounce(req, resp); break; }
			if (0 == req->method().compare("DESCRIBE")) { respCode = _rtspHandler->onDescribe(req, resp); break; }
			break;
		}

		// session-based requests and session has been addressed
		 resp->header("Session",  pSess->id());

		if (0 == req->method().compare("PLAY"))     { respCode = _rtspHandler->procSessionPlay(req, resp, pSess);     break; }
		if (0 == req->method().compare("PAUSE"))    { respCode = _rtspHandler->procSessionPause(req, resp, pSess);    break; }
		if (0 == req->method().compare("TEARDOWN")) { respCode = _rtspHandler->procSessionTeardown(req, resp, pSess); break; }
		if (0 == req->method().compare("ANNOUNCE")) { respCode = _rtspHandler->procSessionAnnounce(req, resp, pSess); break; }
		if (0 == req->method().compare("DESCRIBE")) { respCode = _rtspHandler->procSessionDescribe(req, resp, pSess); break; }

		respCode =405;

	} while(0);

	if (respCode < 100)
		respCode = 500;

	resp->code(respCode);
	std::string respMsg = resp->toRaw();
	_Logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), (int)respMsg.size(), hint().c_str(),true);

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

std::string	RTSPServer::generateSessionID()
{
	char buf[80];
	ZQ::common::Guid guid;
	guid.create();
	guid.toCompactIdstr(buf, sizeof(buf) -2);
	return buf;
}

RTSPServer::Session::Ptr RTSPServer::findSession(const std::string& sessId)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	Session::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		return it->second;
	return NULL;
}

void RTSPServer::addSession(RTSPServer::Session::Ptr sess)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	if (sess)
		_sessMap[sess->id()] = sess;
}

void RTSPServer::removeSession(const std::string& sessId)
{
	ZQ::common::MutexGuard g(_lkSessMap);
	Session::Map::iterator it = _sessMap.find(sessId);
	if (it != _sessMap.end())
		_sessMap.erase(it);
}

size_t RTSPServer::getSessionCount() const
{
	ZQ::common::MutexGuard g(_lkSessMap);
	return _sessMap.size();
}

void RTSPServer::setMaxSession(uint32 maxSession)
{
	_maxSession = maxSession;
}

uint32 RTSPServer::getMaxSession() const
{
	return _maxSession;
}

} }//namespace ZQ::eloop