#include "SocketServer.h"
#include <NativeThreadPool.h>
#include "DoSocket.h"
#include <sys/resource.h>

namespace LibAsync {

LoopCenter::LoopCenter():mIdxLoop(0){

}

LoopCenter::~LoopCenter() {
	stopCenter();
}

bool LoopCenter::startCenter( size_t count) {
	if( count <= 0)
		count =	1;
	ZQ::common::MutexGuard gd(mLocker);
	for( size_t i = 0; i < count; i ++ ) {
		EventLoop* l = new EventLoop();
		if(!l->start()){
			delete l;
			return false;
		}
		l->ignoreSigpipe();
		mLoops.push_back(l);
	}
	return true;
}

void LoopCenter::stopCenter() {
	ZQ::common::MutexGuard gd(mLocker);
	if( mLoops.size() == 0 )
		return;
	for( size_t i = 0 ; i < mLoops.size(); i ++ ) {
		mLoops[i]->stop();
		delete mLoops[i];
	}
	mLoops.clear();
}

///从Center里面获取一个EventLoop，当前的实现版本是roundrobin
EventLoop& LoopCenter::getLoop(){
	size_t idx = 0;
	{
		ZQ::common::MutexGuard gd(mLocker);
		assert(mLoops.size() > 0);
		idx = mIdxLoop++;
		if(mIdxLoop >= mLoops.size())
			mIdxLoop = 0;
	}
	return *mLoops[idx];
}

LoopCenter SocketServer::_lopCenter;

SocketServer::SocketServer(EventLoop& loop, const std::string ip, const unsigned int port, ZQ::common::Log& log)
: _serverIP(ip), _port(port), _log(log), _error(false), _connectionNum(0), Socket(loop)
{
	_pThreadPool = new ZQ::common::NativeThreadPool(20);
	struct rlimit  rl;
	rl.rlim_cur = 64 * 1024;
	rl.rlim_max = 640 * 1024;
	if(setrlimit( RLIMIT_NOFILE, &rl) == 0)
	{
		_log(ZQ::common::Log::L_NOTICE, CLOGFMT(SocketServer, "set rlimit successful."));
	}		
	else
	{
		int err = errno;
		_log(ZQ::common::Log::L_WARNING, CLOGFMT(SocketServer, "set rlimit failed with error[%d]."), err);
	}
}

SocketServer::~SocketServer()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SocketServer, "~SocketServer() entry."));	
	if (_pThreadPool != NULL)
	{
		delete _pThreadPool;
		_pThreadPool = NULL;
	}
}

bool SocketServer::startServer(int backlog)
{
	
	if( !bind(_serverIP, _port))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SocketServer, "startServer() bind failed."));
		return false;
	}
	if ( !accept(10000) )
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SocketServer, "startServer() accept failed."));
		return false;
	}
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SocketServer, "startServer() start successful."));
	return true;
}

Socket::Ptr SocketServer::onSocketAccepted( SOCKET sock )
{
	_connectionNum ++;
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SocketServer, "onSocketAccepted() enter with [%d] connections."), _connectionNum);
	DoSocketPtr doPtr = new DoSocket(_lopCenter.getLoop(), sock, _log, *_pThreadPool);
	//doPtr->onSocketConnected();
	doPtr->initialServerSocket();
	return doPtr;
}

void SocketServer::onSocketError(int err)
{
	_error = true;
	_log(ZQ::common::Log::L_ERROR, CLOGFMT(SocketServer, "onSocketError() enter with error[%d]"), err);
}

}