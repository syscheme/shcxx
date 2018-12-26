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
//	class WatchedLoop
//-------------------------------------
WatchedLoop::WatchedLoop(int msStep, int cpuId=-1)
	: Loop(false), Interruptor(*this), _watchDog(*this),
	_bQuit(false), _cpuId(cpuId), _msStep(msStep)
{
	if (_cpuId >=0)
		NativeThread::setCPUAffinity(_cpuId);

	if (_msStep < WatchDog_MIN_STEP)
		_msStep = WatchDog_MIN_STEP;
	else if (_msStep > WatchDog_MAX_STEP)
		_msStep = WatchDog_MAX_STEP;
}

void WatchedLoop::watch(IObservee* observee, int timeout)
{
	if (NULL == observee)
		return;

	int64 stampNow = ZQ::common::now();

	WatchNode wn;
	wn.obj = observee;
	wn.expiration = stampNow + ((timeout>0) ? timeout: (WatchDog_MIN_STEP/2));

	if (inLoop())
		MAPSET(ObserveeMap, _observeeMap, observee->ident(), wn);		
	else
	{
		ZQ::common::MutexGuard gd(_observeeLock);
		MAPSET(ObserveeMap, _observeeMap, observee->ident(), wn);		
	}
}

void WatchedLoop::unwatch(const std::string& ident)
{
	if (inLoop())
		_observeeMap.erase(ident);
	else
	{
		ZQ::common::MutexGuard gd(_observeeLock);
		_observeeMap.erase(ident);		
	}
}

int WatchedLoop::run(void)
{
	OnLoopStart();
	int r = Loop::run(Loop::Default);
	OnLoopQuit(r);
	return r;
}

void WatchedLoop::poll() 
{
	ZQ::common::MutexGuard gd(_observeeLock);
	int64 stampNow = ZQ::common::now();
	for (ObserveeMap::iterator it = _observeeMap; it != _observeeMap.end(); it++)
	{
		if (it->second.expiration >stampNow)
			continue;
	}

	for (size_t i=0; i<_observeeList.size();i++)
		_observeeList[i]->OnTimer();
}

void WatchDog::OnDeactived()
{
	ZQ::common::MutexGuard gd(_observeeLock);
	for (size_t i=0; i<_observeeList.size();i++)
		_observeeList[i]->OnUnwatch();
	_observeeList.clear();
}

void WatchDog::watch(IObservee* observee)
{
	ZQ::common::MutexGuard gd(_observeeLock);
	if (std::find(_observeeList.begin(),_observeeList.end(), observee) != _observeeList.end())
		return;

	_observeeList.push_back(observee);
}

void WatchDog::unwatch(IObservee* observee)
{
	ZQ::common::MutexGuard gd(_observeeLock);
	for (std::vector<IObservee*>::iterator it= _observeeList.begin(); it != _observeeList.end(); )
	{
		if ((*it) == observee)
			it = _observeeList.erase(it);
		else
			it++;
	}
}

/*
//------------------------------------------
//ITCPEngine
//------------------------------------------
class ITCPEngine
{
public:
	ITCPEngine(const std::string& ip,int port,ZQ::common::Log& logger,TCPServer& server)
		:_ip(ip),
		_port(port),
		_logger(logger),
		_server(server)
	{}

	virtual ~ITCPEngine(){}

public:
	virtual bool startAt() = 0;
	virtual void stop() = 0;

	ZQ::common::Log&		_logger;
	const std::string&		_ip;
	int						_port;
	TCPServer&				_server;

};
*/

// ---------------------------------------
// class TCPSvcLoop
// ---------------------------------------
// Single event loop
class TCPSvcLoop : public ZQ::common::NativeThread, public ZQ::eloop::Loop, public ZQ::eloop::Interruptor
{
public:
	TCPSvcLoop(TCPServer& server, int cpuId=-1)
		: Loop(false), _server(server), Interruptor(*this), _watchDog(*this),
		_bQuit(false), _cpuId(cpuId)
	{
		if (_cpuId >=0)
			NativeThread::setCPUAffinity(_cpuId);
	}

	virtual ~TCPSvcLoop()
	{
		_server._logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPSvcLoop, "destructor!"));
	}

protected: // impl of NativeThread
	int run(void)
	{
		_server.onLoopThreadStart(*this);
		_server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "TCPSvcLoop starts affinite core[%d]"), _cpuId);

		int r = Loop::run(Loop::Default);
		_server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "LoopThread quit!"));
		return r;
	}

	void addSocket(int sock)
	{
		ZQ::common::MutexGuard gd(_LockSocket);
		_ListSocket.push_back(sock);
		Interruptor::wakeup();
	}

	void quit()
	{
		_bQuit = true;
		Interruptor::wakeup();
	}

protected: // impl of Interruptor
	void OnWakedUp()
	{
		// case 1. wakeup to quit
		if (_bQuit)
		{
			_server._logger(ZQ::common::Log::L_INFO, CLOGFMT(TCPSvcLoop, "wakedup to quit"));
			Interruptor::deactive();
			Loop::stop();
			return;
		}

		// case 1. wakeup to accept new connection
		ZQ::common::MutexGuard gd(_LockSocket);
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
				client->setWatchDog(&_watchDog);

			client->OnConnected(elpeSuccess);
		}
	}

	//virtual void doAccept(ElpeError status)
	//{
	//	if (status != elpeSuccess)
	//	{
	//		_logger(ZQ::common::Log::L_ERROR, CLOGFMT(SingleLoopTCPEngine, "doAccept() error code[%d] desc[%s]"),status,errDesc(status));
	//		return;
	//	}

	//	TCPConnection* client = _server.createPassiveConn();
	//	client->init(get_loop());
	//	if (_server._config.watchDogInterval > 0)
	//		client->setWatchDog(&_watchDog);

	//	if (accept((Stream*)client) == 0)
	//		((TCP*)client)->OnConnected(elpeSuccess);
	//	else client->disconnect();
	//}

	//virtual void OnClose()
	//{
	//	_loop.close();
	//	_server.signal();
	//}

private:
	bool               _bQuit;
	TCPServer&		   _server;
	std::list<int>				_ListSocket;
	ZQ::common::Mutex			_LockSocket;
	WatchDog         _watchDog;
	int _cpuId;
};
/*
// ---------------------------------------
// class SingleLoopTCPEngine
// ---------------------------------------
// Single event loop
class SingleLoopTCPEngine : public TCP, public ITCPEngine, public ZQ::common::NativeThread, public WatchDog::IObservee
{
public:
	SingleLoopTCPEngine(const std::string& ip, int port, ZQ::common::Log& logger, TCPServer& server)
		: ITCPEngine(ip, port, logger, server), _loop(false)
	{
		_watchDog.watch(this);
		_watchDog.watch(&server);
	}

	~SingleLoopTCPEngine()
	{
		_watchDog.unwatch(this);
		_watchDog.unwatch(&_server);
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine, "SingleLoopTCPEngine destructor!"));
	}

	// ---------------------------------------
	// class AsyncQuit
	// ---------------------------------------
	class AsyncQuit : public ZQ::eloop::Interruptor
	{
	public:
		virtual void OnAsync()
		{
			close();
		}

		virtual void OnClose()
		{
			SingleLoopTCPEngine* eng = (SingleLoopTCPEngine*)data;
			eng->close();
		}
	};

	virtual void OnTimer() {}
	virtual void OnUnwatch() { _async.send(); }

public:
	virtual bool startAt()
	{
		return ZQ::common::NativeThread::start();
	}

	virtual void stop()
	{
		if (_server._config.watchDogInterval > 0)
			_watchDog.close();
		else
			_async.send();
	}

	virtual int run(void)
	{
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine,"SingleLoopTCPEngine start"));
		ZQ::eloop::TCP::init(_loop);
		_async.data = this;
		_async.init(_loop);
		if (_server._config.watchDogInterval > 0)
		{
			_watchDog.init(_loop);
			_watchDog.start(0, _server._config.watchDogInterval);
		}

		if (bind4(_ip.c_str(),_port) < 0)
			return false;

		if (listen() < 0)
			return false;

		_server.onLoopThreadStart(_loop);
		int r=_loop.run(ZQ::eloop::Loop::Default);
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine,"SingleLoopTCPEngine quit!"));
		return r;
	}

	virtual void doAccept(ElpeError status)
	{
		if (status != elpeSuccess)
		{
			_logger(ZQ::common::Log::L_ERROR, CLOGFMT(SingleLoopTCPEngine, "doAccept() error code[%d] desc[%s]"),status,errDesc(status));
			return;
		}

		TCPConnection* client = _server.createPassiveConn();
		client->init(get_loop());
		if (_server._config.watchDogInterval > 0)
			client->setWatchDog(&_watchDog);

		if (accept((Stream*)client) == 0)
			((TCP*)client)->OnConnected(elpeSuccess);
		else client->disconnect();
	}

	virtual void OnClose()
	{
		_watchDog.unwatch(this);
		_loop.close();
		_server.signal();
	}

private:
	ZQ::eloop::Loop _loop;
	AsyncQuit		_async;
	WatchDog	_watchDog;
};

// ---------------------------------------
// class MultipleLoopHttpEngine
// ---------------------------------------
//Multiple event loops
class MultipleLoopTCPEngine : public ZQ::common::NativeThread, public ITCPEngine
{
public:
	MultipleLoopTCPEngine(const std::string& ip, int port, ZQ::common::Log& logger, TCPServer& server,const std::string& cpuIds)
		: ITCPEngine(ip,port,logger,server), _bRunning(false), _roundCount(0), _quitCount(0), _socket(0)
	{
		_threadCount = 0;
		std::stringstream sstr(cpuIds);
		std::string token;   
		while(getline(sstr, token, ','))  
		{  
			int cpuId = atoi(token.c_str());
			_threadCount++;
			LoopThread *pthread = new LoopThread(_server, *this, _logger);

			pthread->setCPUAffinity(cpuId);

			pthread->start();
			_vecThread.push_back(pthread);
			_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"cpuIds[%s] start loopThread cpuId:%d"),cpuIds.c_str(), cpuId);
		}
	}

	~MultipleLoopTCPEngine()
	{
		std::vector<LoopThread*>::iterator it = _vecThread.begin();
		while(it != _vecThread.end())
		{
			delete *it;
			*it = NULL;
			it = _vecThread.erase(it);
		}

		_vecThread.clear();
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopTCPEngine destructor!"));
	}

	// ---------------------------------------
	// class LoopThread
	// ---------------------------------------
	class LoopThread : public ZQ::common::NativeThread, public Async, public WatchDog::IObservee, ZQ::eloop::Loop
	{
	public:
		LoopThread(TCPServer& server, MultipleLoopTCPEngine& engine, ZQ::common::Log& logger)
			:_logger(logger), _engine(engine), Loop(false), _quit(false), _server(server)
		{
			_watchDog.watch(this);
			_watchDog.watch(&_server);
		}

		~LoopThread()
		{
			_watchDog.unwatch(this);
			_watchDog.unwatch(&_server);
			_logger(ZQ::common::Log::L_INFO, CLOGFMT(LoopThread,"LoopThread destructor!"));
		}

		virtual int run(void)
		{
			Async::init(*this);
			if (_server._config.watchDogInterval > 0)
			{
				_watchDog.init(*this);
				_watchDog.start(0,_server._config.watchDogInterval);
			}

			_server.onLoopThreadStart(*this);
			_logger(ZQ::common::Log::L_INFO, CLOGFMT(LoopThread, "LoopThread start run!"));
			int r = Loop::run(Loop::Default);
			_logger(ZQ::common::Log::L_INFO, CLOGFMT(LoopThread, "LoopThread quit!"));
			return r;
		}

		void addSocket(int sock)
		{
			ZQ::common::MutexGuard gd(_LockSocket);
			_ListSocket.push_back(sock);
		}

		virtual void OnTimer(){}
		virtual void OnUnwatch() { Handle::close(); }

		virtual void OnAsync()
		{
			ZQ::common::MutexGuard gd(_LockSocket);
			if (_quit)
			{
				if (_server._config.watchDogInterval > 0)
					_watchDog.close();
				else
					Handle::close();
				return;
			}

			while( !_ListSocket.empty())
			{
				int sock = _ListSocket.front();
				_ListSocket.pop_front();
				_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(LoopThread,"OnAsync recv sock = %d"),sock);

				TCPConnection* client = _server.createPassiveConn();
				if (NULL == client)
				{
					_logger(ZQ::common::Log::L_ERROR, CLOGFMT(LoopThread, "failed to create passive connection"));
					continue;
				}

				int r = client->init(get_loop());
				if (_server._config.watchDogInterval > 0)
					client->setWatchDog(&_watchDog);
				if (r != 0)
				{
					//printf("tcp init error:%s,name :%s\n", uv_strerror(r), uv_err_name(r));
					_logger(ZQ::common::Log::L_ERROR, CLOGFMT(LoopThread,"OnAsync tcp init err(%d) %s"),r,Handle::errDesc(r));
					delete client;
					continue;
				}

				r = ((TCP*) client)->connected_open(sock);
				if (r != 0)
				{
					_logger(ZQ::common::Log::L_ERROR, CLOGFMT(LoopThread, "OnAsync open socke err(%d) %s"), r, Handle::errDesc(r));
					client->disconnect();
					continue;
				}

				client->OnConnected(elpeSuccess);
			}
		}

		virtual void OnClose()
		{
			_watchDog.unwatch(this);
			Loop::close();
			_engine.QuitNotify(this);
		}

		void quit()
		{
			_quit = true;
		}

	private:

		ZQ::common::Log&			_logger;
		MultipleLoopTCPEngine&		_engine;
		TCPServer&					_server;
		std::list<int>				_ListSocket;
		ZQ::common::Mutex			_LockSocket;
		bool						_quit;
		WatchDog				_watchDog;
		//	int					_Count;
		//	ZQ::common::Mutex	_LockerCount;
	};

public:
	virtual bool startAt()
	{
#ifdef ZQ_OS_MSWIN
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
			return 0;
#endif

		_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		int reuse_value = 1;
		if( setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) != 0 )
			return false;

		if( _socket < 0 )
			return false;

		struct sockaddr_in serv;
		serv.sin_family=AF_INET;
		serv.sin_port=htons(_port);
		serv.sin_addr.s_addr=inet_addr(_ip.c_str());
		if(bind(_socket,(struct sockaddr*)&serv,sizeof(serv)) == -1)
		{
			_logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopTCPEngine,"socket bind error hint[%s:%d]"),_ip.c_str(),_port);
			return false;
		}

		if (0 != listen(_socket,10000))
		{
			_logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopTCPEngine,"socket listen error hint[%s:%d]"),_ip.c_str(),_port);
			return false;
		}

		_bRunning = true;
		return ZQ::common::NativeThread::start();
	}

	virtual void stop()
	{
		_bRunning = false;
#ifdef ZQ_OS_MSWIN
		closesocket(_socket);
		//clean WSA env
		WSACleanup(); 
#else
		// stop a blocking call
		shutdown(_socket, SHUT_RDWR);
		close(_socket);
#endif
		_socket = 0;
		std::vector<LoopThread*>::iterator iter;
		for (iter=_vecThread.begin();iter!=_vecThread.end();iter++)  
		{  
			(*iter)->quit();
			(*iter)->send();
		}
	}

	virtual int run(void)
	{
		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopHttpEngine start"));
		struct sockaddr_storage addr;
		while( _bRunning )
		{
			socklen_t size= (socklen_t) sizeof(addr);
			socket_t sock = accept( _socket, (struct sockaddr*)&addr, &size);
			if( sock < 0 || !_bRunning)
				continue;

			//int flags = fcntl(sock, F_GETFL, 0);
			//if ( -1 == flags)
			//{
			//	closesocket(sock);
			//	continue;
			//}
			//if( -1 == fcntl(sock, F_SETFL, flags | O_NONBLOCK) )
			//{
			//	closesocket(sock);
			//	continue;
			//}

			LoopThread* pthread = _vecThread[_roundCount];
			pthread->addSocket((int)sock);
			pthread->send();

			_roundCount = (++_roundCount) % _threadCount;
		}

		_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopHttpEngine quit"));
		return 0;
	}

	void QuitNotify(LoopThread* server)
	{
		ZQ::common::MutexGuard gd(_Lock);
		_quitCount++;
		if (_quitCount == _threadCount)
			_server.signal();
	}

private:
	bool	_bRunning;

#ifdef ZQ_OS_MSWIN
	typedef SOCKET	socket_t;
#else
	typedef int	socket_t;
#endif

	socket_t _socket;

	std::vector<LoopThread*> _vecThread;
	int		_roundCount;
	int		_threadCount;
	ZQ::common::Mutex _Lock;
	int		_quitCount;
};

*/

// ---------------------------------------
// class TCPConnection
// ---------------------------------------
uint TCPConnection::_enableHexDump = 0;

TCPConnection::TCPConnection(Loop& loop, ZQ::common::Log& log, const char* connId, TCPServer* tcpServer)
: TCP(loop), _logger(log), _pWakeup(NULL),/*_wakeup(*this),*/ _isConnected(false), _tcpServer(tcpServer), _watchDog(NULL), // _isShutdown(false),_isStop(false),
  _peerPort(0), _localPort(0), _stampBusySend(0)
{
	_pWakeup = new WakeUp(*this);
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
}

TCPConnection::~TCPConnection()
{
	if (_pWakeup)
		delete _pWakeup;
	_pWakeup = NULL
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

	if (_watchDog)
		_watchDog->watch(this);

	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "conn[%s] connected: %s"),_connId.c_str(), _linkstr.c_str());
}

bool TCPConnection::disconnect(bool bShutdown)
{
	ZQ::common::MutexGuard gd(_lkSend);
	Buffer::Queue NIL;
	std::swap(_queSend, NIL);

	if(_pWakeup)
		_pWakeup->close();
	//_wakeup.close();
	if (bShutdown)
		shutdown();
	else close();

	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "%s conn[%s] %s"), bShutdown?"shutting down" :"closing", _connId.c_str(), _linkstr.c_str());
}

void TCPConnection::OnClose()
{
	_isConnected = false;
	if (_tcpServer)
		_tcpServer->delConn(this);

	if (_watchDog)
		_watchDog->unwatch(this);

	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection, "OnClose() conn[%s] %s"),_connId.c_str(), _linkstr.c_str());
	_linkstr ="";

	TCP::OnClose();
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

	if (!is_active() || is_closing())
		return 0;

	_queSend.push(new Buffer(data, len));
	//return _wakeup.send();
	return _pWakeup->send();
}

void TCPConnection::OnShutdown(ElpeError status)
{
	_logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection,"OnShutdown() conn[%s] %s, error(%d): %s"),_connId.c_str(), _linkstr.c_str(), status,errDesc(status));
	close();
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

void TCPConnection::OnSendEnqueued()
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

	if (!is_active() || is_closing() || !_isConnected)
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
	for 

	return true;
}

void TCPServer::addConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_connMap[servant->connId()] = servant;
}

void TCPServer::delConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_connMap.erase(servant->connId());
	if (!_isStart&&_connMap.empty())
		_engine->stop();
}

TCPConnection* TCPServer::findConn( const std::string& connId)
{
	ZQ::common::MutexGuard gd(_connCountLock);
	ConnMAP::iterator it = _connMap.find(connId);
	if (it != _connMap.end())
		return it->second;
	return NULL;
}

TCPConnection* TCPServer::createPassiveConn()
{
	return new TCPConnection(_logger,NULL,this);
}

void TCPServer::signal()
{
	_sysWakeUp.signal();
}

void TCPServer::onLoopThreadStart(ZQ::eloop::Loop& loop)
{
	{
		ZQ::common::MutexGuard gd(_onStartLock);
		if (_isOnStart)
			return;
		else
			_isOnStart = true;
	}

	onStart(loop);
}

} }//namespace ZQ::eloop
