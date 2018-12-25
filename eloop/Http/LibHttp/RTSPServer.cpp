#include "RTSPServer.h"
<<<<<<< HEAD
#include "TimeUtil.h"
#include "Guid.h"
#include "SystemUtils.h"

=======

#include "Guid.h"
#include "SystemUtils.h"
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
	virtual ~RTSPPassiveConn(){}
=======
	~RTSPPassiveConn(){}
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	virtual void OnTimer(){}

	virtual void onError( int error,const char* errorDescription );

<<<<<<< HEAD
=======
	virtual void	onDataSent(size_t size);
	virtual void	onDataReceived( size_t size );

>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
RTSPHandler::RTSPHandler(const RTSPMessage::Ptr& req, IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps)
: _app(app), _server(server), _dirProps(dirProps), _req(req)
{
	_app._cOngoings.inc();
}

RTSPHandler::~RTSPHandler()
{
	_app._cOngoings.dec();
=======
RTSPHandler::RTSPHandler(IBaseApplication& app, RTSPServer& server, const RTSPHandler::Properties& dirProps)
: _app(app), _server(server), _dirProps(dirProps)
{
}

RTSPHandler::~RTSPHandler() {}

void RTSPHandler::onDataSent(size_t size)
{
}

void RTSPHandler::onDataReceived( size_t size )
{
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
}

std::string RTSPHandler::mediaSDP(const std::string& mid) { return "";}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::onOptions(RTSPResponse::Ptr& resp)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::onOptions(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	resp->header("Public","OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, PAUSE");
	return RTSPMessage::rcOK;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::onDescribe(RTSPResponse::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3012
	std::string url = _req->url(); 
=======
RTSPMessage::ExtendedErrCode RTSPHandler::onDescribe(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp)
{
	// URL: rtsp://127.0.0.1:9960/3012
	std::string url = req->url(); 
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::onAnnounce(RTSPResponse::Ptr& resp)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::onAnnounce(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	return RTSPMessage::rcOK;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::onGetParameter(RTSPResponse::Ptr& resp)
{
	return RTSPMessage::rcMethodNotAllowed;
}

RTSPMessage::ExtendedErrCode RTSPHandler::onSetParameter(RTSPResponse::Ptr& resp)
{
	return RTSPMessage::rcOK; // dummy yes
}

RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetup(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetup(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// URL: rtsp://127.0.0.1:9960/3201/(rtx/audio/video)
<<<<<<< HEAD
	std::string url = _req->url(); 
=======
	std::string url = req->url(); 
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	// mid and stream name
	size_t urlEndPos = url.rfind("/");
	std::string streamName = url.substr(urlEndPos + 1);
	url = url.erase(urlEndPos);
	size_t midStartPos = url.rfind("/");
	std::string mid = url.substr(midStartPos + 1);

<<<<<<< HEAD
	std::string strTran = _req->header( "Transport" );

	if(strTran.find("TCP") != std::string::npos)
	{
	}
	else if(strTran.find("multicast") != std::string::npos)
	{
	}
	else		//UDP
	{
=======
	std::string strTran = req->header( "Transport" );

	if(strTran.find("TCP") != std::string::npos)
	{

	}
	else if(strTran.find("multicast") != std::string::npos)
	{
		
	}
	else		//UDP
	{
		
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	}

	// TODO: the handler impl here: pSess->setup();

	resp->header("Transport", strTran);
	return RTSPMessage::rcOK;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPlay(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPlay(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

<<<<<<< HEAD
	std::string strTime = _req->header("Range");
=======
	std::string strTime = req->header("Range");
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	// TODO: the handler impl here: pSess->play();

	 resp->header("RTP-Info", svrsess->streamsInfo());
	 return RTSPMessage::rcOK;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPause(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionPause(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;
	// TODO: the handler impl here: pSess->pause();

	return RTSPMessage::rcNotImplement;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionTeardown(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionTeardown(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here: pSess->teardown();
	return RTSPMessage::rcNotImplement;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionAnnounce(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionAnnounce(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionDescribe(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionDescribe(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPServer::Session::Ptr svrsess = RTSPServer::Session::Ptr::dynamicCast(sess);
	if (NULL == svrsess)
		return RTSPMessage::rcInternalError;

	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionGetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionGetParameter(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

<<<<<<< HEAD
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetParameter(RTSPResponse::Ptr& resp, RTSPSession::Ptr& sess)
=======
RTSPMessage::ExtendedErrCode RTSPHandler::procSessionSetParameter(const RTSPMessage::Ptr& req, RTSPServerResponse::Ptr& resp, RTSPSession::Ptr& sess)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	// TODO: the handler impl here
	return RTSPMessage::rcNotImplement;
}

// ---------------------------------------
<<<<<<< HEAD
// class RTSPResponse
// ---------------------------------------
RTSPResponse::RTSPResponse(RTSPServer& server,const RTSPMessage::Ptr& req)
: _server(server), RTSPMessage(req->getConnId(), RTSPMessage::RTSP_MSG_RESPONSE),_req(req),_isResp(false)
{
	header("Method-Code", methodToStr(req->method())); // TODO: to remove
=======
// class RTSPServerResponse
// ---------------------------------------
RTSPServerResponse::RTSPServerResponse(RTSPServer& server,const RTSPMessage::Ptr& req)
: _server(server), RTSPMessage(req->getConnId(), RTSPMessage::RTSP_MSG_RESPONSE),_req(req),_isResp(false)
{
	header(Header_MethodCode, req->method());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	cSeq(req->cSeq());
	_server.addReq(this);
}

<<<<<<< HEAD
int RTSPResponse::getTimeLeft()
{
	if (_server._config.procTimeout <=0)
		return DUMMY_PROCESS_TIMEOUT;

	return (int)(_server._config.procTimeout - _req->elapsed());
}

TCPConnection* RTSPResponse::getConn() 
=======
int RTSPServerResponse::getRemainTime()
{
	int remainTime = int ((int64)(_server._config.procTimeout) + _req->_stampCreated - ZQ::common::now());
	if (remainTime < 0)
		return 0;
	return remainTime;
}

TCPConnection* RTSPServerResponse::getConn() 
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{	
	return _server.findConn(getConnId()); 
}

<<<<<<< HEAD
void RTSPResponse::post(int statusCode, const char* errMsg, bool bAsync) 
=======
void RTSPServerResponse::post(int statusCode, const char* errMsg, bool bAsync) 
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	{
		// to avoid double post response
		ZQ::common::MutexGuard g(_lkIsResp);
		if (_isResp)
			return;
		_isResp = true;
	}

	_server.removeReq(this);

<<<<<<< HEAD
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

=======
	//if (statusCode != 408 && getRemainTime() <= 0)
	//	statusCode = 408;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	code(statusCode);

	if (errMsg != NULL)
		status(errMsg);

	std::string respMsg = toRaw();
<<<<<<< HEAD
	// TODO: _conn._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), _conn.linkstr(),true);
=======
	// TODO: _conn._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), _conn.hint().c_str(),true);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	TCPConnection* conn = _server.findConn(getConnId());
	if (conn == NULL)
	{
<<<<<<< HEAD
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPResponse, "post() drop %s per conn[%s] already closed"), txn.c_str(), getConnId().c_str());
=======
		_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(RTSPServerResponse, "post() conn[%s] already closed"),getConnId().c_str());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
		return;
	}

	int ret = 0;
	if (bAsync)
<<<<<<< HEAD
		ret = conn->enqueueSend(respMsg);
	else
	{
		if (TCPConnection::_enableHexDump > 0)
			_server._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), txn.c_str() ,true);
		ret = conn->enqueueSend((const uint8*) respMsg.c_str(), respMsg.size());
=======
		ret = conn->AsyncSend(respMsg);
	else
	{
		if (TCPConnection::_enableHexDump > 0)
			_server._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), (conn->hint() + " post").c_str(),true);
		ret = conn->write(respMsg.c_str(), respMsg.size());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	}
	 
	if (ret < 0)
	{
<<<<<<< HEAD
		conn->OnConnectionError(ret, ZQ::eloop::Handle::errDesc(ret));
		return;
	}

	_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPResponse, "post() %s ret(%d) took %dms"), txn.c_str(), statusCode, _req->elapsed());
=======
		conn->onError(ret,ZQ::eloop::Handle::errDesc(ret));
		return;
	}

	int64 elapsed = ZQ::common::now() - _req->_stampCreated;
	std::string sessId = header(Header_Session);

	_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPServerResponse, "post() sessId[%s] %s(%d) ret(%d) took %lldms"), sessId.c_str(), _req->method().c_str(), _req->cSeq(), statusCode, elapsed);

	//_server._logger.hexDump(ZQ::common::Log::L_INFO, respMsg.c_str(), (int)respMsg.size(), conn->hint().c_str(),true);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
}

// ---------------------------------------
// class RTSPPassiveConn
// ---------------------------------------
void RTSPPassiveConn::onError( int error,const char* errorDescription )
{
	if(_rtspHandler)
		_rtspHandler->onError(error,errorDescription);
}

<<<<<<< HEAD
=======
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

>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
void RTSPPassiveConn::OnResponse(RTSPMessage::Ptr resp)
{
}

void RTSPPassiveConn::OnRequest(RTSPMessage::Ptr req)
{
<<<<<<< HEAD
	RTSPResponse::Ptr resp = new RTSPResponse(_server, req);

	int respCode = RTSPMessage::rcInternalError;
=======
	RTSPServerResponse::Ptr resp = new RTSPServerResponse(_server, req);

	int respCode =500;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	std::string sessId;

	do {
		sessId =  req->header(Header_Session);
		resp->header(Header_Server, _server._config.serverName);
		resp->header(Header_Session,  sessId);

		int pendings =  _server.getPendingRequest();
		if (pendings >2)
<<<<<<< HEAD
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
=======
			_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequests() enqueuing %s(%d) into pendings[%d /%d]"), req->method().c_str(), req->cSeq(), pendings, (int)_server._config.maxPendings);

		if (_server._config.maxPendings >0 && pendings >= _server._config.maxPendings)
		{
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn, "OnRequests() rejecting %s(%d) per too many pendings [%d /%d]"), req->method().c_str(), req->cSeq(), pendings, _server._config.maxPendings);
			respCode =503;
			break;
		}

		_rtspHandler = _server.createHandler(req->url(), *this);
		if(!_rtspHandler)
		{
			// should make a 404 response
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPPassiveConn, "OnRequests failed to find a suitable handle to process url: %s"), req->url().c_str() );
			respCode =404;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
				respCode = RTSPMessage::rcSessNotFound;
=======
				respCode =454;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
				break;
			}
		}

		if (NULL == pSess)
		{
			// handle if it is a SETUP
<<<<<<< HEAD
			if (RTSPMessage::mtdSETUP == req->method()) 
=======
			if (0 == req->method().compare(Method_SETUP)) 
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
			{ 
				sessId = _server.generateSessionID();
				resp->header(Header_Session,  sessId);
				RTSPServer::Session::Ptr sess = _rtspHandler->_app.newSession(_server, sessId.c_str());
				if (sess == NULL)
				{
					if (_tcpServer)
<<<<<<< HEAD
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), linkstr(), req->cSeq());

					respCode = RTSPMessage::rcInternalError;
=======
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

					respCode =500;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
					break;
				}
				
				pSess = RTSPSession::Ptr::dynamicCast(sess);
				if (NULL == pSess)
				{
					if (_tcpServer)
<<<<<<< HEAD
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), linkstr(), req->cSeq());

					respCode = RTSPMessage::rcInternalError;
=======
						_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() create session failed sessId[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

					respCode =500;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
					break;
				}

				if (_tcpServer)
<<<<<<< HEAD
					_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() building up new session[%s] hint%s SETUP(%d)"), sessId.c_str(), linkstr(), req->cSeq());

				respCode = _rtspHandler->procSessionRequest(RTSPMessage::mtdSETUP, resp, pSess);
=======
					_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() building up new session[%s] hint%s SETUP(%d)"), sessId.c_str(), hint().c_str(), req->cSeq());

				respCode = _rtspHandler->procSessionSetup(req, resp, pSess);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
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
=======
			if (0 == req->method().compare(Method_OPTIONS))	{ respCode = _rtspHandler->onOptions(req, resp);   if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
			if (0 == req->method().compare(Method_ANNOUNCE)) { respCode = _rtspHandler->onAnnounce(req, resp); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
			if (0 == req->method().compare(Method_DESCRIBE)) { respCode = _rtspHandler->onDescribe(req, resp); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }

			respCode =405; break;
		}

		// session-based requests and session has been addressed
		 resp->header(Header_Session,  pSess->id());

		if (0 == req->method().compare(Method_PLAY))     { respCode = _rtspHandler->procSessionPlay(req, resp, pSess);     if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_PAUSE))    { respCode = _rtspHandler->procSessionPause(req, resp, pSess);    if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_ANNOUNCE)) { respCode = _rtspHandler->procSessionAnnounce(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_DESCRIBE)) { respCode = _rtspHandler->procSessionDescribe(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_TEARDOWN)) { respCode = _rtspHandler->procSessionTeardown(req, resp, pSess); _server.removeSession(pSess->id()); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_GetParameter)) { respCode = _rtspHandler->procSessionGetParameter(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }
		if (0 == req->method().compare(Method_SetParameter)) { respCode = _rtspHandler->procSessionSetParameter(req, resp, pSess); if (RTSPMessage::Err_AsyncHandling == respCode) return; break; }

		respCode =405;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	} while(0);

	if (respCode < 100)
<<<<<<< HEAD
		respCode = RTSPMessage::rcInternalError;
=======
		respCode = 500;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

	resp->post(respCode, NULL, false);

// 	resp->code(respCode);
// 	std::string respMsg = resp->toRaw();
// 	write(respMsg.c_str(), respMsg.size());
// 
// 	if (TCPConnection::_enableHexDump > 0)
<<<<<<< HEAD
// 		_logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), (int)respMsg.size(), linkstr(),true);
// 	int64 elapsed = ZQ::common::now() - req->_stampCreated;
// 	
// 	if (_tcpServer)
// 		_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() sessId[%s] %s(%d) ret(%d) took %lldms"), sessId.c_str(), RTSPMessage::methodToStr(req->method()), req->cSeq(), respCode, elapsed);
=======
// 		_logger.hexDump(ZQ::common::Log::L_DEBUG, respMsg.c_str(), (int)respMsg.size(), hint().c_str(),true);
// 	int64 elapsed = ZQ::common::now() - req->_stampCreated;
// 	
// 	if (_tcpServer)
// 		_tcpServer->_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(RTSPPassiveConn, "OnRequest() sessId[%s] %s(%d) ret(%d) took %lldms"), sessId.c_str(), req->method().c_str(), req->cSeq(), respCode, elapsed);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
	conn->enqueueSend((const uint8*)response.c_str(), response.size());
=======
	conn->write(response.c_str(), response.size());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
=======
	try {
		dir.re.assign(uriEx);
	}
	catch( const boost::regex_error& )
	{
		_logger(ZQ::common::Log::L_WARNING, CLOGFMT(HttpServer, "mount() failed to add [%s:%s] as url uriEx"), vsite.c_str(), uriEx.c_str());
		return false;
	}
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

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

<<<<<<< HEAD
RTSPHandler::Ptr RTSPServer::createHandler(const RTSPMessage::Ptr& req, RTSPPassiveConn& conn, const std::string& virtualSite)
=======
RTSPHandler::Ptr RTSPServer::createHandler( const std::string& uri, RTSPPassiveConn& conn, const std::string& virtualSite)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	RTSPHandler::AppPtr app = NULL;

	// cut off the paramesters
<<<<<<< HEAD
	std::string uriWithnoParams = req->url();
=======
	std::string uriWithnoParams = uri;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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
<<<<<<< HEAD
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
=======
		if (boost::regex_match(uriWithnoParams, it->re))
		{
			if (it->app)
				handler = it->app->create(*this, it->props);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

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
<<<<<<< HEAD
			if (!(*it) || (*it)->getTimeLeft() > 0)
=======
			if (!(*it) || (*it)->getRemainTime() > 0)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
			{
				it++;
				continue;
			}
			
			//timeout
<<<<<<< HEAD
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPServer, "checkReqStatus() req[%s(%d)] timeout per %d, cancelling from pendings"), RTSPMessage::methodToStr((*it)->method()), (*it)->cSeq(), (int)_config.procTimeout);
=======
			_logger(ZQ::common::Log::L_WARNING, CLOGFMT(RTSPServer, "checkReqStatus() req[%s(%d)] timeout per %d, cancelling from pendings"), (*it)->method().c_str(), (*it)->cSeq(), (int)_config.procTimeout);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
			listToCancel.push_back(*it);
			it= _awaitRequests.erase(it);
		}
	}

	for(RequestList::iterator itCancel= listToCancel.begin(); itCancel != listToCancel.end(); itCancel++)
<<<<<<< HEAD
		(*itCancel)->post(RTSPMessage::rcProcessTimeout);
}

void RTSPServer::addReq(RTSPResponse::Ptr resp)
=======
		(*itCancel)->post(408);
}

void RTSPServer::addReq(RTSPServerResponse::Ptr resp)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
{
	ZQ::common::MutexGuard g(_lkReqList);
	if (std::find(_awaitRequests.begin(),_awaitRequests.end(), resp) != _awaitRequests.end())
		return;
	_awaitRequests.push_back(resp);
}

<<<<<<< HEAD
void RTSPServer::removeReq(RTSPResponse::Ptr resp)
=======
void RTSPServer::removeReq(RTSPServerResponse::Ptr resp)
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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