// FileName : RtspHandlerImpl.cpp
// Author   : Zheng Junming
// Date     : 2009-11
// Desc     : 

#include "RtspHandlerImpl.h"
#include <iostream>

namespace RtspSample
{

RtspHandlerImpl::RtspHandlerImpl(ZQ::common::Log& log)
:_log(log)
{

}

RtspHandlerImpl::~RtspHandlerImpl()
{

}

bool RtspHandlerImpl::HandleMsg(ZQRtspCommon::IRtspReceiveMsg *receiveMsg, ZQRtspCommon::IRtspSendMsg* sendMsg)
{
	std::cout << receiveMsg->getStartline() << std::endl;
	std::cout << "Cseq:" << receiveMsg->getHeader("CSeq") << std::endl;
	//testRequest(receiveMsg, sendMsg);
	//testResponse(receiveMsg, sendMsg);
	return true;
}

void RtspHandlerImpl::testRequest(ZQRtspCommon::IRtspReceiveMsg* request, ZQRtspCommon::IRtspSendMsg* response)
{
	std::string strStart = request->getStartline();
	response->setHeader("getStartline()", strStart.c_str());

	/*ZQRtspCommon::RTSP_VerbCode method = request->getVerb();
	response->setHeader("getVerb()", getMethod(method).c_str());*/

	std::string strURL = request->getUri();
	response->setHeader("getUri()", strURL.c_str());

	std::string strProtocol = request->getProtocol();
	response->setHeader("getProtocol()", strProtocol.c_str());

	std::string strSeq = request->getHeader("CSeq").c_str();
	response->setHeader("getHeader(CSeq)", strSeq.c_str());

	/*std::string strLocalIP, strLocalPort, strRemoteIP, strRemotePort;
	request->getAddressInfo(strLocalIP, strLocalPort, strRemoteIP, strRemotePort);
	std::string strAdd = strLocalIP + ":" + strLocalPort + ";" + strRemoteIP + ":" + strRemotePort;
	response->setHeader("getAddressInfo()", strAdd.c_str());*/

	response->post();
}

void RtspHandlerImpl::testResponse(ZQRtspCommon::IRtspReceiveMsg* request, ZQRtspCommon::IRtspSendMsg* response)
{
	response->setStartline(request->getStartline().c_str());
	response->setHeader("CSeq", request->getHeader("CSeq").c_str());
	/*response->setHeader("Require", request->getHeader("Require").c_str());
	response->setHeader("SessionGroup", request->getHeader("SessionGroup").c_str());
	response->setHeader("EncryptionType", request->getHeader("EncryptionType").c_str());
	response->setHeader("EncryptControl", request->getHeader("EncryptControl").c_str());
	response->setHeader("Transport", request->getHeader("Transport").c_str());
	response->setHeader("OnDemandSessionId", request->getHeader("OnDemandSessionId").c_str());
	response->setHeader("Policy", request->getHeader("Policy").c_str());
	response->setHeader("InbandMarker", request->getHeader("InbandMarker").c_str());*/
	response->setHeader("Hello", "My name is zjm");
	response->setContent(request->getContent().c_str());
	response->post();
}

}