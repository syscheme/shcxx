// FileName : RtspClient.h
// Date     : 2009-11
// Desc     : 

#ifndef __ZQ_RTSP_ENGINE_CLIENT_H__
#define __ZQ_RTSP_ENGINE_CLIENT_H__

#include "Log.h"
#include "RtspInterfaceImpl.h"
#include "DataCommunicatorUnite.h"

namespace ZQRtspEngine
{

class RtspClient : public ZQ::DataPostHouse::ASocket
{
public:
	RtspClient(ZQ::common::Log& log, ZQ::DataPostHouse::DataPostDak* dak, 
		ZQ::DataPostHouse::DataPostHouseEnv& env);
	~RtspClient();
public:
	bool start(const std::string& remoteIp, const std::string& remotePort, ZQ::DataPostHouse::SharedObjectPtr userData = NULL);
	void stop();
	void release();
	
	// must call IRtspClientRequest->release() to delete it
	ZQRtspCommon::IRtspSendMsg* getRequest();
	int sendRawMsg(const std::string& strRawMsg);
private:
	ZQ::common::Log& _log;
	ZQ::DataPostHouse::DataPostDak* _dak;
	ZQ::DataPostHouse::DataPostHouseEnv& _env;
private:
	std::string _strRemoteIP;
	std::string _strRemotePort;
};

typedef ZQ::DataPostHouse::ObjectHandle<RtspClient> RtspClientPtr;

}

#endif // end for __ZQ_RTSP_CLIENT_H__

