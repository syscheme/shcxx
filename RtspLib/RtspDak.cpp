// FileName : RtspDak.cpp
// Author   : Zheng Junming
// Date     : 2009-07
// Desc     : 

#include "RtspDak.h"
#include "RtspDialogImpl.h"

namespace ZQRtspCommon
{

RtspDak::RtspDak(ZQ::common::Log &log, int32 nReceiveThreads, int32 nProcessThreads)
:_log(log), _nReceiveThreads(nReceiveThreads), _fac(NULL), _dak(NULL), _handler(NULL), 
_nProcessThreads(nProcessThreads), _processPool(nProcessThreads), _bStart(false)
{
}

RtspDak::~RtspDak()
{
	stop();
	delete _handler;
	_handler = NULL;
}

void RtspDak::registerHandler(ZQRtspCommon::IHandler *handler)
{
	if (NULL == handler || true == _bStart)
	{
		_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "registerHandler() : new Handler is empty, ignore it"));
		return;
	}
	if (_handler != NULL)
	{
		delete _handler;
		_handler = NULL;
		_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "registerHandler() : delete old handler[%p] first"), _handler);
	}
	_handler = handler;
	_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "registerHandler() : register new handler[%p]"), handler);
}

bool RtspDak::initialize()
{
	_fac = new (std::nothrow) RtspDialogFactoryImpl(_log, _processPool, _handler);
	if (!_fac)
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspDak, "initialize() failed to new dialog factory"));
		return false;
	}
	_env.dataFactory = _fac;
	_env.mLogger = &_log;
	_dak = new (std::nothrow) ZQ::DataPostHouse::DataPostDak(_env, _fac);
	if (!_dak)
	{
		_fac = NULL;
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspDak, "initialize() failed to new data post dak"));
		return false;
	}
	_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "initialize() success to initialize RTSP Engine"));
	return true;
}

bool RtspDak::start()
{
	if (true == _bStart)
	{
		return true;
	}
	if (NULL == _handler)
	{
		return false;
	}
	if (!initialize())
	{
		return false;
	}
	if(!_dak->startDak(_nReceiveThreads))
	{
		finalize();
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(RtspDak, "start() failed to start dak with %d concurrent thread"), _nReceiveThreads);
		return false;
	}
	_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "start() start Engine with [%d] receiving threads and [%d] processing threads"), _nReceiveThreads, _nProcessThreads);
	_bStart = true;
	return true;
}

void RtspDak::finalize()
{
	_quitHandle.signal();
	delete _dak;
	_dak = NULL;
	_fac = NULL;
}

void  RtspDak::stop()
{
	if (true == _bStart)
	{
		_bStart = false;
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(RtspDak, "stop() stopping Rtsp Dak..."));
		_dak->stopDak();
		while (_processPool.activeCount() > 0)
		{
			_quitHandle.wait(2000);
		}
		_processPool.stop();
		finalize();
		_log(ZQ::common::Log::L_INFO, CLOGFMT(RtspDak, "stop() rtsp Dak is stoped."));
	}
}

ZQ::DataPostHouse::DataPostHouseEnv& RtspDak::getDataPostHouseEnv()
{
	return _env;
}

ZQ::DataPostHouse::DataPostDak* RtspDak::getDak()
{
	return _dak;
}

void RtspDak::release()
{
	delete this;
}

} // end for ZQRtspCommon
