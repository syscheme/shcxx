// FileName : RtspClientFactory.h
// Date     : 2009-11
// Desc     : 

#ifndef __ZQ_RTSP_ENGINE_CLIENT_FACTORY_H__
#define __ZQ_RTSP_ENGINE_CLIENT_FACTORY_H__

#include "RtspInterface.h"
#include "RtspClient.h"

namespace ZQRtspEngine
{

class RtspClientFactory
{
public:
	RtspClientFactory(ZQ::common::Log& log, ZQRtspCommon::IRtspDak* rtspDak);
	~RtspClientFactory();
public:
	RtspClientPtr createRtspClient(const std::string& remoteIp, const std::string& remotePort, ZQ::DataPostHouse::SharedObjectPtr userData = NULL);
private:
	ZQ::common::Log& _log;
	ZQRtspCommon::IRtspDak* _rtspDak;
	ZQ::DataPostHouse::DataPostHouseEnv& _env;
	ZQ::DataPostHouse::DataPostDak* _dak;

};

}

#endif
