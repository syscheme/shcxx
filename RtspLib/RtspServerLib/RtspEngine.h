// FileName : RtspEngine.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : RTSP Engine is a RTSP server which is responsible for receiving and send RTSP messages

#ifndef __ZQ_RTSP_ENGINE_SERVER_H__
#define __ZQ_RTSP_ENGINE_SERVER_H__

//ZQCommomStlp
#include "Log.h"

// DataPostHouse
#include "DataCommunicatorUnite.h"
#include "DataCommunicatorSSL.h"
#include "RtspInterface.h"

namespace ZQRtspEngine
{

class RtspEngine
{
public:
	RtspEngine(ZQ::common::Log& log, ZQRtspCommon::IRtspDak* rtspDak);
private:
	~RtspEngine(void);

public:

	void release();

	///@Remark set SSL certificate file and private key file
	void setCertAndKeyFile(const std::string strCertFile, const std::string strKeyFile, const std::string strCertPasswd);

	///@Remark start RTSP TCP Communicators, if success, stopAllCommunicators() must be called
	bool startTCPRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort);

	///@Remark start RTSP SSL Communicators, if success, stopAllCommunicators() must be called
	bool startSSLRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort);

	///@Remark start RTSP UDP Communicators, if success, stopAllCommunicators() must be called
	bool startUDPRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort);

	///@Remark stop RTSP Engine relatived Communicators
	void stopAllCommunicators();

	template<class X>
	void stopCommunicator(ZQ::DataPostHouse::ObjectHandle<X> XPtr, bool& bStart, const std::string strSuccess = "No success message")
	{
		if (XPtr && bStart)
		{
			XPtr->stop();
			bStart = false;
			XPtr = NULL;
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(RtspEngine, "stopCommunicator() : %s"), strSuccess.c_str());
		}
	}

private: // forbidden copy and assign
	RtspEngine(const RtspEngine& rtspEngine); 
	RtspEngine& operator=(const RtspEngine& rtspEngine);

private:
	ZQ::common::Log& _log;
	ZQRtspCommon::IRtspDak* _rtspDak;
	ZQ::DataPostHouse::DataPostHouseEnv& _env;
	ZQ::DataPostHouse::DataPostDak* _dak;
private:
	// RTSP TCP
	ZQ::DataPostHouse::AServerSocketTcpPtr _socketRtspTcpIPv4;
	ZQ::DataPostHouse::AServerSocketTcpPtr _socketRtspTcpIPv6;

	// RTSP UDP
	ZQ::DataPostHouse::AServerSocketUdpPtr _socketRtspUdpIPv4;
	ZQ::DataPostHouse::AServerSocketUdpPtr _socketRtspUdpIPv6;

	//RTSP SSL
	ZQ::DataPostHouse::SSLServerPtr _socketRtspSSLIPv4;
	ZQ::DataPostHouse::SSLServerPtr _socketRtspSSLIPv6;
private:
	bool _bStartRtspTcpIPv4;
	bool _bStartRtspTcpIPv6;

	bool _bStartRtspUdpIPv4;
	bool _bStartRtspUdpIPv6;

	bool _bStartRtspSSLIPv4;
	bool _bStartRtspSSLIPv6;

private: // SSL relative resource
	std::string _strCertificateFile;
	std::string _strPrivatekeyFile;
	std::string _strCertPasswd;
};

} // end for ZQRtspEngine

#endif // end for #ifndef __ZQ_RTSP_ENGINE_H__

