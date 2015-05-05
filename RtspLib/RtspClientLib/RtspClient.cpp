// FileName : RtspClient.cpp
// Date     : 2009-11
// Desc     : 

#include "RtspClient.h"
#include "SystemUtils.h"

namespace ZQRtspEngine
{

RtspClient::RtspClient(ZQ::common::Log& log, ZQ::DataPostHouse::DataPostDak* dak, ZQ::DataPostHouse::DataPostHouseEnv& env)
: ASocket(env,*dak), _log(log), _dak(dak), _env(env)
{
}

RtspClient::~RtspClient()
{
	_dak = NULL;
}

bool RtspClient::start(const std::string& remoteIp, const std::string& remotePort, ZQ::DataPostHouse::SharedObjectPtr userData)
{
	int family = (remoteIp.find(":") != std::string::npos) ? AF_INET6 : AF_INET;
	mAddrInfoHelper.init(family, SOCK_STREAM, IPPROTO_TCP);
	if(!mAddrInfoHelper.convert(remoteIp , remotePort))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspClient, 
			"start() can't get address information with ip[%s] port[%s] and errorCode[%u]"), 
			remoteIp.c_str(), remotePort.c_str(), SYS::getLastErr(SYS::SOCK));
		return false;
	}
	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	if (!createSocket(adInfo->ai_family, SOCK_STREAM , IPPROTO_TCP))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspClient, 
			"start() failed to create client socket and errorCode [%u]"), SYS::getLastErr(SYS::SOCK));
		return false;
	}
	if (!connect(remoteIp, remotePort))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspClient, 
			"start() failed to connect server with ip[%s] port[%s]"), remoteIp.c_str(), remotePort.c_str());
		return false;
	}
	initializeSockName();
	ZQ::DataPostHouse::IDataCommunicatorExPtr pComm = this;
	this->mType = ZQ::DataPostHouse::COMM_TYPE_TCP;
	this->mCompletionKey.dataCommunicator = pComm;
	this->mCompletionKey.mStatus = true;

	ZQ::DataPostHouse::IDataDialogPtr dialog = mEnv.getDataDialogFactory()->createDataDialog(pComm);	
	if (!dialog)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspClient,"start() failed to create dialog for communicator [%lld]"), getCommunicatorId());
		close();
		return false;
	}
	attchDialog(dialog);
	attachUserData(userData);
	dialog->onCommunicatorSetup(pComm);
	if (!_dak->addnewCommunicator(pComm))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspClient, "start() failed to add communicator [%lld] to DAK , error is[%u]"), getCommunicatorId(), SYS::getLastErr());
		onCommunicatorClosed();
		return false;
	}
	int32 iRet = readAsync( );
	if( iRet == ERROR_CODE_OPERATION_CLOSED )
	{
		_log(ZQ::common::Log::L_ERROR,CLOGFMT(RtspClient, "start() failed to call readAsync for communicator [%lld] and error [%u] , ERROR_CODE_OPERATION_CLOSED"), getCommunicatorId(), SYS::getLastErr(SYS::SOCK));
		onCommunicatorClosed();
		return false;
	}
	else if (iRet < 0 )
	{
		if ( iRet != ERROR_CODE_OPERATION_PENDING )
		{
			_log(ZQ::common::Log::L_ERROR,CLOGFMT(RtspClient, "start() failed to call readAsync for communicator [%lld] and error [%u]"), getCommunicatorId(), SYS::getLastErr(SYS::SOCK));
			onDataAsyncError();
			return false;
		}					
	}
	_strRemoteIP = remoteIp;
	_strRemotePort = remotePort;
	return true;
}

void RtspClient::stop()
{
	onCommunicatorClosed();
}

void RtspClient::release()
{
	/*delete this;*/
}

ZQRtspCommon::IRtspSendMsg* RtspClient::getRequest()
{
	ZQ::DataPostHouse::IDataCommunicatorPtr communicator = NULL;
	communicator = ZQ::DataPostHouse::IDataCommunicatorPtr::dynamicCast(this);
	return new (std::nothrow) ZQRtspCommon::RtspSendMsg(communicator, _log);
}

int RtspClient::sendRawMsg(const std::string& strRawMsg)
{
	int ret = write(strRawMsg.c_str(), strRawMsg.size());
	char hint[0x200];
	sprintf(hint, "CONN["FMT64U"]TID[%08X] send to peer[%s:%s]", 
		getCommunicatorId(), SYS::getCurrentThreadID(), _strRemoteIP.c_str(), _strRemotePort.c_str());
	_log.hexDump(ZQ::common::Log::L_INFO, strRawMsg.c_str(), strRawMsg.size(), hint, true);
	return ret;
}

}
