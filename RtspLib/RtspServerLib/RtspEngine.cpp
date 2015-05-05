// FileName : RtspEngine.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspEngine.h"

namespace ZQRtspEngine
{

RtspEngine::RtspEngine(ZQ::common::Log& log, ZQRtspCommon::IRtspDak* rtspDak)
: _log(log), _rtspDak(rtspDak), _env(rtspDak->getDataPostHouseEnv()), _dak(rtspDak->getDak()),
_socketRtspTcpIPv4(NULL), _socketRtspTcpIPv6(NULL), _socketRtspUdpIPv4(NULL), 
_socketRtspUdpIPv6(NULL),_socketRtspSSLIPv4(NULL), _socketRtspSSLIPv6(NULL), 
_bStartRtspTcpIPv4(false), _bStartRtspTcpIPv6(false),_bStartRtspUdpIPv4(false), 
_bStartRtspUdpIPv6(false), _bStartRtspSSLIPv4(false), _bStartRtspSSLIPv6(false)
{
}

RtspEngine::~RtspEngine(void)
{
}

void RtspEngine::release()
{
	delete this;
}

void RtspEngine::stopAllCommunicators()
{
	// RTSP TCP IPv4
	stopCommunicator(_socketRtspTcpIPv4, _bStartRtspTcpIPv4, "Stop RTSP TCP IPv4 communicator");

	// RTSP TCP IPv6
	stopCommunicator(_socketRtspTcpIPv6, _bStartRtspTcpIPv6, "Stop RTSP TCP IPv6 communicator");

	// RTSP UDP IPv4
	stopCommunicator(_socketRtspUdpIPv4, _bStartRtspUdpIPv4, "Stop RTSP UDP IPv4 communicator");

	// RTSP UDP IPv6
	stopCommunicator(_socketRtspUdpIPv6, _bStartRtspUdpIPv6, "Stop RTSP UDP IPv6 communicator");

	// RTSP SSL IPv4
	stopCommunicator(_socketRtspSSLIPv4, _bStartRtspSSLIPv4, "Stop RTSP SSL IPv4 communicator");

	// RTSP SSL IPv6
	stopCommunicator(_socketRtspSSLIPv6, _bStartRtspSSLIPv6, "Stop RTSP SSL IPv6 communicator");

}

bool RtspEngine::startTCPRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort)
{
	// TCP IPv4
	_socketRtspTcpIPv4 = new (std::nothrow) ZQ::DataPostHouse::AServerSocketTcp(*_dak, _env);
	if (!_socketRtspTcpIPv4)
	{
		_log(ZQ::common::Log::L_ERROR, "startTCPRtsp() : Fail to create RTSP TCP IPv4 communicator");
		return false;
	}
	_bStartRtspTcpIPv4 = _socketRtspTcpIPv4->startServer(strLocalIPv4, strLocalPort);
	if (_bStartRtspTcpIPv4)
	{
		_log(ZQ::common::Log::L_DEBUG, "startTCPRtsp() : RTSP TCP IPv4 communicator is listening");
	}

	// TCP IPv6
	_socketRtspTcpIPv6 = new (std::nothrow) ZQ::DataPostHouse::AServerSocketTcp(*_dak, _env);
	if (!_socketRtspTcpIPv6)
	{
		_log(ZQ::common::Log::L_ERROR, "startTCPRtsp() : Fail to create RTSP TCP IPv6 communicator");
		return false;
	}
	_bStartRtspTcpIPv6 = _socketRtspTcpIPv6->startServer(strLocalIPv6, strLocalPort);
	if (_bStartRtspTcpIPv6)
	{
		_log(ZQ::common::Log::L_DEBUG, "startTCPRtsp() : RTSP TCP IPv6 communicator is listening");
	}
	return (_bStartRtspTcpIPv4 || _bStartRtspTcpIPv6);
}

bool RtspEngine::startUDPRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort)
{
	_socketRtspUdpIPv4 = new (std::nothrow) ZQ::DataPostHouse::AServerSocketUdp(*_dak, _env);
	if (!_socketRtspUdpIPv4)
	{
		_log(ZQ::common::Log::L_ERROR, "startUDPRtsp() : Fail to create RTSP UDP IPv4 communicator");
		return false;
	}
	_bStartRtspUdpIPv4 = _socketRtspUdpIPv4->startServer(strLocalIPv4, strLocalPort);
	if (_bStartRtspUdpIPv4)
	{
		_log(ZQ::common::Log::L_DEBUG, "startUDPRtsp() : RTSP UDP IPv4 communicator is listening");
	}

	_socketRtspUdpIPv6 = new (std::nothrow) ZQ::DataPostHouse::AServerSocketUdp(*_dak, _env);
	if (!_socketRtspUdpIPv6)
	{
		_log(ZQ::common::Log::L_ERROR, "startUDPRtsp() : Fail to create RTSP UDP IPv6 communicator");
		return false;
	}
	_bStartRtspUdpIPv6 = _socketRtspUdpIPv6->startServer(strLocalIPv4, strLocalPort);
	if (_bStartRtspUdpIPv6)
	{
		_log(ZQ::common::Log::L_DEBUG, "startUDPRtsp() : RTSP UDP IPv6 communicator is listening");
	}
	return (_bStartRtspUdpIPv4 || _bStartRtspUdpIPv6);
}

bool RtspEngine::startSSLRtsp(const std::string strLocalIPv4, const std::string strLocalIPv6, const std::string strLocalPort)
{
	// TCP IPv4
	_socketRtspSSLIPv4 = new (std::nothrow) ZQ::DataPostHouse::SSLServer(*_dak, _env);
	if (!_socketRtspSSLIPv4)
	{
		_log(ZQ::common::Log::L_ERROR, "startSSLRtsp() : Fail to create RTSP SSL IPv4 communicator");
		return false;
	}
	_socketRtspSSLIPv4->setCertAndKeyFile(_strCertificateFile, _strPrivatekeyFile, _strCertPasswd);
	_bStartRtspSSLIPv4 = _socketRtspSSLIPv4->startServer(strLocalIPv4, strLocalPort);
	if (_bStartRtspSSLIPv4)
	{
		_log(ZQ::common::Log::L_DEBUG, "startSSLRtsp() : RTSP SSL IPv4 communicator is listening");
	}

	// TCP IPv6

	_socketRtspSSLIPv6 = new (std::nothrow) ZQ::DataPostHouse::SSLServer(*_dak, _env);
	if (!_socketRtspSSLIPv6)
	{
		_log(ZQ::common::Log::L_ERROR, "startSSLRtsp() : Fail to create RTSP SSL IPv6 communicator");
		return false;
	}
	_socketRtspSSLIPv6->setCertAndKeyFile(_strCertificateFile, _strPrivatekeyFile, _strCertPasswd);
	_bStartRtspSSLIPv6 = _socketRtspSSLIPv6->startServer(strLocalIPv6, strLocalPort);
	if (_bStartRtspSSLIPv6)
	{
		_log(ZQ::common::Log::L_DEBUG, "startSSLRtsp() : RTSP SSL IPv6 communicator is listening");
	}
	return (_bStartRtspSSLIPv4 || _bStartRtspSSLIPv6);
}

void RtspEngine::setCertAndKeyFile(const std::string strCertFile, const std::string strKeyFile, const std::string strCertPasswd)
{
	_strCertificateFile = strCertFile;
	_strPrivatekeyFile = strKeyFile;
	_strCertPasswd = strCertPasswd;
}

} // end for ZQRtspEngine


