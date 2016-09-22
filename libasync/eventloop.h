#ifndef __asynchttp_eventloop_header_file__
#define __asynchttp_eventloop_header_file__

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <ZQ_common_conf.h>
#include <Locks.h>
#include <NativeThread.h>

#ifdef ZQ_OS_MSWIN
#	include <WinSock2.h>
#	include <ws2tcpip.h>
#elif defined ZQ_OS_LINUX
#  include <sys/epoll.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netdb.h>
//#include <sys/eventfd.h>

#define EPOLLEVENTS 	128
#define BUFFERSIZE		1024
#else
#error "unsupported OS"
#endif

#include <map>
#include <set>
#include <vector>
#include <list>
#include <Pointer.h> //for smart pointer


namespace LibAsync {

	typedef enum _ErrorCode
	{
		ERR_ERROR			= -1,	//generic error
		ERR_EOF				= -2,	//end of stream, we also take CONNECTION_RESET as EOF due to lot of tcp application will do close a connection brutally
		ERR_INVAID			= -3,	//invalid parameter
		ERR_ADDRINUSE		= -4,	//address can not be bound
		ERR_ADDRNOTAVAIL	= -5,	//interface is not found
		ERR_TIMEOUT			= -6,	//timeout in operation
		ERR_CONNREFUSED		= -7,	//connection refused
		ERR_RECVFAIL    = -8, //revc return false
		ERR_SENDFAIL    = -9, //send return false
		ERR_EAGAIN      = -10,
		ERR_SOCKETVAIN = -11, // the socket can not been send/recv
		ERR_MEMVAIN = -12,   // the mem malloc err 
		ERR_BUFFERTOOBIG = -13,
		ERR_UDPSOCKETNOTALIVE = -14,
		//linux
		ERR_EPOLLREGISTERFAIL  = -21,
		ERR_EPOLLEXCEPTION = - 22,
		ERR_EOF2           = -23,
		ERR_EOF3           = -24
	}ErrorCode;

	const char* ErrorCodeToStr( ErrorCode c );

    class Socket;
    //	typedef SocketPtr;
    typedef ZQ::common::Pointer<Socket>		SocketPtr;
} //namespace LibAsync



namespace LibAsync {
	class ZQ_COMMON_API SocketAddrHelper{
	public:// inheritance is not allowed
		SocketAddrHelper(bool bTcp = true);
		SocketAddrHelper(const std::string& ip, unsigned short port, bool bTcp = true);
		SocketAddrHelper(const std::string& ip, const std::string& service, bool bTcp = true);
		~SocketAddrHelper();
		void					init(bool bTcp = true);
		bool					parse( const std::string& ip, const std::string& service, bool bTcp = true);
		bool					parse( const std::string& ip, unsigned short port, bool bTcp = true);
		const struct addrinfo*	info() const;
		bool					multicast() const;
	private:
		
		void					clear();
	private:
		struct addrinfo		mHint;
		struct addrinfo*	mInfo;
		bool				mDecodeOk;
	};


#ifdef ZQ_OS_MSWIN	
	typedef enum _OperateType
	{
		OP_CONNECT,
		OP_ACCEPT,
		OP_SEND,
		OP_RECV
	}OperateType;

	typedef struct _IOCP_OVERLAPPED
	{
		OVERLAPPED	overlapped;
		WSABUF*	buf;
		OperateType	opType;
        SocketPtr sockRef;
	}IOCP_OVERLAPPED, *PIOCP_OVERLAPPED;
#elif defined ZQ_OS_LINUX
	class DummyEventFd {
	public:
		DummyEventFd();
		~DummyEventFd();
		bool create();
		void destroy();
		void read(); // being wakeuped
		void write();// wakeup
		bool isA( int fd ) const;
		int fd() const {
			return mSockPair[0];
		}
	private:
		int		mSockPair[2];
	};
#endif

	struct ZQ_COMMON_API AsyncBuffer{
		char*		base;
		size_t		len;
		AsyncBuffer():base(NULL),len(0){}
		AsyncBuffer( char* a, size_t l):base(a),len(l){}
		void reset() { base = NULL; len = 0; }
	};
	typedef std::vector<AsyncBuffer>	AsyncBufferS;

	size_t buffer_size( const AsyncBufferS& bufs);

    class ZQ_COMMON_API BufferHelper
    {
    public:
        BufferHelper(const AsyncBuffer& buf)
			:_bufs(1,buf) { 
			_dataSize = buffer_size(_bufs);
		}
        BufferHelper(const AsyncBufferS& bufs)
			: _bufs(bufs) { 
			_dataSize = buffer_size(_bufs);
		}
        virtual ~BufferHelper() {}

        void	adjust( size_t sentSize);

        bool 	isEOF() const;

        AsyncBufferS getBuffers() const {return _bufs;}

		AsyncBufferS getAt( size_t expectSize );

		inline size_t	size() const { return _dataSize; }

    private:
        AsyncBufferS 	_bufs;
		size_t			_dataSize;
	};


	class EventLoop;

	/**
	我们有很多的应用都需要timer的支持。
	在LibAsync里面我们可以借助epoll_wait/GetQueuedCompletionStatus来实现。
	*/
	class ZQ_COMMON_API Timer;
	class Timer: public virtual ZQ::common::SharedObject {
	public:	
		typedef ZQ::common::Pointer<Timer> Ptr;
		typedef boost::function<void()> 	FUNC_ONTIMER;

		virtual ~Timer();

		static Timer::Ptr create(EventLoop& loop);

		uint64				target() const { return mTarget; }

		/// 设置timer的时间， 单位是毫秒
		void				update( uint64 delta );

		///取消一个还未被执行的timer事件
		///timer被取消以后onTimer不会被调用到
		void				cancel();

		void				updateTimer( uint64 delta ) {
			update(delta);
		}

		void				cancelTimer() {
			cancel();
		}

		void				bindCB( FUNC_ONTIMER cb );

	protected:
		Timer(EventLoop& loop);

	private:
		Timer( const Timer& );
		Timer& operator=( const Timer&);


	protected:
		friend class EventLoop;
		void				onTimerEvent();
		/// 如果你想要收到timer时间，请override这个函数
		virtual	void		onTimer( ) { }
	private:
		EventLoop&			mLoop;
		uint64				mTarget;
		FUNC_ONTIMER		mFuncCB;
	};

	/**
	NOTE: DO LOCK YOUR INSTANCE IF YOU USE AsyncWork in multithread environment
	*/
	class ZQ_COMMON_API AsyncWork : public virtual ZQ::common::SharedObject {
	public:
		typedef ZQ::common::Pointer<AsyncWork>	Ptr;
		typedef boost::function<void()> FUNC_ONASYNCWORK;
		virtual ~AsyncWork();

		static AsyncWork::Ptr create(EventLoop& loop);

		/// queueWork will wakeup event loop and let it have an oppotunity to proceed.
		/// every single queueWork will lead to one onAsyncWork callback
		void	queueWork();

		void	bindCB( FUNC_ONASYNCWORK cb );

		EventLoop& getLoop() {
			return mLoop;
		}

	protected:
		friend class EventLoop;
		AsyncWork( EventLoop& loop);
		void			onWorkExecute( );

		virtual void	onAsyncWork( ) {
		}

	private:
		AsyncWork( const AsyncWork& );
		AsyncWork& operator=( const AsyncWork& );

	private:
		EventLoop&		mLoop;
		bool			mWorkQueued;
		FUNC_ONASYNCWORK mFuncCB;
	};

	/**
	EventLoop的作用主要是包装iocp/epoll用来获取事件通知。
	每一个loop运行在一个线程里面， 各个不同loop之间不相互干扰。
	每一个socket只能和一个EventLoop相关联，同时操作于该socket的timer也必须关联于同一个EventLoop，
	否则，我们就需要关心多个线程操作同一个对象的事情了.
	同时EventLoop还需要为我们提供Timer事件通知，这个其实就是一个timer事件记录表。
	*/

	class LoopCenter {
	public:
		virtual ~LoopCenter() {};
		virtual void	addSocket( int id ) = 0;
		virtual void	removeSocket( int id ) = 0;
	};

	class ZQ_COMMON_API EventLoop : public ZQ::common::NativeThread {
	public:
		EventLoop(ZQ::common::Log& log, int cpuid = -1, LoopCenter* center = NULL, int mId = 0 );
		//EventLoop(int cpuid = -1);
		virtual ~EventLoop();
		ZQ::common::Log&  getLog(){ return mLog;}
	public:
		/// 启动thread，运行eventloop
		/// 对于windows来说是运行GetQueuedCompletionStatus
		/// 对于linux来说是运行epoll_wait
		bool	start();

		/// 和start对应，停止thread
		void	stop();

		void	addAsyncWork(AsyncWork::Ptr work);

		bool	addTimer( Timer::Ptr t );
		void	removeTimer( Timer::Ptr t );

#ifdef ZQ_OS_MSWIN
		bool	addSocket(SocketPtr sock);

#elif defined ZQ_OS_LINUX
		bool	registerEvent(SocketPtr sock, int event);
		bool	unregisterEvent(SocketPtr sock, int event);
		void  ignoreSigpipe();
#endif

		inline void increateSockCount() { 
			if(mLoopCenter)
				mLoopCenter->addSocket(mLoopId);
		}

		inline void decreateSockCount() {
			if( mLoopCenter)
				mLoopCenter->removeSocket( mLoopId );
		}

	private:
		int			run();

	private:

		///创建loop,一般来说我们假设这个动作一定会成功，不会抛出任何错误
		///这个函数将会被构造函数调用
		///在Windows上是CreateIoCompletionPort
		///在Linux上是epoll_create
		///实现的时候请仔细阅读文档，以确保输入参数是正确的
		void		createLoop( );

		///销毁loop
		void		destroyLoop( );

		///唤醒EventLoop
		/// 在Windows上可以通过PostQueuedCompletionStatus来实现
		/// 在Linux上可以通过eventfd来实现
		/// 两者都需要在processEvent里面区别对待是真的事件到了，还是被主动唤醒的
		///   或者说是因为timeout了
		void		wakeupLoop( );

		///这个用于OS-dependent event processing
		/// expireAt期望醒来的时间，如果这个时间点已经离当前时间非常接近或者已经小于当前时间。那么就
		/// 使用一个设定的很小的值，例如10ms
		void		processEvent( int64 expireAt );
	private:
		struct TimerInfo{
			int64		target;
			Timer::Ptr	timer;
			TimerInfo():target(0),timer(NULL){}
			TimerInfo(Timer::Ptr t):target(t->target()),timer(t){}
			bool operator<( const TimerInfo& rhs ) const {
				if( target < rhs.target )
					return true;
				else if(target == rhs.target )
					return timer.get() < rhs.timer.get();
				else 
					return false;
			}
		};
		typedef std::set<TimerInfo>			TIMERINFOS;
		TIMERINFOS			mTimers;
		int64				mNextWakeup;
		ZQ::common::Mutex	mLocker;
		bool				mbRunning;
#ifdef ZQ_OS_MSWIN
		HANDLE				mhIOCP;
#elif defined ZQ_OS_LINUX
	  int          mEpollFd;	  
	  DummyEventFd mDummyEventFd;
#endif
	  std::list<AsyncWork::Ptr>	mAsyncWorks;
	  bool						mbAsyncWorkMessagePosted;

	  int					mCpuId;
	  ZQ::common::Log&       mLog;
	  int64                  mPreTime;
	  LoopCenter*			mLoopCenter;
	  int					mLoopId;
	};

}//namespace LibAsync

#endif//__asynchttp_eventloop_header_file__
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312
