// FileName : RtspDialogImpl.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#ifndef __ZQ_RTSP_COMMON_DIALOG_H__
#define __ZQ_RTSP_COMMON_DIALOG_H__

//ZQCommomStlp
#include "Log.h"
#include "NativeThreadPool.h"

// DataPostHouse
#include "DataCommunicatorUnite.h"

#include "RtspUtils.h"

namespace ZQRtspCommon
{
	
class IHandler;

class RtspDialogImpl : public ZQ::DataPostHouse::IDataDialog
{
	friend class RtspDialogFactoryImpl;
public:
	~RtspDialogImpl();

public:
	virtual void onCommunicatorSetup(ZQ::DataPostHouse::IDataCommunicatorPtr communicator);
	virtual void onCommunicatorDestroyed(ZQ::DataPostHouse::IDataCommunicatorPtr communicator);
	virtual	bool onRead(const int8* buffer ,size_t bufSize);
	virtual	void onWritten(size_t bufSize);
	virtual	void onError();
private:
	RtspDialogImpl(ZQ::common::Log& log, ZQ::common::NativeThreadPool& processPool, IHandler* handler, int32 lMaxPendingRequest);
private:
	bool chopping(const char* data, int bytesNeedtoDecode, int& bytesDecoded, 
		int& bytesSkipped, RtspMessageT& rtspMessage);

private:
	ZQ::DataPostHouse::IDataCommunicatorPtr _communicator;
	std::string _strSavedMsg;
	ZQ::common::Log& _log;
	ZQ::common::NativeThreadPool& _processPool;
	IHandler* _handler;

private:
	std::string _strLocalIP;
	std::string _strLocalPort;
	std::string _strRemoteIP;
	std::string _strRemotePort;

	int         _lMaxPendingRequest;
};

class RtspDialogFactoryImpl : public ZQ::DataPostHouse::IDataDialogFactory
{
public:
	RtspDialogFactoryImpl(ZQ::common::Log& log, ZQ::common::NativeThreadPool& processPool, IHandler* handler, int32 lMaxPendingRequest);
	~RtspDialogFactoryImpl();

public:
	virtual void onClose(CommunicatorS& comms);
	virtual ZQ::DataPostHouse::IDataDialogPtr onCreateDataDialog(ZQ::DataPostHouse::IDataCommunicatorPtr communicator);
	virtual void onReleaseDataDialog(ZQ::DataPostHouse::IDataDialogPtr idalog, 
		ZQ::DataPostHouse::IDataCommunicatorPtr communicator);

private:
	ZQ::common::Log& _log;
	ZQ::common::NativeThreadPool& _processPool;
	IHandler* _handler;
	int32 _lMaxPendingRequest;

};

} // end for ZQRtspCommon

#endif // end for #ifndef __ZQ_RTSP_ENGINE_DIALOG_H__
