#include "TCPServer.h"
#include "TimeUtil.h"

#include <sstream>
#include <algorithm>
#include <list>

namespace ZQ {
namespace eloop {

#define MAX_CSEQ    0x0fffffff
#define MAX_BUFS_PER_SEND  (10)

#define WatchDog_MIN_STEP          (200) // 200msec
#define WatchDog_MAX_STEP          (WatchDog_MIN_STEP *20) // 4sec

//-------------------------------------
//	class InterruptibleLoop
//-------------------------------------
InterruptibleLoop::InterruptibleLoop(int msHeartbeat, int cpuId)
	: Loop(false), Interruptor(*this), _heartbeat(*this),
	_bQuit(false), _cpuId(cpuId), _msHeartbeat(msHeartbeat)
{
	if (_cpuId >=0)
		_cpuId = NativeThread::setCPUAffinity(_cpuId);

	if (_msHeartbeat < LOOP_HEARTBEAT_MIN)
		_msHeartbeat = LOOP_HEARTBEAT_MIN;
	else if (_msHeartbeat > LOOP_HEARTBEAT_MAX)
		_msHeartbeat = LOOP_HEARTBEAT_MAX;
}

bool InterruptibleLoop::inLoop()
{
	return _loopDepth.get() >0;
}

void InterruptibleLoop::watch(InterruptibleLoop::IPing* observee)
{
	if (NULL == observee)
		return;

	if (inLoop())
		MAPSET(PingMap, _pingMap, observee->ident(), observee);
	else
	{
		ZQ::common::MutexGuard gd(_lkLoop);
		MAPSET(PingMap, _pingMap, observee->ident(), observee);
	}
}

void InterruptibleLoop::unwatch(const std::string& ident)
{
	if (inLoop())
		_pingMap.erase(ident);		
	else
	{
		ZQ::common::MutexGuard gd(_lkLoop);
		_pingMap.erase(ident);		
	}
}

void InterruptibleLoop::ping(const std::string& ident)
{
	if (inLoop())
		_wakees.push_back(ident);		
	else
	{
		ZQ::common::MutexGuard gd(_lkLoop);
		_wakees.push_back(ident);		
	}

	Interruptor::wakeup();
}

int InterruptibleLoop::run(void)
{
	OnLoopStart();
	int r = Loop::run(Loop::Default);
	OnLoopQuit(r);
	return r;
}

void InterruptibleLoop::poll(bool isHeartbeat) 
{
	ZQ::common::MutexGuard gd(_lkLoop);
	_loopDepth.inc();

	PingMap toWake;
	if (!isHeartbeat)
	{
		for (size_t i =0; i < _wakees.size(); i++)
		{
			PingMap::iterator it = toWake.find(_wakees[i]);
			if (toWake.end() != it)
				continue;

			it = _pingMap.find(_wakees[i]);
			if (_pingMap.end() == it || NULL == it->second)
				continue;

			toWake.insert(PingMap::value_type(it->first, it->second));
		}

		_wakees.clear();
	}
	else toWake = _pingMap;

	for (PingMap::iterator it = _pingMap.begin(); it != _pingMap.end(); it++)
		it->second->OnPing(isHeartbeat);

	_loopDepth.dec();
}

void InterruptibleLoop::OnDeactived()
{
	ZQ::common::MutexGuard gd(_lkLoop);
	_pingMap.clear();
}

bool InterruptibleLoop::start()
{
	_heartbeat.start(_msHeartbeat, true);
	return NativeThread::start();
}

void InterruptibleLoop::quit()
{
	_heartbeat.stop();
	_bQuit = true;
	Interruptor::wakeup();
}

// ---------------------------------------
// class TCPSvcLoop
// ---------------------------------------
// Single event loop
class TCPSvcLoop : public InterruptibleLoop
{
public:
	TCPSvcLoop(TCPServer& server, int cpuId=-1)
		: InterruptibleLoop(LOOP_HEARTBEAT_MIN, cpuId), _server(server)
	{
	}

	//void addSocket(int sock)
	//{
	//	ZQ::common::MutexGuard gd(_LockSocket);
	//	_ListSocket.push_back(sock);
	//	Interruptor::wakeup();
	//}

protected: // impl of InterruptibleLoop
	void OnLoopStart() { _server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "TCPSvcLoop starts affinite core[%d]"), affinitedId()); }
	void OnLoopQuit(int ret) { _server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "LoopThread quit!")); }

	void poll(bool isHeartbeat)
	{
		// case 1. wakeup to accept new connection
		/*ZQ::common::MutexGuard gd(_LockSocket);
		if (_ListSocket.empty())
			return;

		_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPSvcLoop, "%d pending incomming connections"), _ListSocket.size());
		while( !_ListSocket.empty())
		{
			int sock = _ListSocket.front();
			_ListSocket.pop_front();
			_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPSvcLoop,"OnAsync recv sock = %d"),sock);

			TCPConnection* client = _server.createPassiveConn(*this);
			if (NULL == client)
			{
				_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPSvcLoop, "failed to create passive connection for so[%d]"), sock);
				continue;
			}

			int r = ((TCP*) client)->connected_open(sock);
			if (r != 0)
			{
				_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPSvcLoop, "OnAsync open socke err(%d) %s"), r, Handle::errDesc(r));
				client->disconnect();
				delete client;
				continue;
			}

			if (_server._config.watchDogInterval > 0)
				watch(client);

			client->OnConnected(elpeSuccess);
		}*/

		InterruptibleLoop::poll(isHeartbeat);
	}

private:
	TCPServer&		   _server;
	std::list<int>	   _ListSocket;
	ZQ::common::Mutex  _LockSocket;
};

// ---------------------------------------
// class TCPConnection
// ---------------------------------------
uint TCPConnection::_enableHexDump = 0;

TCPConnection::TCPConnection(InterruptibleLoop& loop, ZQ::common::Log& log, const char* connId, TCPServer* tcpServer)
: _intLoop(loop), TCP(loop), _logger(log), _isConnected(false), _tcpServer(tcpServer), // _watchDog(NULL), _isShutdown(false),_isStop(false),
  _peerPort(0), _localPort(0), _stampBusySend(0)
{
	_lastCSeq.set(1);
	if (connId != NULL)
		_connId = connId;
	else
	{
		char buf[80];
		ZQ::common::Guid guid;
		guid.create();
		guid.toCompactIdstr(buf, sizeof(buf) -2);
		_connId = buf;
	}

	_intLoop.watch(this);
}

TCPConnection::~TCPConnection()
{
	_intLoop.unwatch(this);
}

//int TCPConnection::init(Loop &loop)
//{
//	//_wakeup.init(loop);
//	//if(_pWakeup)
//	//	_pWakeup->init(loop);
//	_pWakeup = new WakeUp(*this);
//
//	return ZQ::eloop::TCP::init(loop);
//}

void TCPConnection::OnConnected(ElpeError status)
{
	TCP::OnConnected(status);
	_isConnected = true;

	char ip[32] = {0};
	_peerPort = 0;
	getpeerIpPort(ip, _peerPort);
	_peerIp = ip;

	ip[0] = 0;
	_localPort = 0;
	getlocaleIpPort(ip, _localPort);
	_localIp = ip;

	std::ostringstream oss, reverseOss;
	if (_tcpServer == NULL)
		oss<<"["<<_localIp<<":"<<_localPort<<"->"<<_peerIp<<":"<<_peerPort<<"]";
	else
		oss<<"["<<_peerIp<<":"<<_peerPort<<"->"<<_localIp<<":"<<_localPort<<"]";

	_linkstr = oss.str();

	read_start();
	if (_tcpServer)
		_tcpServer->addConn(this);

	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "conn[%s] connected: %s"),_connId.c_str(), _linkstr.c_str());
}

bool TCPConnection::disconnect(bool bShutdown)
{
	ZQ::common::MutexGuard gd(_lkSend);
	Buffer::Queue NIL;
	std::swap(_queSend, NIL);

	//_wakeup.close();
	if (bShutdown)
		shutdown();
	else deactive();

	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "%s conn[%s] %s"), bShutdown? "shutting down" :"closing", _connId.c_str(), _linkstr.c_str());
}

void TCPConnection::OnDeactived()
{
	_isConnected = false;
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "OnClose() conn[%s] %s"),_connId.c_str(), _linkstr.c_str());
	_linkstr ="";
	
	if (_tcpServer)
	{
		_tcpServer->delConn(this);
		delete this;
	}
}

void TCPConnection::OnWrote(int status)
{
	ZQ::common::MutexGuard gd(_lkSend);
	_stampBusySend =0;
	_sendNext();
}

int TCPConnection::enqueueSend(const uint8* data, size_t len)
{
	ZQ::common::MutexGuard gd(_lkSend);
	return _enqueueSend(data, len);
}

int TCPConnection::_enqueueSend(const uint8* data, size_t len)
{
	if (NULL ==data || len <=0)
		return 0;

	if (!isActive())
		return 0;

	_queSend.push(new Buffer(data, len));
	_intLoop.ping(ident());

	return (int)len;
}

void TCPConnection::OnShutdown(ElpeError status)
{
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection,"OnShutdown() conn[%s] %s, error(%d): %s"),_connId.c_str(), _linkstr.c_str(), status,errDesc(status));
	deactive();
}

uint TCPConnection::lastCSeq()
{
	int v = _lastCSeq.add(1);
	if (v>0 && v < MAX_CSEQ)
		return (uint) v;

	static ZQ::common::Mutex lock;
	ZQ::common::MutexGuard g(lock);
	v = _lastCSeq.add(1);
	if (v >0 && v < MAX_CSEQ)
		return (uint) v;

	_lastCSeq.set(1);
	v = _lastCSeq.add(1);

	return (uint) v;
}

void TCPConnection::OnPing(bool isHeartbeat)
{
	ZQ::common::MutexGuard gd(_lkSend);
	_sendNext();
}

void TCPConnection::_sendNext(size_t maxlen)
{
	size_t bytes2Sent =0;
	eloop_buf_t eb[MAX_BUFS_PER_SEND];
	Buffer::Queue tmpQueue;
	size_t cEb = 0;

	if (!isActive() || !_isConnected)
	{
		Buffer::Queue NIL;
		std::swap(_queSend, NIL);
		return;
	}

	int64 stampNow = ZQ::common::now();
	if ((stampNow - _stampBusySend) <1000)
		return; // sounds like the previous send is still going, yield

	while ((maxlen>0 && bytes2Sent< maxlen) && !_queSend.empty() && cEb < MAX_BUFS_PER_SEND)
	{
		Buffer::Ptr buf = _queSend.front();
		_queSend.pop();
		if (!buf ||  buf->len()<=0)
			continue;

		tmpQueue.push(buf);
		eb[cEb].base = (char*)buf->data();
		eb[cEb].len  =  buf->len();
		if (eb[cEb].len <=0)
			continue;

		bytes2Sent += eb[cEb++].len;
	}

	if (cEb<=0)
		return;

	_stampBusySend = stampNow;
	int ret = TCPConnection::write(eb, cEb);
	if (ret < 0)
		OnConnectionError(ret, "send failed");

	else _logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "conn[%s] message sent [%d]nbuf(s) total [%d]byte(s)"), _connId.c_str(), cEb, bytes2Sent);
}

// ---------------------------------------
// class ServiceSocket
// ---------------------------------------
// Single event loop
class ServiceSocket : public TCP
{
public:
	ServiceSocket(Loop& loop, TCPServer& server)
		: TCP(loop), _server(server)
	{}

// impl of TCP
	void doAccept(ElpeError status)
	{
		if (status != elpeSuccess)
		{
			_server._logger(ZQ::common::Log::L_ERROR, CLOGFMT(SingleLoopTCPEngine, "doAccept() error code[%d] desc[%s]"),status,errDesc(status));
			return;
		}

		TCPConnection* client = NULL;
		{
			ZQ::common::MutexGuard g(_server._connCountLock);
			if (_server._thrdLoops.size()<=0)
				return;

			_server._idxLoop = ++_server._idxLoop % _server._thrdLoops.size();
			TCPSvcLoop* loop = dynamic_cast<TCPSvcLoop*>(_server._thrdLoops[_server._idxLoop]);
			if (NULL == loop)
				return;

			client = _server.createPassiveConn(*loop);
		}

		if (NULL == client)
			return;

		if (TCP::accept(client) == 0)
			((TCP*)client)->OnConnected(elpeSuccess);
		else client->disconnect();
	}

	TCPServer& _server;
};

// ---------------------------------------
// class TCPServer
// ---------------------------------------
bool TCPServer::start()
{
	if (NULL != _soService)
		return true;

	// start the eloop(s)
	ZQ::common::MutexGuard g(_connCountLock);
	if (_config.mode != MULTIPE_LOOP_MODE)
	{
		TCPSvcLoop* loop = new TCPSvcLoop(*this);
		if (NULL != loop)
		{
			loop->start();
			_thrdLoops.push_back(loop);
			_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPServer, "started single loop"));
		}
	}
	else
	{
		std::stringstream sstr(_config.cpuIds);
		std::string token;   
		while(getline(sstr, token, ','))  
		{  
			TCPSvcLoop* loop = new TCPSvcLoop(*this, atoi(token.c_str()));
			if (NULL == loop)
				continue;

			loop->start();
			_thrdLoops.push_back(loop);
		}

		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPServer, "started %d loops on core: %s"), _thrdLoops.size(), _config.cpuIds);
	}

	if (_thrdLoops.size()<=0)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPServer, "start failed as none loop created"));
		return false;
	}

	// _engine->startAt();
	_soService = new ServiceSocket(*_thrdLoops[0], *this);
	if (NULL == _soService)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPServer, "failed to create service socket"));
		return false;
	}

	if (_soService->bind4(_config.host.c_str(), _config.port) < 0)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPServer, "failed to bind local[%s :%d]"), _config.host.c_str(), _config.port);
		return false;
	}

	if (_soService->listen() < 0)
	{
		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPServer, "failed to listen"));
		return false;
	}

	_logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPServer, "start serving[%s :%d]"), _config.host.c_str(), _config.port);
	return true;
}

bool TCPServer::stop()
{
	// step 1. stop accepting incomming connections
	if (NULL != _soService)
		delete _soService;
	_soService = NULL;

	ZQ::common::MutexGuard gd(_connCountLock);

	// step 2. stop all loops
	for (size_t i =0; i < _thrdLoops.size(); i++)
	{
		TCPSvcLoop* loop = _thrdLoops[i];
		if (NULL == loop)
			continue;

		loop->quit();
		delete loop;
	}

	_thrdLoops.clear();

	// step 2. clear all passive connections
	// for 

	return true;
}

void TCPServer::addConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_connMap[servant->ident()] = servant;
}

void TCPServer::delConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_connMap.erase(servant->ident());
	//if (!_isStart&&_connMap.empty())
	//	_engine->stop();
}

TCPConnection* TCPServer::findConn( const std::string& connId)
{
	ZQ::common::MutexGuard gd(_connCountLock);
	ConnMAP::iterator it = _connMap.find(connId);
	if (it != _connMap.end())
		return it->second;
	return NULL;
}

//void TCPServer::signal()
//{
//	_sysWakeUp.signal();
//}
//
//void TCPServer::onLoopThreadStart(ZQ::eloop::Loop& loop)
//{
//	{
//		ZQ::common::MutexGuard gd(_onStartLock);
//		if (_isOnStart)
//			return;
//		else
//			_isOnStart = true;
//	}
//
//	onStart(loop);
//}

} } //namespace ZQ::eloop
