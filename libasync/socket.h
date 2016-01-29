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
		/// ��ǰֻ��עclient����ôserver::accept�����������Ժ���˵
		///////////////////////////////////////////  
		/// bind local address
		bool	bind(const std::string& ip, unsigned short port);

		/// listen and accept new connection
		/// remember to change net.core.somaxconn so that we can get a higher backlog from kernel view
		bool	accept( int backlog = 10 * 1000 );

		// get address information, can only apply to connected socket, false is returned if not
		bool	getLocalAddress(std::string& ip, unsigned short& port) const;
		bool	getPeerAddress(std::string& ip, unsigned short& port) const;

		/// ��ǰ����ֻ����ip:portģʽ�� ����domain֮�����ҪDNS�����Ķ�����ʱ������
		/// NOTE:DNS����Ҳ��һ���������ã������Ƕ�ȡ/etc/resolve.conf ����tcp/udp DNS��ѯ
		///      ���Ҫ֧���������Ҫ��һ���첽��DNS resolver�� ������Ժ�����飬���ڲ�����
		/// ��connect����ɹ������Ժ������ֿ���
		///   1. ��ľ��������ӳɹ��ˣ� ��ô��Ҫ��������onConnected
		///   2. ����WOULDBLOCK/EAGAIN֮��ģ����ʱ�򷵻سɹ���Ȼ��ȴ����ӳɹ����¼�������ȷ�����ӳɹ�
		///       �Ժ����onConnected
		/// �����κδ������޷��ɹ��������������ʱ�򶼷���false  
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
        // ��EPOLL��ע��д�¼�������socket��д��ʱ��ͨ���ص�onWritable֪ͨ��socket�ɷ�
		bool 	registerWrite();
		bool    setDeferAccept();
		bool    socketShutdown();
		bool   	socketShutdownStaus(){ return mLingerTime; }
		bool    realClose(int type = 0);
		inline void    setLingerTime(uint64 time){ 
			mLingerTime = time;
		}
#endif//ZQ_OS

	protected:

		
		/// ����������ݵ�����user code���뱣֤��onError����onRecved������֮ǰbuf���ã����򽫻���δ֪������
		/// �ɹ�����recv�����Ժ��������յ������ݣ���ô��Ҫֱ�ӵ���onRecved
		/// ����ֱ�ӷ���true�� Ȼ��ȴ��¼����������ҵ���onRecved
		/// �κε����޷��ɹ�����recv���������������false
		/// recv��������receive_some��Ҳ����˵������ṩ��buffer��С��10���ֽڣ���ô���յ�[1,10]��С�����ݶ��᷵��    
		bool	recv( AsyncBuffer buf );
		bool	recv( AsyncBufferS& bufs );

		/// ���������ݵ����� user code���뱣֤��onError����onSent������֮ǰbuf���ã����򽫻���δ֪������
		/// linux��windows�������ʵ�ֲ���ܴ� ��ô
		/// ��linux���棬����ֱ�ӷ������ݣ� �������ʧ�ܣ���ô��õ�һ��EAGAIN֮��Ĵ���
		/// �õ���������Ժ󣬰ѵ�ǰ��Ҫ���͵����ݼ�¼�������ȴ����Է��͵��¼�������Ȼ������Щδ���͵����ݡ�
		/// ���е����ݷ�������Ժ����onSent
		/// ��windows���棬����ͼ��ˣ�����WSASendȻ��ȴ��¼������Ϳ�����
		/// send��������send_all,Ҳ����˵���������е����ݶ���������Ժ���ܵ���onSent.��;����Ļ��͵���onError
		bool	send( AsyncBuffer buf );
		bool	send( const AsyncBufferS& bufs );
		
		//   ���������ݵ�����user codeͨ������ֵ���أ� ��������Ҫ���䷵��ֵ�������������Ķ���
		//   ���ͳɹ��򷵻ط��͵����ݴ�С��
		virtual int     sendDirect(AsyncBuffer buf);
		virtual int     sendDirect(const AsyncBufferS& bufs);
		///NOTE:///////////////////////////////////////////
		///  ���ڿ������κ�һ��ʱ�����close
		///  ���������ϱ���Ҫ����close�Ժ�socket�����Ƿ񻹱�epoll/IOCP���ã������һ���ؼ��㡣
		///  ������Ǳ�ص�crash dump  
		void	close();

		int		lastError() const;

	private:
		bool	open( int family, int type, int protocol );

		bool	isOpened() const;// return true if socket is created, false if closed

		bool	getNameInfo( bool local, std::string& ip, unsigned short& port ) const;
#ifdef ZQ_OS_LINUX
		//get the sent pos,
		// completeSize�_�已�_�__�__size
		//�_�__以�__send/recv�__buffer个�_��_�__裹已�_send/recv�_�_��__�__buffer
		//int pos �_��_�_��__�_��_�_�_�send/recv�_�__buffer�__�__�__�__
		int  getCompletePos(const AsyncBufferS& bufs, int completeSize, int& pos);

		//�_�_�bufs�__iovec�_��_�__转�_?


		//startNum表示bufs�__�_send/recv�__起�_buf
		//pos 表示起�_buf已�_send/recv�__�_置�_�_就�_��_�次send/recv�__起�_�_置�__	
		//maxNum �__�_iov�_��_大�_�_�_�止�_�__

		void	mapBufsToIovs(const AsyncBufferS& bufs, struct iovec* iov, int startNum, int pos, int maxNum);

		virtual bool    acceptAction(bool firstAccept = false);
		virtual bool    recvAction();
		virtual bool    sendAction(bool firstSend = false);
		virtual void    errorAction(int err);
		
		/// Ϊ�˽����socket shutdown֮�����recv�����⡣
		/// ����privateȨ�ޱ�֤�ⲿ�Լ�������shutdown֮���޷����øú���
		/// �ú���������socket shutdown֮��ȥrecv����ʱ����
		bool	recv(bool shutdown);

#elif defined ZQ_OS_MSWIN
		bool	innerAccept();
#endif		
	protected:
		/// ���κ�ʱ�������� onError�����ܻᱻ���õ���
		/// ������send��ʱ����� onSent���ᱻ���õ�������onError�ᱻ���õ�  
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
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312
