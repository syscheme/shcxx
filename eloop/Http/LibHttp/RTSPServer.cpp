#include "RTSPServer.h"
#include "TimeUtil.h"
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
	RTSPPassiveConn(RTSPServer& server)
		: RTSPConnection(server._logger, NULL, &server), _server(server)
	{}

	virtual ~RTSPPassiveConn(){}

	virtual void OnTimer(){}

	virtual void onError( int error,const char* errorDescription );

	virtual void	onDataSent(size_t size);
	virtual void	onDataReceived( size_t size );

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp);
	virtual void OnRequest(RTSPMessage::Ptr req);

protected:
	RTSPHandler::Ptr _rtspHandler;
	RTSPServer&  _server;

private:
	static void simpleResponse(int code,uint32 cseq,RTSPConnection* conn);
};

// ---------------------------------------
// class RTSPHandler
// ---------------------------------------
void RTSPHandler::Session::destroy() 
{
	_server.removeSession(_id);
}

RTSPHandler::RTSPHandler(const RTSPMessage::Ptr& req, IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps)
: _app(app), _server(server), _dirProps(dirProps), _req(req)
{
	_app._cOngoings.inc();
}

RTSPHandler::~RTSPHandler()
{
	_app._cOngoings.dec();
}

void RTSPHandler::onDataSent(size_t size)
{
}

void RTSPHandler::onDataReceived( size_t size )
{
}

std::string RTSPHandler::mediaSDP(const std::string& mid) { return "";}

RTSPMessage::ExtendedErrCode RTSPHandler::onOptions(RTSPResponse::Ptr& resp)
{
	resp->header("Public","OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, PAUSE");
	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onDescribe(RTSPResponse::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3012
	std::string url = _req->url(); 
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

RTSPMessage::ExtendedErrCode RTSPHandler::onAnnounce(RTSPResponse::Ptr& resp)
{
	return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onGetParameter(RTSPResponse::Ptr& resp)
{
	return RTSPMessage::rcMethodNotAllowed;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onSetParameter(RTSPResponse::Ptr& resp)
{
	return RTSPMessage::rcOK; // dummy yes
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetup(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// URL: rtsp://127.0.0.1:9960/3201/(rtx/audio/video)
	std::string url = _req->url(); 

	// mid and stream name
	size_t urlEndPos = url.rfind("/");
	std::string streamName = url.substr(urlEndPos + 1);
	url = url.erase(urlEndPos);
	size_t midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);

	std::string strTran = _req->header( "Transport" );

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

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPlay(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	std::string strTime = _req->header("Range");

	// TODO: the handler impl here: pSess->play();

	 resp->header("RTP-Info", svrsess->streamsInfo());
	 return RTSPMessage::rcOK;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPause(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;
	// TODO: the handler impl here: pSess->pause();

	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionTeardown(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here: pSess->teardown();
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionAnnounce(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionDescribe(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionGetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
{
	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

// ---------------------------------------
// class RTSPResponse
// ---------------------------------------
RTSPResponse::RTSPResponse(RTSPServer& server,const RTSPMessage::Ptr& req)
: _server(server), RTSPMessage(req->getConnId(), RTSPMessage::RTSP_MSG_RESPONSE),_req(req),_isResp(false)
{
	cSeq(req->cSeq());
	_server.addReq(this);
}

int RTSPResponse::getTimeLeft()
{
	if (_server._config.procTimeout <=0)
		return DUMMY_PROCESS_TIMEOUT;

	return (int)(_server._config.procTimeout - _req->elapsed());
}

TCPConnection* RTSPResponse::getConn() 
{	
	return _server.findConn(getConnId()); 
}

void RTSPResponse::post(int statusCode, const char* errMsg, bool bAsync) 
{
	{
		// to avoid double post response
		ZQ::common::MutexGuard g(_lkIsResp);
		if (_isResp)
			return;
		_isResp = true;
	}

	_server.removeReq(this);

	std::string reqId = header(Header_RequestId);
	if (reqId.empty())
	{
		char tmp[100];
		snprintf(tmp, sizeof(tmp)-2, "%s(%d)@%s", methodToStr(_req->method()), _req->cSeq(), _req->getConnId().c_str());
		reqId = tmp;
		header(Header_RequestId, reqId);
	}

	std::string txn = std::string("resp-of-req[") + reqId + "]";

	if (statusCode < 100 || statusCode >999)
	{
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPResponse, "post() statusCode(%d) in %s, taking '500 ServerError' instead"), statusCode, txn.c_str());
		statusCode = rcInternalError;
	}

	code(statusCode);

	if (errMsg != NULL)
		status(errMsg);

	std::string respMsg = toRaw();
	// TODO: _conn._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), _conn.hint().c_str(),true);

	TCPConnection* conn = _server.findConn(getConnId());
	if (conn == NULL)
	{
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPResponse, "post() drop %s per conn[%s] already closed"), txn.c_str(), getConnId().c_str());
		return;
	}

	int ret = 0;
	if (bAsync)
		ret = conn->AsyncSend(respMsg);
	else
	{
		if (TCPConnection::_enableHexDump > 0)
			_server._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), txn.c_str() ,true);
		ret = conn->write(respMsg.c_str(), respMsg.size());
	}
	 
	if (ret < 0)
	{
		conn->onError(ret,ZQ::eloop::Handle::errDesc(ret));
		return;
	}

	_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPResponse, "post() %s ret(%d) took %dms"), txn.c_str(), statusCode, _req->elapsed());
}

// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
void RTSPPassiveConn::onError( int error,const char* errorDescription )
{
	if(_rtspHandler)
		_rtspHandler->onError(error,errorDescription);
}

void RTSPPassiveConn::onDataSent(size_t size)
{
	if(_rtspHandler)
		_rtspHandler->onDataSent(size);
}

void RTSPPassiveConn::onDataReceived( size_t size )
{
	if(_rtspHandler)
		_rtspHandler->onDataReceived(size);
}

void RTSPPassiveConn::OnResponse(RTSPMessage::Ptr resp)
{
}

void RTSPPassiveConn::OnRequest(RTSPMessage::Ptr req)
{
	RTSPResponse::Ptr resp = new RTSPResponse(_server, req);

	int respCode = RTSPMessage::rcInternalError;
	std::string sessId;

	do {
		sessId =  req->header(Header_Session);
		resp->header(Header_Server, _server._config.serverName);
		resp->header(Header_Session,  sessId);

		int pendings =  _server.getPendingRequest();
		if (pendings >2)
		{
			if (_server._config.maxPendings >2 && pendings >= _server._config.maxPendings)
			{
				_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn, "OnRequest() rejecting %s(%d) per too many pendings [%d /%d]"), RTSPMessage::methodToStr(req->method()), req->cSeq(), pendings, _server._config.maxPendings);
				respCode = RTSPMessage::rcServiceUnavail;
				break;
			}

			_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() enqueuing %s(%d) into pendings[%d /%d]"), RTSPMessage::methodToStr(req->method()), req->cSeq(), pendings, (int)_server._config.maxPendings);
		}

		_rtspHandler = _server.createHandler(req, *this);
		if(!_rtspHandler)
		{
			// should make a 404 response
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn, "OnRequest() failed to find a suitable handle to process url: %s"), req->url().c_str() );
			respCode = RTSPMessage::rcObjectNotFound;
			break;
		}

		// check if the request is session-based or not
		RTSPSession::Ptr pSess = NULL; // should be server-side session

		if (!sessId.empty())
		{
			resp->header(Header_Session,  sessId);
			pSess = _server.findSession(sessId);//RTSPServer::Session::Ptr::dynamicCast(_server.findSession(sid));
			if (NULL == pSess)
			{
				respCode = RTSPMessage::rcSessNotFound;
				break;
			}
		}

		if (NULL == pSess)
		{
			// handle if it is a SETUP
			if (RTSPMessage::mtdSETUP == req->method()) 
			{ 
				sessId = _server.generateSessionID();
				resp->header(Header_Session,  sessId);
				RTSPServer::Session::Ptr sess = _rtspHandler->_app.newSession(_server, sessId.c_str());
				if (sess == NULL)
				{
					if (_tcpServer)
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

					respCode = RTSPMessage::rcInternalError;
					break;
				}
				
				pSess = RTSPSession::Ptr::dynamicCast(sess);
				if (NULL == pSess)
				{
					if (_tcpServer)
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

					respCode = RTSPMessage::rcInternalError;
					break;
				}

				if (_tcpServer)
					_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() building up new session[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

				respCode = _rtspHandler->procSessionRequest(RTSPMessage::mtdSETUP, resp, pSess);
				if (RTSPMessage::Err_AsyncHandling == respCode)
				{
					// the request is current being handled async-ly, add the session and quit the processing
					resp->header(Header_Session,  pSess->id());
					_server.addSession(sess);
					return;
				}

				if (RTSP_RET_SUCC(respCode))
				{
					resp->header(Header_Session,  pSess->id());
					_server.addSession(sess);
				}

				break;
			}

			// non-session based requests
			respCode =RTSPMessage::rcMethodNotAllowed;
			switch(req->method())
			{
			case RTSPMessage::mtdOPTIONS:       respCode = _rtspHandler->onOptions(resp); break;
			case RTSPMessage::mtdANNOUNCE:      respCode = _rtspHandler->onAnnounce(resp); break;
			case RTSPMessage::mtdDESCRIBE:      respCode = _rtspHandler->onDescribe(resp); break;
			case RTSPMessage::mtdGET_PARAMETER: respCode = _rtspHandler->onGetParameter(resp); break;
			case RTSPMessage::mtdSET_PARAMETER: respCode = _rtspHandler->onSetParameter(resp); break;
			default:
				break;
			}

			if (RTSPMessage::Err_AsyncHandling == respCode)
				return;
			
			break;
		}

		// session-based requests and session has been addressed
		resp->header(Header_Session,  pSess->id());

		// if (0 == req->method().compare(Method_PLAY))     { respCode = _rtspHandler->procSessionPlay(req, resp, pSess);     if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_PAUSE))    { respCode = _rtspHandler->procSessionPause(req, resp, pSess);    if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_ANNOUNCE)) { respCode = _rtspHandler->procSessionAnnounce(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_DESCRIBE)) { respCode = _rtspHandler->procSessionDescribe(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_TEARDOWN)) { respCode = _rtspHandler->procSessionTeardown(req, resp, pSess); _server.removeSession(pSess->id()); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_GetParameter)) { respCode = _rtspHandler->procSessionGetParameter(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		// if (0 == req->method().compare(Method_SetParameter)) { respCode = _rtspHandler->procSessionSetParameter(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }

		// respCode =405;

		respCode = _rtspHandler->procSessionRequest(req->method(), resp, pSess);
		if (RTSPMessage::Err_AsyncHandling == respCode)
			return; 

	} while(0);

	if (respCode < 100)
		respCode = RTSPMessage::rcInternalError;

	resp->post(respCode, NULL, false);

// 	resp->code(respCode);
// 	std::string respMsg = resp->toRaw();
// 	write(respMsg.c_str(), respMsg.size());
// 
// 	if (TCPConnection::_enableHexDump > 0)
// 		_logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), (int)respMsg.size(), hint().c_str(),true);
// 	int64 elapsed = ZQ::common::now() - req->_stampCreated;
// 	
// 	if (_tcpServer)
// 		_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() sessId[%s] %s(%d) ret(%d) took %lldms"), sessId.c_str(), RTSPMessage::methodToStr(req->method()), req->cSeq(), respCode, elapsed);
// 
// 	_server.removeReq(resp->get());
}

void RTSPPassiveConn::simpleResponse(int code,uint32 cseq,RTSPConnection* conn)
{
	RTSPMessage::Ptr resp = new RTSPMessage("",RTSPMessage::RTSP_MSG_RESPONSE);
	resp->code(code);
	resp->cSeq(cseq);
	resp->status(RTSPMessage::code2status(code));

	std::string response = resp->toRaw();
	conn->write(response.c_str(), response.size());
}

// ---------------------------------------
// class RTSPServer
// ---------------------------------------
RTSPServer::RTSPServer( const TCPServer::ServerConfig& conf, ZQ::common::Log& logger, uint32 maxSess)
:TCPServer(conf, logger), _maxSession(maxSess)
{}

RTSPServer::~RTSPServer(){}

TCPConnection* RTSPServer::createPassiveConn()
{
	return new RTSPPassiveConn(*this);
}

bool RTSPServer::mount(const std::string& uriEx, RTSPHandler::AppPtr app, const RTSPHandler::Properties& props, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	MountDir dir;

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

bool RTSPServer::unmount(const std::string& uriEx, const char* virtualSite)
{
	std::string vsite = (virtualSite && *virtualSite) ? virtualSite :DEFAULT_SITE;

	// address the virtual site
	bool ret = false;
	VSites::iterator itSite = _vsites.find(vsite);
	if (_vsites.end() == itSite)
		return ret;

	for(MountDirs::iterator it = itSite->second.begin(); it < itSite->second.end();)
	{
		if (it->uriEx == uriEx)
		{
			it = itSite->second.erase(it);
			ret = true;
		}
		else  it++;
	}

	return ret;
}

RTSPHandler::Ptr RTSPServer::createHandler(const RTSPMessage::Ptr& req, RTSPPassiveConn& conn, const std::string& virtualSite)
{
	RTSPHandler::AppPtr app = NULL;

	// cut off the paramesters
	std::string uriWithnoParams = req->url();
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
		boost::regex re;
		try {
			re.assign(it->uriEx);
		}
		catch( const boost::regex_error& )
		{
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), virtualSite.c_str(), it->uriEx.c_str());
			continue;
		}

		if (boost::regex_match(uriWithnoParams, re))
		{
			if (it->app)
				handler = it->app->create(*this, req, it->props);

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
	if (!sess)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPServer, "addSession() session is null"));
		return;
	}

	size_t sessSize = 0;
	{
		ZQ::common::MutexGuard g(_lkSessMap);
		Session::Map::iterator it = _sessMap.find(sess->id());
		if (it != _sessMap.end())
		{
			_logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPServer, "addSession() sessId[%s] already exists"), sess->id().c_str());
			return;
		}

		_sessMap[sess->id()] = sess;
		sessSize = _sessMap.size();
	}
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPServer, "addSession() sessId[%s],sessSize[%d]"), sess->id().c_str(), sessSize);
}

void RTSPServer::removeSession(const std::string& sessId)
{
	size_t sessSize = 0;
	{
		ZQ::common::MutexGuard g(_lkSessMap);
		Session::Map::iterator it = _sessMap.find(sessId);
		if (it != _sessMap.end())
			_sessMap.erase(it);
		sessSize = _sessMap.size();
	}
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPServer, "removeSession() sessId[%s],sessSize[%zd]"), sessId.c_str(), sessSize);
}

const RTSPServer::Session::Map& RTSPServer::getSessList() 
{ 
	ZQ::common::MutexGuard g(_lkSessMap);
	return _sessMap; 
}

void RTSPServer::OnTimer()
{
	checkReqStatus();
}

void RTSPServer::checkReqStatus()
{
	RequestList listToCancel;
	{
		ZQ::common::MutexGuard g(_lkReqList);
		for(RequestList::iterator it= _awaitRequests.begin(); it != _awaitRequests.end();)
		{
			if (!(*it) || (*it)->getTimeLeft() > 0)
			{
				it++;
				continue;
			}
			
			//timeout
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPServer, "checkReqStatus() req[%s(%d)] timeout per %d, cancelling from pendings"), RTSPMessage::methodToStr((*it)->method()), (*it)->cSeq(), (int)_config.procTimeout);
			listToCancel.push_back(*it);
			it= _awaitRequests.erase(it);
		}
	}

	for(RequestList::iterator itCancel= listToCancel.begin(); itCancel != listToCancel.end(); itCancel++)
		(*itCancel)->post(RTSPMessage::rcProcessTimeout);
}

void RTSPServer::addReq(RTSPResponse::Ptr resp)
{
	ZQ::common::MutexGuard g(_lkReqList);
	if (std::find(_awaitRequests.begin(),_awaitRequests.end(), resp) != _awaitRequests.end())
		return;
	_awaitRequests.push_back(resp);
}

void RTSPServer::removeReq(RTSPResponse::Ptr resp)
{
	ZQ::common::MutexGuard g(_lkReqList);
	for(RequestList::iterator it= _awaitRequests.begin(); it != _awaitRequests.end();)
	{
		if (*it == resp)
			it= _awaitRequests.erase(it);
		else
			it++;
	}
}

int RTSPServer::getPendingRequest()
{
	ZQ::common::MutexGuard g(_lkReqList);
	return (int)_awaitRequests.size();
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