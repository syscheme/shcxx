// FileName : RtspDialogImpl.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspDialogImpl.h"
#include "RtspInterfaceImpl.h"
#include "RtspMsgProcessor.h"
#include "TimeUtil.h"
#include "SystemUtils.h"

namespace ZQRtspCommon
{

//---------------RtspDialog implement--------------------------------------------------------
RtspDialogImpl::RtspDialogImpl(ZQ::common::Log& log, ZQ::common::NativeThreadPool& processPool, IHandler* handler, int32 lMaxPendingRequest)
:_communicator(NULL), _strSavedMsg(""), _log(log), _processPool(processPool), _handler(handler), _lMaxPendingRequest(lMaxPendingRequest)
{
}

RtspDialogImpl::~RtspDialogImpl()
{
	if (_communicator)
	{
		_communicator = NULL;
	}
	if (_handler != NULL)
	{
		_handler = NULL;
	}
}

void RtspDialogImpl::onCommunicatorSetup(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
{
	if (!communicator)
	{
		return ;
	}
	_communicator = communicator;
	_communicator->getRemoteAddress(_strRemoteIP, _strRemotePort);
	_communicator->getLocalAddress(_strLocalIP, _strLocalPort);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDialogImpl, 
		"CONN[%08llu]TID[%08X] connection is established between peer[%s:%s] and local[%s:%s]"),  
		_communicator->getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), _strRemotePort.c_str(), _strLocalIP.c_str(), _strLocalPort.c_str());
}

void RtspDialogImpl::onCommunicatorDestroyed(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
{
	if (_handler && communicator)
	{
		_handler->onCommunicatorError(communicator);
	}
	if (_communicator)
	{
		_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDialogImpl, 
			"CONN[%08llu]TID[%08X] connection is closed between peer[%s:%s] and local[%s:%s]"),  
			_communicator->getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), 
			_strRemotePort.c_str(), _strLocalIP.c_str(), _strLocalPort.c_str());
		_communicator = NULL;
	}
}

#define METHOD_SETUP         "SETUP"
#define METHOD_TEARDOWN      "TEARDOWN"
#define METHOD_SETPARAMETER  "SETPARAMETER"
#define METHOD_GETPARAMETER  "GETPARAMETER"
#define METHOD_RESPONSE      "RESPONSE"

std::string  getVerbString(RTSP_VerbCode verb)
{
	switch(verb)
	{
	case RTSP_MTHD_SETUP:
		return "SETUP";
	case RTSP_MTHD_TEARDOWN:
		return "TEARDOWN";
	case RTSP_MTHD_GET_PARAMETER:
		return "GETPARAMETER";
	case RTSP_MTHD_SET_PARAMETER:
		return "SETPARAMETER";
	case RTSP_MTHD_RESPONSE:
		return "RESPONSE";
	default:
		return "UNKNOWN";
	}
}
bool RtspDialogImpl::onRead(const int8* buffer ,size_t bufSize)
{
	// unparse content
	std::string strBuf = _strSavedMsg;
	strBuf.append((const char*)buffer, bufSize);
	_strSavedMsg = "";

	// rtsp message content
	RtspMessageT rtspMessage;

	bool bCompletePkg = false;
	const char* currentPosition = strBuf.c_str();
	int bytesNeedtoDecode = (int)strBuf.size();
	int bytesDecoded = 0;
	int bytesSkipped = 0;
	while (bytesDecoded < bytesNeedtoDecode)
	{
		// chopping message
		rtspMessage.receiveTime = ZQ::common::now();
		bCompletePkg = chopping(currentPosition, bytesNeedtoDecode, bytesDecoded, bytesSkipped, rtspMessage);
		if (!bCompletePkg)
		{
			_strSavedMsg.assign(currentPosition, bytesNeedtoDecode);
			break;
		}
		else
		{

			// hexical dump this request 
			char hint[0x200];
			sprintf(hint, "CONN["FMT64U"]TID[%08X] receive from peer[%s:%s]", 
				_communicator->getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), _strRemotePort.c_str());
			_log.hexDump(ZQ::common::Log::L_INFO, currentPosition + bytesSkipped, bytesDecoded - bytesSkipped, hint, true);

			int curRequestPendingSize	= _processPool.pendingRequestSize();
			int curActiveThreadCount	= _processPool.activeCount();
			int curTotalThreadCount		= _processPool.size();
			int limitedPendingSize		= _lMaxPendingRequest;

			// new one request object
			IRtspReceiveMsg* request = new (std::nothrow) RtspReceiveMsg(rtspMessage, _communicator, _log);
			IRtspSendMsg* response = new (std::nothrow) RtspSendMsg(_communicator, _log);
			RTSP_VerbCode verb = request->getVerb();

			if (verb != RTSP_MTHD_TEARDOWN &&  limitedPendingSize> 0   && curRequestPendingSize > 0  && curRequestPendingSize > limitedPendingSize) 
			{
				//	_log( (curRequestPendingSize > (limitedPendingSize>>1)) ? ZQ::common::Log::L_WARNING : ZQ::common::Log::L_DEBUG, CLOGFMT(RtspDialogImpl,"onRead() server busy with pending requests[%d/%d]"),
				//		curRequestPendingSize, limitedPendingSize );

				std::string strCSeq, onDemandSessionId;
				if(request)
				{
					strCSeq = request->getHeader("CSeq");
					onDemandSessionId = request->getHeader("OnDemandSessionId");
					request->release();
				}
				glog(ZQ::common::Log::L_ERROR, CLOGFMT(RtspDialogImpl,"onRead() method[%s]Seq[%s]Sess[%s], thread[running/Total:%d/%d] pending requests[%d] exceeded limition[%d], rejected"),
					getVerbString(verb).c_str(), strCSeq.c_str(), onDemandSessionId.c_str(), curActiveThreadCount, curTotalThreadCount, curRequestPendingSize, limitedPendingSize);

				if(response)
				{
					response->setHeader("CSeq",               strCSeq.c_str());
					if(!onDemandSessionId.empty())
						response->setHeader("OnDemandSessionId", onDemandSessionId.c_str());
					response->setHeader("Method-Code", getVerbString(verb).c_str());
					response->setStartline("RTSP/1.0 503 Service Unavailable");
					response->post();
					response->release();
				}
			}
			else if (request != NULL && response != NULL)
			{
				// new one process object
				RtspMsgProcessThread* rtspMsgProcessThread = new (std::nothrow) RtspMsgProcessThread(_processPool, _handler, request, response, _log);
				if (rtspMsgProcessThread != NULL)
				{
					rtspMsgProcessThread->start();
				}
			}

			// get one complete message and maybe it has another message.
			bytesNeedtoDecode -= bytesDecoded;
			currentPosition += bytesDecoded;
			bytesDecoded = 0;
		}
	}
	return true;
}

bool RtspDialogImpl::chopping(const char* data, int bytesNeedtoDecode, int& bytesDecoded, 
							int& bytesSkipped, RtspMessageT& rtspMessage)
{
	// clear return value first
	bytesDecoded = 0;
	bytesSkipped = 0;
	RtspUtils::resetMessage(rtspMessage);

	// get start line 
	std::string strLine;
	int lineLen = 0;

	std::string strFirstWord;
	while (bytesDecoded < bytesNeedtoDecode)
	{
		if (!StringHelper::getLine(data, bytesNeedtoDecode - bytesDecoded, lineLen, strLine))
		{
			return false;
		}
		data += lineLen;
		bytesDecoded += lineLen;

		// convert first word into upper case
		if (strLine.empty() || !StringHelper::getFirstWord(strLine, strFirstWord))
		{
			continue;
		}
		if (strFirstWord.find("RTSP") != std::string::npos || RtspUtils::getMethod(strFirstWord) != 0)
		{
			break;
		}
	} // end for while

	if (bytesDecoded >= bytesNeedtoDecode)
	{
		return false;
	}
	bytesSkipped = bytesDecoded - lineLen;
	rtspMessage.strStartLine = strLine;

	// get headers and content
	std::string strKey;
	std::string strValue;
	while (bytesDecoded < bytesNeedtoDecode)
	{
		if (!StringHelper::getLine(data, bytesNeedtoDecode - bytesDecoded, lineLen, strLine))
		{
			return false;
		}
		data += lineLen;
		bytesDecoded += lineLen;

		if (strLine.empty())
		{
			std::map<std::string, std::string>::iterator iter = rtspMessage.rtspHeaders.find("Content-Length");
			if (iter != rtspMessage.rtspHeaders.end())
			{
				int contentLen = atoi(iter->second.c_str());
				if (contentLen > 0)
				{
					if (contentLen > bytesNeedtoDecode - bytesDecoded)
					{
						return false;
					}
					bytesDecoded += contentLen;
					rtspMessage.strContent.assign(data, contentLen);
				}
			}
			return true;
		}
		else
		{
			if (!StringHelper::splitString(strLine, strKey, strValue))
			{
				return false;
			}
			rtspMessage.rtspHeaders.insert(std::make_pair(strKey, strValue));
		}
	}
	return false;
}

void RtspDialogImpl::onWritten(size_t bufSize)
{

}

void RtspDialogImpl::onError()
{
	if (_handler && _communicator)
	{
		_handler->onCommunicatorError(_communicator);
	}
	if (_communicator)
	{
		_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDialogImpl, 
			"CONN["FMT64U"]TID[%08X] connection is closed between peer[%s:%s] and local[%s:%s] as error[%d]"),  
			_communicator->getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), 
			_strRemotePort.c_str(), _strLocalIP.c_str(), _strLocalPort.c_str(), SYS::getLastErr(SYS::SOCK));
		_communicator = NULL;
	}
}

//---------------REDialogFactoryImpl implement----------------------------------------------
RtspDialogFactoryImpl::RtspDialogFactoryImpl(ZQ::common::Log& log, ZQ::common::NativeThreadPool& processPool, IHandler* handler, int32 lMaxPendingRequest)
: _log(log), _processPool(processPool), _handler(handler),_lMaxPendingRequest(lMaxPendingRequest)
{

}

RtspDialogFactoryImpl::~RtspDialogFactoryImpl()
{
	_handler = NULL;
}

void RtspDialogFactoryImpl::onClose(CommunicatorS &comms)
{
	CommunicatorS::iterator it = comms.begin();
	for(; it != comms.end(); it++)
	{
		(*it)->close();
	}
}

ZQ::DataPostHouse::IDataDialogPtr RtspDialogFactoryImpl::onCreateDataDialog(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
{
	return new (std::nothrow) RtspDialogImpl(_log, _processPool, _handler, _lMaxPendingRequest);
}

void RtspDialogFactoryImpl::onReleaseDataDialog(ZQ::DataPostHouse::IDataDialogPtr idalog, 
	ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
{
	communicator->close();
}

} // end for ZQRtspCommon
