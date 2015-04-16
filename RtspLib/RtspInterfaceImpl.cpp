// FileName : RtspInterfaceImpl.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspInterfaceImpl.h"
#include "strHelper.h"
#include "TimeUtil.h"
#include "SystemUtils.h"

namespace ZQRtspCommon
{

//---------------------------------- RtspSendMsg Implementation---------------------------

RtspSendMsg::RtspSendMsg(ZQ::DataPostHouse::IDataCommunicatorPtr communicator, ZQ::common::Log& log)
:_communicator(communicator), _log(log)
{
	if (_communicator)
	{
		_communicator->getRemoteAddress(_strRemoteIP, _strRemotePort);
	}
}

RtspSendMsg::~RtspSendMsg()
{
	_communicator = NULL;
}

void RtspSendMsg::release()
{
	delete this;
}

void RtspSendMsg::reset()
{
	RtspUtils::resetMessage(_rtspMsg);
}

void RtspSendMsg::setStartline(const char *buf)
{
	if (buf != NULL)
	{
		_rtspMsg.strStartLine = buf;
	}
}

void RtspSendMsg::setContent(const char *content)
{
	if (content != NULL)
	{
		_rtspMsg.strContent = content;
	}
}

void RtspSendMsg::setHeader(const char *key, const char *value)
{
	if (key != NULL && value != NULL)
	{
		std::string strKey(key);
		std::string strValue(value);
		std::map<std::string, std::string>::iterator iter = _rtspMsg.rtspHeaders.find(strKey);
		if (iter != _rtspMsg.rtspHeaders.end())
		{
			iter->second = strValue;
		}
		else
		{
			_rtspMsg.rtspHeaders.insert(std::make_pair(strKey, strValue));
		}
	}
}

int32 RtspSendMsg::post(const std::string& strRawMsg)
{
	if (!_communicator)
	{
		return -1;
	}
	std::string res;
	if (strRawMsg.empty())
	{
		RtspUtils::composeMessage(_rtspMsg, res);
	}
	else
	{
		res = strRawMsg;
	}
	int32 ret = _communicator->write(res.c_str(), res.size());
	char hint[0x200];
	sprintf(hint, "CONN["FMT64U"]TID[%08X] send to peer[%s:%s]", _communicator->getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), _strRemotePort.c_str());
	_log.hexDump(ZQ::common::Log::L_INFO, res.c_str(), res.size(), hint, true);
	return ret;
}

//---------------------------------- RtspReceiveMsg Implementation---------------------------

RtspReceiveMsg::RtspReceiveMsg(ZQRtspCommon::RtspMessageT &rtspMsg, ZQ::DataPostHouse::IDataCommunicatorPtr communicator, ZQ::common::Log &log)
:_communicator(communicator), _log(log), _strUri(""), _strProtocol(""), _method(RTSP_MTHD_NULL), _strStatusString("")
{
	// assign message
	_rtspMsg.strStartLine = rtspMsg.strStartLine;
	std::copy(rtspMsg.rtspHeaders.begin(), rtspMsg.rtspHeaders.end(), std::inserter(_rtspMsg.rtspHeaders, _rtspMsg.rtspHeaders.begin()));
	_rtspMsg.strContent = rtspMsg.strContent;

	std::vector<std::string> strVec;
	ZQ::common::stringHelper::SplitString(_rtspMsg.strStartLine, strVec, " \t\f\v");

	if (strVec.size() > 3)
	{
		_method = RTSP_MTHD_RESPONSE;
		_strProtocol = strVec[0];
		_statusCode = atoi(strVec[1].c_str());
		for (size_t i = 2; i < strVec.size(); i++)
		{
			_strStatusString += strVec[i];
			if (i != strVec.size())
			{
				_strStatusString += " ";
			}
		}
	}

	if (strVec.size() == 3)
	{
		_method = (RTSP_VerbCode)(RtspUtils::getMethod(strVec[0]));
		if (_method != 0)
		{
			_strUri = strVec[1];
			_strProtocol = strVec[2];
		}
		else
		{
			_method = RTSP_MTHD_RESPONSE;
			_strProtocol = strVec[0];
			_statusCode = atoi(strVec[1].c_str());
			_strStatusString = strVec[2];
		}
	}
}

RtspReceiveMsg::~RtspReceiveMsg()
{
	_communicator = NULL;
}

void RtspReceiveMsg::release()
{
	delete this;
}

RTSP_VerbCode RtspReceiveMsg::getVerb() const
{
	return _method;
}

const std::string RtspReceiveMsg::getUri() const
{
	return _strUri;
}

const std::string RtspReceiveMsg::getProtocol() const
{
	return _strProtocol;
}

const std::string RtspReceiveMsg::getStartline() const
{
	return _rtspMsg.strStartLine;
}

const std::string RtspReceiveMsg::getHeader(const char* key) const
{
	std::string strResult("");
	std::string strKey(key);
	std::map<std::string, std::string>::const_iterator iter = _rtspMsg.rtspHeaders.find(strKey);
	if (iter != _rtspMsg.rtspHeaders.end())
	{
		strResult = iter->second;
	}
	return strResult;

}

ZQ::DataPostHouse::IDataCommunicatorPtr RtspReceiveMsg::getCommunicator() const
{
	return _communicator;
}

const std::string RtspReceiveMsg::getContent() const
{
	return _rtspMsg.strContent;
}

int RtspReceiveMsg::getStatus() const
{
	return _statusCode;
}

const std::string RtspReceiveMsg::getStatusString() const
{
	return _strStatusString;
}

int64 RtspReceiveMsg::getReceiveTime() const
{
	return _rtspMsg.receiveTime;
}

std::string RtspReceiveMsg::getRawMsg() const
{
	std::string strRawMsg;
	RtspUtils::composeMessage(_rtspMsg, strRawMsg);
	return strRawMsg;
}

} // end for ZQRtspCommon
