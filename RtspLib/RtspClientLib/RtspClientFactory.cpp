// FileName : RtspClientFactory.cpp
// Date     : 2009-11
// Desc     : 

#include "RtspClientFactory.h"

namespace ZQRtspEngine
{

RtspClientFactory::RtspClientFactory(ZQ::common::Log& log, ZQRtspCommon::IRtspDak* rtspDak)
:_log(log), _rtspDak(rtspDak), _env(rtspDak->getDataPostHouseEnv()), _dak(rtspDak->getDak())
{

}

RtspClientFactory::~RtspClientFactory()
{
	_dak = NULL;
	_rtspDak = NULL;
}

RtspClientPtr RtspClientFactory::createRtspClient(const std::string &remoteIp, const std::string &remotePort, ZQ::DataPostHouse::SharedObjectPtr userData)
{
	RtspClientPtr rtspClient = new (std::nothrow) RtspClient(_log, _dak, _env);
	if (!rtspClient)
	{
		return NULL;
	}
	if (!rtspClient->start(remoteIp, remotePort, userData))
	{
		rtspClient->release();
		return NULL;
	}
	return rtspClient;
}

}
