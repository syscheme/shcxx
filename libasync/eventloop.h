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
	�����кܶ��Ӧ�ö���Ҫtimer��֧�֡�
	��LibAsync�������ǿ��Խ���epoll_wait/GetQueuedCompletionStatus��ʵ�֡�
	*/
	class ZQ_COMMON_API Timer;
	class Timer: public virtual ZQ::common::SharedObject {
	public:	
		typedef ZQ::common::Pointer<Timer> Ptr;
		typedef boost::function<void()> 	FUNC_ONTIMER;

		virtual ~Timer();

		static Timer::Ptr create(EventLoop& loop);

		uint64				target() const { return mTarget; }

		/// ����timer��ʱ�䣬 ��λ�Ǻ���
		void				update( uint64 delta );

		///ȡ��һ����δ��ִ�е�timer�¼�
		///timer��ȡ���Ժ�onTimer���ᱻ���õ�
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
		/// �������Ҫ�յ�timerʱ�䣬��override�������
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
	EventLoop��������Ҫ�ǰ�װiocp/epoll������ȡ�¼�֪ͨ��
	ÿһ��loop������һ���߳����棬 ������ͬloop֮�䲻�໥���š�
	ÿһ��socketֻ�ܺ�һ��EventLoop�������ͬʱ�����ڸ�socket��timerҲ���������ͬһ��EventLoop��
	�������Ǿ���Ҫ���Ķ���̲߳���ͬһ�������������.
	ͬʱEventLoop����ҪΪ�����ṩTimer�¼�֪ͨ�������ʵ����һ��timer�¼���¼��
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
		/// ����thread������eventloop
		/// ����windows��˵������GetQueuedCompletionStatus
		/// ����linux��˵������epoll_wait
		bool	start();

		/// ��start��Ӧ��ֹͣthread
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

		///����loop,һ����˵���Ǽ����������һ����ɹ��������׳��κδ���
		///����������ᱻ���캯������
		///��Windows����CreateIoCompletionPort
		///��Linux����epoll_create
		///ʵ�ֵ�ʱ������ϸ�Ķ��ĵ�����ȷ�������������ȷ��
		void		createLoop( );

		///����loop
		void		destroyLoop( );

		///����EventLoop
		/// ��Windows�Ͽ���ͨ��PostQueuedCompletionStatus��ʵ��
		/// ��Linux�Ͽ���ͨ��eventfd��ʵ��
		/// ���߶���Ҫ��processEvent��������Դ�������¼����ˣ����Ǳ��������ѵ�
		///   ����˵����Ϊtimeout��
		void		wakeupLoop( );

		///�������OS-dependent event processing
		/// expireAt����������ʱ�䣬������ʱ����Ѿ��뵱ǰʱ��ǳ��ӽ������Ѿ�С�ڵ�ǰʱ�䡣��ô��
		/// ʹ��һ���趨�ĺ�С��ֵ������10ms
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
