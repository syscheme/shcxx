#ifndef __asynchttp_socket_header_file__
#define __asynchttp_socket_header_file__

#include <ZQ_common_conf.h>
#include <NativeThread.h>
#include <assert.h>

#ifdef ZQ_OS_MSWIN
#	include <WinSock2.h>
#	include <ws2tcpip.h>
#	include <MSWSock.h>
#elif defined ZQ_OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#define SOCKET    int
//#define      MAXBUFFER     100
#else
#error "unsupported OS"
#endif

#include <map>
#include <vector>
#include <Pointer.h> //for smart pointer

#include "eventloop.h"

namespace LibAsync {
	
	class ZQ_COMMON_API Socket;
	class LingerTimer : public virtual Timer{
		public:
			typedef ZQ::common::Pointer<LingerTimer> Ptr;
			typedef ZQ::common::Pointer<Socket> SocketPtr;
			~LingerTimer();
			LingerTimer(SocketPtr sock);
		public:
			virtual void  onTimer();

		private:
			SocketPtr     socketPtr;
	};


	class  Socket : public virtual ZQ::common::SharedObject {
	protected:
		Socket(EventLoop& loop);
		Socket(EventLoop& loop, SOCKET sock);
	private:
		//disallow copy construction
		Socket( const Socket&);
		Socket& operator=(const Socket&);
	public:
	    typedef ZQ::common::Pointer<Socket>	Ptr;
		virtual ~Socket(void);

#ifdef ZQ_OS_MSWIN
		static bool	getAcceptEx();
		static bool getConnectEx();
#endif
		// use function create to make a socket instance
		static Socket::Ptr	create(EventLoop& loop);
		
		EventLoop&			getLoop() const  { return mLoop; }

		/// detect if current socket is alive for data transmission
		/// That is, if socket is connected, it's alive, 
		/// If socket is closed or just opened, it's NOT alive
		bool	alive() const;

		/// NOTE///////////////////////////////////
		/// 当前只关注client，那么server::accept的事情留到以后再说
		///////////////////////////////////////////  
		/// bind local address
		bool	bind(const std::string& ip, unsigned short port);

		/// listen and accept new connection
		/// remember to change net.core.somaxconn so that we can get a higher backlog from kernel view
		bool	accept( int backlog = 10 * 1000 );

		// get address information, can only apply to connected socket, false is returned if not
		bool	getLocalAddress(std::string& ip, unsigned short& port) const;
		bool	getPeerAddress(std::string& ip, unsigned short& port) const;

		/// 当前我们只考虑ip:port模式， 对于domain之类的需要DNS解析的东西暂时不考虑
		/// NOTE:DNS解析也是一个阻塞调用，不管是读取/etc/resolve.conf 还是tcp/udp DNS查询
		///      如果要支持这个就需要做一个异步的DNS resolver， 这个是以后的事情，现在不考虑
		/// 当connect请求成功发起以后，有两种可能
		///   1. 真的就立即连接成功了， 那么需要立即调用onConnected
		///   2. 返回WOULDBLOCK/EAGAIN之类的，这个时候返回成功，然后等待连接成功的事件，并且确认连接成功
		///       以后调用onConnected
		/// 出现任何错误导致无法成功发起连接请求的时候都返回false  
		bool	connect( const std::string& ip, unsigned short port );

		///after server side socket created, ListenSocket should invoke this method 
		///to make a onSocketConnected callback
		bool	initialServerSocket();
		void	onServerSocketInitialized();

	public:

		bool	setReuseAddr( bool reuse );
		bool	setSendBufSize( int size );
		bool	setRecvBufSize( int size );

#ifdef ZQ_OS_LINUX
		bool    setSysLinger();
		bool	setNoDelay( bool noDelay );
		bool	setCork( bool cork );
        // 向EPOLL中注册写事件，当该socket可写的时，通过回调onWritable通知该socket可发
		bool 	registerWrite();
		bool    setDeferAccept();
		bool    socketShutdown();
		bool    realClose();
		inline void    setLingerTime(uint64 time){ 
			mLingerTime = time;
		}
#endif//ZQ_OS

	protected:

		
		/// 发起接收数据的请求，user code必须保证在onError或者onRecved被调用之前buf可用，否则将会有未知错误发生
		/// 成功发起recv动作以后，如果真的收到了数据，那么需要直接调用onRecved
		/// 否则直接返回true， 然后等待事件发生，并且调用onRecved
		/// 任何导致无法成功发起recv动作的情况都返回false
		/// recv的语义是receive_some，也就是说如果你提供的buffer大小是10个字节，那么接收到[1,10]大小的数据都会返回    
		bool	recv( AsyncBuffer buf );
		bool	recv( AsyncBufferS& bufs );

		/// 发起发送数据的请求， user code必须保证在onError或者onSent被调用之前buf可用，否则将会有未知错误发生
		/// linux和windows对于这个实现差异很大， 那么
		/// 在linux下面，首先直接发送数据， 如果发送失败，那么会得到一个EAGAIN之类的错误
		/// 得到这个错误以后，把当前需要发送的数据记录下来，等待可以发送的事件到来，然后发送哪些未发送的数据。
		/// 所有的数据发送完毕以后调用onSent
		/// 在windows上面，这个就简单了，调用WSASend然后等待事件发生就可以了
		/// send的语义是send_all,也就是说必须是所有的数据都发送完毕以后才能调用onSent.中途出错的话就调用onError
		bool	send( AsyncBuffer buf );
		bool	send( const AsyncBufferS& bufs );
		
		//   发起发送数据的请求，user code通过返回值返回， 调用者需要跟句返回值来决定接下来的动作
		//   发送成功则返回发送的数据大小。
		virtual int     sendDirect(AsyncBuffer buf);
		virtual int     sendDirect(const AsyncBufferS& bufs);
		///NOTE:///////////////////////////////////////////
		///  由于可以在任何一个时间调用close
		///  所以事先上必须要考虑close以后socket对象是否还被epoll/IOCP引用，这个是一个关键点。
		///  否则就是遍地的crash dump  
		void	close();

		int		lastError() const;

	private:
		bool	open( int family, int type, int protocol );

		bool	isOpened() const;// return true if socket is created, false if closed

		bool	getNameInfo( bool local, std::string& ip, unsigned short& port ) const;
#ifdef ZQ_OS_LINUX
		//get the sent pos,
		// completeSize_凡瀹_____size
		//杩___浠ュ__send/recv__buffer涓_帮___瑁瑰凡缁_send/recv涓__ㄥ____buffer
		//int pos ___哄___帮_杈__send/recv涓___buffer________
		int  getCompletePos(const AsyncBufferS& bufs, int completeSize, int& pos);

		//瀹__bufs__iovec_扮___杞_?


		//startNum琛ㄧずbufs__瑕_send/recv__璧峰_buf
		//pos 琛ㄧず璧峰_buf宸茬_send/recv__浣_缃_涔_灏辨__send/recv__璧峰_浣_缃__	
		//maxNum __瀹_iov_扮_澶у_锛__叉瓒___

		void	mapBufsToIovs(const AsyncBufferS& bufs, struct iovec* iov, int startNum, int pos, int maxNum);

		virtual bool    acceptAction(bool firstAccept = false);
		virtual bool    recvAction();
		virtual bool    sendAction(bool firstSend = false);
		virtual void    errorAction(int err);
		
		/// 为了解决，socket shutdown之后调用recv的问题。
		/// 采用private权限保证外部以及子类再shutdown之后无法调用该函数
		/// 该函数仅供于socket shutdown之后去recv数据时调用
		bool	recv(bool shutdown);

#elif defined ZQ_OS_MSWIN
		bool	innerAccept();
#endif		
	protected:
		/// 在任何时候发生错误， onError都可能会被调用到。
		/// 例如在send的时候出错， onSent不会被调用到，而是onError会被调用到  
		virtual	void	onSocketError(int err){
			close();
		}

		virtual	void	onSocketConnected() { }

		virtual	void	onSocketRecved(size_t size) { }

		virtual void	onSocketSent(size_t size) { }
#ifdef ZQ_OS_LINUX
		virtual void    onWritable() { }
#endif
		virtual Socket::Ptr	onSocketAccepted( SOCKET sock ) {
#ifdef ZQ_OS_LINUX
			::close(sock);
#elif defined ZQ_OS_MSWIN
			closesocket(sock);
#endif //ZQ_OS
			return NULL;
		}
		

	private:
		friend	class EventLoop;
		friend class UDPSocket;

		EventLoop&			mLoop;
		int					mLastError;
		bool				mbAlive;
		bool                mRecValid;
		bool				mSendValid;
		bool				mbListenSocket;// is this socket used for listen

#ifdef ZQ_OS_MSWIN
		SOCKET					mSocket;
		SOCKET					mAcceptSocket;
		static LPFN_ACCEPTEX	mlpfnAcceptEx;
		static LPFN_CONNECTEX	mlpfnConnectEx;
#elif defined ZQ_OS_LINUX
		bool				innerSend( const AsyncBufferS& bufs );
		class PostponeSend : public AsyncWork {
		public:
			PostponeSend( Socket::Ptr sock ):
				AsyncWork(sock->getLoop()),
				mSock(sock){
			}
			virtual ~PostponeSend(){
			}
			typedef ZQ::common::Pointer<PostponeSend> Ptr;

			void send( const AsyncBufferS& buf ) {
				mBuf = buf;
				queueWork();
			}
		protected:
			virtual void onAsyncWork() {
				AsyncBufferS buffers(mBuf);
				mBuf.clear();
				mSock->innerSend(buffers);
			}
		private:
			Socket::Ptr		mSock;
			AsyncBufferS	mBuf;
		};

		int					mSocket;
		int                 mSocketEvetns;
		
		AsyncBufferS    	mRecBufs;
		size_t              mRecedSize;
		
		AsyncBufferS        mSendBufs;
		size_t              mSentSize;
		bool                mWriteable;
		bool             	mInEpoll;

		LingerTimer::Ptr    mLingerPtr;
		uint64              mLingerTime;
		AsyncBufferS        mLingerRecv;
		bool                mShutdown;
#else
#	error "unsupported OS"
#endif
	};
}//namespace LibAsync

#endif//__asynchttp_socket_header_file__
//vim: ts=4:sw=4:autoindent
