// FileName : RtspDak.h
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#ifndef __ZQ_RTSP_COMMON_RTSP_DAK_H__
#define __ZQ_RTSP_COMMON_RTSP_DAK_H__

#include "Log.h"
#include "NativeThreadPool.h"
#include "RtspInterface.h"
#include "SystemUtils.h"

namespace ZQRtspCommon
{

class IHandler;

class RtspDak : public IRtspDak
{
public:
	RtspDak(ZQ::common::Log& log, int32 nReceiveThreads, int32 nProcessThreads = 10, int32 lMaxPendingRequest = 100);
	~RtspDak();
public:
	///@Remark start dak to process requests, if success, stop() must be called
	bool start();

	///@Remark stop dak
	void stop();

	///@remark : RtspDak is responsible for freeing IHandler pointers, 
	///          this method must be called before you call start()
	void registerHandler(IHandler* handler);

	ZQ::DataPostHouse::DataPostHouseEnv& getDataPostHouseEnv();

	ZQ::DataPostHouse::DataPostDak* getDak();

	/// release this request object
	virtual void release();

private:
	bool initialize();

	void finalize();

private:
	ZQ::common::Log& _log;
	int32 _nReceiveThreads;

private:
	ZQ::DataPostHouse::DataPostHouseEnv _env;
	ZQ::DataPostHouse::IDataDialogFactoryPtr _fac;
	ZQ::DataPostHouse::DataPostDak* _dak;
	IHandler* _handler;
private:
	int32 _nProcessThreads;
	ZQ::common::NativeThreadPool _processPool;
	SYS::SingleObject _quitHandle;
	bool _bStart;
	int32 _lMaxPendingRequest;
};

} // end for ZQRtspCommon

#endif // end for __ZQ_RTSP_COMMON_RTSP_DAK_H__
