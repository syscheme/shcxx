#include "TCPServer.h"
#include <sstream>

namespace ZQ {
namespace eloop {

//------------------------------------------
//ITCPEngine
//------------------------------------------
class ITCPEngine
{
public:
	ITCPEngine(const std::string& ip,int port,ZQ::common::Log& logger,TCPServer& server)
		:_ip(ip),
		_port(port),
		_Logger(logger),
		_server(server)
	{}

	virtual ~ITCPEngine(){}

public:
	virtual bool startAt() = 0;
	virtual void stop() = 0;

	ZQ::common::Log&		_Logger;
	const std::string&		_ip;
	int						_port;
	TCPServer&				_server;

};


// ---------------------------------------
// class SingleLoopTCPEngine
// ---------------------------------------
// Single event loop
class SingleLoopTCPEngine:public TCP,public ITCPEngine,public ZQ::common::NativeThread
{
public:
	SingleLoopTCPEngine(const std::string& ip,int port,ZQ::common::Log& logger,TCPServer& server)
		:ITCPEngine(ip,port,logger,server)
	{

	}
	~SingleLoopTCPEngine()
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine,"SingleLoopTCPEngine destructor!"));
	}

	// ---------------------------------------
	// class AsyncQuit
	// ---------------------------------------
	class AsyncQuit:public ZQ::eloop::Async
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

public:
	virtual bool startAt()
	{
		return ZQ::common::NativeThread::start();
	}
	virtual void stop()
	{
		_async.send();
	}
	virtual int run(void)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine,"SingleLoopTCPEngine start"));
		ZQ::eloop::TCP::init(_loop);
		_async.data = this;
		_async.init(_loop);
		if (bind4(_ip.c_str(),_port) < 0)
			return false;

		if (listen() < 0)
			return false;

		int r=_loop.run(ZQ::eloop::Loop::Default);
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(SingleLoopTCPEngine,"SingleLoopTCPEngine quit!"));
		return r;
	}

	virtual void doAccept(ElpeError status)
	{
		if (status != elpeSuccess)
		{
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(SingleLoopTCPEngine,"doAccept() error code[%d] desc[%s]"),status,errDesc(status));
			return;
		}

		TCPConnection* client = _server.createPassiveConn();
		client->init(get_loop());

		if (accept((Stream*)client) == 0) {
			client->start();
		}
		else {
			client->stop();
		}
	}

	virtual void OnClose()
	{
		_loop.close();
		_server.single();
	}

private:
	ZQ::eloop::Loop _loop;
	AsyncQuit		_async;
};


// ---------------------------------------
// class MultipleLoopHttpEngine
// ---------------------------------------
//Multiple event loops
class MultipleLoopTCPEngine:public ZQ::common::NativeThread,public ITCPEngine
{
public:
	MultipleLoopTCPEngine(const std::string& ip,int port,ZQ::common::Log& logger,TCPServer& server)
		:ITCPEngine(ip,port,logger,server), _bRunning(false), _roundCount(0), _quitCount(0), _socket(0)
	{
		CpuInfo cpu;
		_threadCount = cpu.getCpuCount();

		for (int i = 0;i < _threadCount;i++)
		{
			ServantThread *pthread = new ServantThread(_server, *this,_Logger);

			pthread->setCPUAffinity(i);

			pthread->start();
			_vecThread.push_back(pthread);
			_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"cpuId:%d,cpuCount:%d,mask:%d"),i,_threadCount,1<<i);
		}
	}
	~MultipleLoopTCPEngine()
	{
		std::vector<ServantThread*>::iterator it = _vecThread.begin();
		while(it != _vecThread.end())
		{
			delete *it;
			*it = NULL;
			it = _vecThread.erase(it);
		}

		_vecThread.clear();
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopTCPEngine destructor!"));
	}


	// ---------------------------------------
	// class ServantThread
	// ---------------------------------------
	class ServantThread:public ZQ::common::NativeThread,public Async
	{
	public:
		ServantThread(TCPServer& server,MultipleLoopTCPEngine& engine,ZQ::common::Log& logger)
			:_Logger(logger), _engine(engine),	_quit(false), _server(server)
		{
			_loop = new Loop(false);
		}

		~ServantThread()
		{
			_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread destructor!"));
		}

		virtual int run(void)
		{
			Async::init(*_loop);
			_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread start run!"));
			int r = _loop->run(Loop::Default);
			_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ServantThread,"ServantThread quit!"));
			return r;
		}

		void addSocket(int sock)
		{
			ZQ::common::MutexGuard gd(_LockSocket);
			_ListSocket.push_back(sock);
		}

		virtual void OnAsync()
		{
			ZQ::common::MutexGuard gd(_LockSocket);
			if (_quit)
			{
				close();
				return;
			}
			while( !_ListSocket.empty())
			{
				int sock = _ListSocket.front();
				_ListSocket.pop_front();
				_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(ServantThread,"OnAsync recv sock = %d"),sock);

				TCPConnection* client = _server.createPassiveConn();
				int r = client->init(get_loop());
				if (r != 0)
				{
					//printf("tcp init error:%s,name :%s\n", uv_strerror(r), uv_err_name(r));
					_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServantThread,"OnAsync tcp init errorcode[%d] describe[%s]"),r,Handle::errDesc(r));
					delete client;
					continue;
				}

				r = client->connected_open(sock);
				if (r != 0)
				{
					_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(ServantThread,"OnAsync open socke errorcode[%d] describe[%s]"),r,Handle::errDesc(r));
					client->stop();
					continue;
				}
				client->start();
			}
		}

		virtual void OnClose()
		{
			_loop->close();
			if (_loop != NULL)
			{
				delete _loop;
			}
			_engine.QuitNotify(this);
		}

		void quit(){ _quit = true;}

	private:
		ZQ::common::Log&			_Logger;
		Loop						*_loop;
		MultipleLoopTCPEngine&		_engine;
		TCPServer&					_server;
		std::list<int>				_ListSocket;
		ZQ::common::Mutex			_LockSocket;
		bool						_quit;
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

		_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

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
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopTCPEngine,"socket bind error hint[%s:%d]"),_ip.c_str(),_port);
			return false;
		}

		if (0 != listen(_socket,10000))
		{
			_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(MultipleLoopTCPEngine,"socket listen error hint[%s:%d]"),_ip.c_str(),_port);
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
		std::vector<ServantThread*>::iterator iter;
		for (iter=_vecThread.begin();iter!=_vecThread.end();iter++)  
		{  
			(*iter)->quit();
			(*iter)->send();
		}
	}

	virtual int run(void)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopHttpEngine start"));
		struct sockaddr_storage addr;
		while( _bRunning )
		{
			socklen_t size= (socklen_t)sizeof( addr );
			int sock = accept( _socket, (struct sockaddr*)&addr, &size);
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

			ServantThread* pthread = _vecThread[_roundCount];
			pthread->addSocket(sock);
			pthread->send();

			_roundCount = (++_roundCount) % _threadCount;
		}

		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(MultipleLoopTCPEngine,"MultipleLoopHttpEngine quit"));
		return 0;
	}

	void QuitNotify(ServantThread* sev)
	{
		ZQ::common::MutexGuard gd(_Lock);
		_quitCount++;
		if (_quitCount == _threadCount)
			_server.single();
	}

private:
	bool	_bRunning;
	int		_socket;
	std::vector<ServantThread*> _vecThread;
	int		_roundCount;
	int		_threadCount;
	ZQ::common::Mutex _Lock;
	int		_quitCount;
};

// ---------------------------------------
// class TCPConnection
// ---------------------------------------
bool TCPConnection::start()
{
	_isConnected = true;
	read_start();
	initHint();
	if (_tcpServer)
	{
		_Logger(ZQ::common::Log::L_DEBUG, CLOGFMT(TCPConnection,"new connection from [%s]"),_Hint.c_str());
		_tcpServer->addConn(this);
	}
	return onStart();
}

bool TCPConnection::stop()
{
	_isConnected = false;
	shutdown();
	return onStop();
}

void TCPConnection::initHint()
{
	char peerIp[17] = {0};
	int peerPort = 0;
	getpeerIpPort(peerIp,peerPort);

	char localIp[17] = {0};
	int localPort = 0;
	getlocaleIpPort(localIp, localPort);


	std::ostringstream oss;
	if (_tcpServer == NULL)
		oss<<"["<<localIp<<":"<<localPort<<"==>"<<peerIp<<":"<<peerPort<<"]";
	else
		oss<<"["<<peerIp<<":"<<peerPort<<"==>"<<localIp<<":"<<localPort<<"]";
	_Hint = oss.str();
}


void TCPConnection::OnClose()
{
	if (_tcpServer)
		_tcpServer->delConn(this);
	delete this;
}

void TCPConnection::OnShutdown(ElpeError status)
{
	if (status != elpeSuccess)
		_Logger(ZQ::common::Log::L_ERROR, CLOGFMT(TCPConnection,"shutdown error code[%d] Description[%s]"),status,errDesc(status));

	close();
}


// ---------------------------------------
// class TCPServer
// ---------------------------------------
bool TCPServer::start()
{
	if (_isStart)
		return true;

	if (_Config.mode == MULTIPE_LOOP_MODE)
		_engine = new MultipleLoopTCPEngine(_Config.host, _Config.port, _Logger, *this);
	else
		_engine = new SingleLoopTCPEngine(_Config.host,_Config.port,_Logger,*this);

	_isStart = true;
	_engine->startAt();
	return onStart();
}

bool TCPServer::stop()
{
	if (!_isStart)
		return true;

	_isStart = false;
	if (_PassiveConn.empty())
	{
		_engine->stop();
	}
	else
	{
		ConnList::iterator itconn;
		for(itconn = _PassiveConn.begin();itconn != _PassiveConn.end();itconn++)
			(*itconn)->shutdown();
	}

	_sysWakeUp.wait(-1);
	if (_engine != NULL)
	{
		delete _engine;
		_engine = NULL;
	}
	return onStop();
}

void TCPServer::addConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_PassiveConn.insert(servant);
}

void TCPServer::delConn( TCPConnection* servant )
{
	ZQ::common::MutexGuard gd(_connCountLock);
	_PassiveConn.erase(servant);
	if (!_isStart&&_PassiveConn.empty())
		_engine->stop();
}

TCPConnection* TCPServer::createPassiveConn()
{
	return new TCPConnection(_Logger,this);
}

void TCPServer::single()
{
	_sysWakeUp.signal();
}

} }//namespace ZQ::eloop