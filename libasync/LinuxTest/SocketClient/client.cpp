#include "client.h"
#include <assert.h>
#include <string.h>
#include <TimeUtil.h>
namespace LibAsync{

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
		if( i == 0)
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

LoopCenter SockClient::_lopCenter;


SockClient::SockClient(ZQ::common::Log& log, const std::string& ip, unsigned short port, EventLoop& loop, const std::string& name, writeThread::Ptr ptr)
: _log(log), _ip(ip), _port(port), _recvOK(false), _recvBytes(0), _fileName(name), _writePtr(ptr), _startRecv(0), _endRecv(0), Socket(loop)
{
	AsyncBuffer  sendBuf;
	sendBuf.len = 1024;
	sendBuf.base = (char*) malloc(sendBuf.len * sizeof(char));

	snprintf(sendBuf.base, sendBuf.len, "%s\0", _fileName.c_str());
	sendBuf.len = strlen(sendBuf.base);
	_sendBufs.push_back(sendBuf);

	AsyncBuffer  recvBuf1, recvBuf2;
	recvBuf1.len = 1024 * 250;
	recvBuf1.base = (char*)malloc(recvBuf1.len * sizeof(char));
	_recvBufs.push_back(recvBuf1);
	recvBuf2.len = 1024 * 250;
	recvBuf2.base = (char*)malloc(recvBuf2.len * sizeof(char));
	_recvBufs.push_back(recvBuf2);
}

SockClient::~SockClient()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "~SockClient() socket[%p] deleteed."), this);
	for (AsyncBufferS::iterator iter = _sendBufs.begin(); iter != _sendBufs.end(); iter++)
	{
		free(iter->base);
		iter->len = 0;
	}
	
	for (AsyncBufferS::iterator iter = _recvBufs.begin(); iter != _recvBufs.end(); iter++)
	{
		free(iter->base);
		iter->len = 0;
	}
	_writePtr = NULL;
}

void SockClient::doConnect()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "connect() file[%s] socket[%p]  entry."), _fileName.c_str(), this);
	if ( !connect(_ip, _port) )
	{
		_recvOK = true;
		_endRecv = ZQ::common::TimeUtil::now();
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "connect() connect failed."));
	}
}

void SockClient::onSocketConnected()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketConnected() file[%s]."), _fileName.c_str());
	if(!sendBuf()){
		//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketConnected() file[%s] sendBuf successful."), _fileName.c_str());
	//else
		_recvOK = true;
		_endRecv = ZQ::common::TimeUtil::now();
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SockClient, "onSocketConnected() file[%s] sendBuf failed."), _fileName.c_str());	
	}
}

bool SockClient::sendBuf()
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "sendBuf() file[%s]."), _fileName.c_str());
	if( !_sendBufs.empty())
	{
 		if( !send(_sendBufs) )
 			return false;	
	}
 	else 
 	{
 		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "sendBuf() the data is empty."));
		return false;
 	}
 	return true;
}

void SockClient::onSocketSent(size_t size)
{
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketSent() send request file[%s] successful client[%p]."), _fileName.c_str(), this);
	_sendBufs.clear();
	if ( !recvBuf() ){
	//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketSent() call recv successful file[%s] client[%p]."), _fileName.c_str(), this);
	//else
		_recvOK = true;
		_endRecv = ZQ::common::TimeUtil::now();
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SockClient, "onSocketSent() call rcv failed file[%s]  client[%p]."), _fileName.c_str(), this);
	}
}

bool SockClient::recvBuf()
{
	for (AsyncBufferS::iterator iter = _recvBufs.begin(); iter != _recvBufs.end(); iter++)
		memset(iter->base, '\0', iter->len);
//	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "recvBuf() file[%s] client[%p]."), _fileName.c_str(), this);
	if( !_recvBufs.empty() )
		 return recv(_recvBufs);
	return false;
}

void SockClient::onSocketRecved(size_t size)
{
	if(_startRecv == 0)
		_startRecv = ZQ::common::TimeUtil::now();
	_recvBytes += size;
	int usingTime = (int)(ZQ::common::TimeUtil::now() - _startRecv);
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketRecved() client[%p] recv totalSIze[%d] using[%d]ms."), this, _recvBytes, usingTime);
	/*std::string dataBuf = "";
	int readSize = size;
	AsyncBufferS::const_iterator iterBuf = _recvBufs.begin();
	for (; iterBuf != _recvBufs.end() ; iterBuf ++)
	{
		if (readSize <=  0)
			break;
		int bufLen = 0;
		if (readSize > iterBuf->len)
		{
			bufLen = iterBuf->len;
		}
		else
		{
			bufLen = readSize;
		}
		readSize -= bufLen;
		if (bufLen > 0)
			dataBuf.append(iterBuf->base, bufLen);
	}*/
	//_writePtr->addData(_fileName, dataBuf);
	if ( !recvBuf() ){
		//_log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "onSocketRecved() call recv successful file[%s] client[%p]."), _fileName.c_str(), this);
	//else
		_recvOK = true;
		_endRecv = ZQ::common::TimeUtil::now();
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(SockClient, "onSocketRecved() call rcv failed file[%s] client[%p]."), _fileName.c_str(), this);
	}
}


void SockClient::onSocketError(int err)
{
	_recvOK = true;
	_endRecv = ZQ::common::TimeUtil::now();
	_log(ZQ::common::Log::L_ERROR, CLOGFMT(SockClient, "onSocketError() client[%p] with errro[%d]."), this, err);
	Socket::onSocketError(err);
}

void SockClient::addRecvBuf( AsyncBuffer buf )
{
	_recvBufs.push_back(buf);
}

void SockClient::addSendBuf( AsyncBuffer buf )
{
	_sendBufs.push_back(buf);
}

bool SockClient::status()
{
	if(_recvOK){
			//int usingTime = (int)(ZQ::common::TimeUtil::now() - _startRecv);
			int usingTime = (int)(_endRecv - _startRecv);
			float bps = 0;
			if(usingTime > 0)
				bps = _recvBytes / (usingTime * 1000.0);
		    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(SockClient, "status() client[%p] recv totalSIze[%d] using[%d]ms [%.3f]Mbps."), this, _recvBytes, usingTime, bps);
	
	}

	return _recvOK;


}

}
