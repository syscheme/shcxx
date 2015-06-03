#include <sstream>
#include "socket.h"

namespace LibAsync{

	SocketAddrHelper::SocketAddrHelper()
	:mInfo(NULL){
		init();
	}

	SocketAddrHelper::SocketAddrHelper(const std::string& ip, const std::string& service)
		:mInfo(NULL){
		init();
		parse(ip,service);
	}

	SocketAddrHelper::SocketAddrHelper(const std::string& ip, unsigned short port)
		:mInfo(NULL) {
		init(true);
		std::ostringstream oss;oss<<port;
		parse(ip,oss.str());
	}
	SocketAddrHelper::~SocketAddrHelper(){
		clear();
	}

	void SocketAddrHelper::init(bool bTcp) {
		memset(&mHint,0,sizeof(mHint));
		mHint.ai_family = AF_UNSPEC;
		mHint.ai_protocol = bTcp ? IPPROTO_TCP:IPPROTO_UDP;
		mHint.ai_socktype = bTcp ? SOCK_STREAM: SOCK_DGRAM;
	}
	
	bool SocketAddrHelper::parse( const std::string& ip, unsigned short port) {
		std::ostringstream oss;oss<<port;
		return parse(ip, oss.str());
	}

	bool SocketAddrHelper::parse(const std::string& ip, const std::string& service){
		clear();
		if(getaddrinfo(ip.c_str(),service.c_str(),&mHint,&mInfo) != 0 ) {
			mDecodeOk = false;
			return false;
		}
		mDecodeOk = true;
		return true;
	}

	void SocketAddrHelper::clear(){
		if(mInfo)
			freeaddrinfo(mInfo);
		mInfo = NULL;
		mDecodeOk = false;
	}

	const struct addrinfo* SocketAddrHelper::info() const {
		return ( mDecodeOk && mInfo )? mInfo : NULL;
	}

	bool	SocketAddrHelper::multicast() const {
		assert(mInfo != NULL);
		if(!mInfo)
			return false;

		const struct addrinfo* addr = mInfo;
		bool bIPv4 = addr->ai_family == AF_INET;
		if( bIPv4 ) { 
			const struct sockaddr_in* in4 = (const struct sockaddr_in*)(mInfo->ai_addr);
			long ip = ntohl(in4->sin_addr.s_addr);
			return IN_CLASSD(ip); //TODO: not very accurate, will be changed
		} else { //ipv6 
			const struct sockaddr_in6* in6 = (const struct sockaddr_in6*)(mInfo->ai_addr);
			return *(unsigned short*)((char*) (&(in6->sin6_addr))) ==0xFFFF;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	class ServerSocketInitialize : public AsyncWork {
	public:
		ServerSocketInitialize(EventLoop& loop, SocketPtr sock):
		  AsyncWork(loop), mSock(sock){
		  }
	public:
		void	initializeServerSocket( ){
			queueWork();
		}

	protected:
		virtual void	onAsyncWork( );
	private:
		SocketPtr		mSock;
	};
	////
	void ServerSocketInitialize::onAsyncWork( ) {
		if(!mSock)
			return;
		mSock->onServerSocketInitialized();
	}

	//////////////////////////////////////////////////////////////////////////
	Socket::Socket(EventLoop& loop)
	:mLoop(loop),
	mLastError(0),
	mbAlive(false),
	mRecValid(false),
	mSendValid(false),
	mbListenSocket(false),
	mSocket(-1)
#ifdef ZQ_OS_LINUX
	,mSocketEvetns(0),
	mRecedSize(0),
	mSentSize(0),
	mWriteable(false)
#endif
	{
	}

	Socket::Socket(EventLoop& loop, SOCKET sock)
	:mLoop(loop),
	mLastError(0),
	mbAlive(true),
	mRecValid(true),
	mSendValid(true),
	mbListenSocket(false),
	mSocket(sock)
#ifdef ZQ_OS_LINUX
	,mSocketEvetns(0),
	mRecedSize(0),
	mSentSize(0),
	mWriteable(false)

#endif//
	{
	}

	bool Socket::initialServerSocket() {
		(new ServerSocketInitialize(mLoop,this))->initializeServerSocket();
		return true;
	}

	void Socket::onServerSocketInitialized() {
#ifdef ZQ_OS_MSWIN
		if(!mLoop.addSocket(this)) {
			onSocketError(ERR_ERROR);
			return;
		} 
#endif//ZQ_OS		
		onSocketConnected();
		return;
	}

	Socket::~Socket(){
		close();
	}

	Socket::Ptr Socket::create(EventLoop& loop) {
		return new Socket(loop);
	}
	bool Socket::alive() const{
		return mbAlive;
	}

	bool Socket::bind(const std::string& ip, unsigned short port) {
		SocketAddrHelper helper;
		if(!helper.parse(ip,port))
			return false;
		const struct addrinfo* info = helper.info();
		assert(info != NULL && "should never be false");
		if(!isOpened()) {			
			if(!open(info->ai_family, info->ai_socktype, info->ai_protocol))
				return false;			
		}
		if(alive()) {
			assert(false);// abort in DEBUG mode
			return false;//can't bind a already connected socket
		}
#ifdef ZQ_OS_MSWIN
#define socklen_t int
#endif
		setReuseAddr(true);
		return ::bind(mSocket,info->ai_addr, (socklen_t)info->ai_addrlen) == 0;
	}

	bool Socket::getLocalAddress(std::string& ip, unsigned short& port) const {
		return getNameInfo(true, ip, port);
	}

	bool Socket::getPeerAddress(std::string& ip, unsigned short& port) const {
		return getNameInfo(false, ip, port);
	}

	bool Socket::getNameInfo( bool local, std::string& ip, unsigned short& port ) const {
		struct sockaddr_storage addr;
#ifdef ZQ_OS_MSWIN
#define socklen_t            int
#endif
		socklen_t addrSize = sizeof(addr);
		if( local ) {
			if(getsockname(mSocket,(struct sockaddr*)&addr, &addrSize) != 0)
				return false;
		} else {
			if(getpeername(mSocket,(struct sockaddr*)&addr, &addrSize) != 0)
				return false;
		}
		char strIp[32] = {0};
		char strPort[16] = {0};
		if( getnameinfo((const struct sockaddr*)&addr, addrSize, 
			strIp,sizeof(strIp)-1,
			strPort, sizeof(strPort)-1 ,
			NI_NUMERICHOST|NI_NUMERICSERV ) !=0 ) {
			return false;
		}
		ip = strIp;
		port = (unsigned short)atoi(strPort);
		return true;
	}

	bool	Socket::recv( AsyncBuffer buf ) {
		  AsyncBufferS bufs;bufs.push_back(buf);
		  return recv(bufs);
	}

	bool Socket::send( AsyncBuffer buf )
	{
		  AsyncBufferS bufs;bufs.push_back(buf);
		  return send(bufs);
	}

	int Socket::sendDirect( AsyncBuffer buf )
	{
		AsyncBufferS bufs;bufs.push_back(buf);
		return sendDirect(bufs);
	}

	bool Socket::setReuseAddr( bool reuse ) {
		int reuse_value = reuse ? 1 : 0;
		return setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_value, sizeof(reuse_value)) == 0;

	}

	bool Socket::setSendBufSize( int size ) {
#ifdef ZQ_OS_LINUX
        return 0 == setsockopt( mSocket, SOL_SOCKET, SO_SNDBUFFORCE, (const char*)&size, sizeof(size));
#else
        return 0 == setsockopt( mSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
#endif
	}

	bool Socket::setRecvBufSize( int size ) {
#ifdef ZQ_OS_LINUX
        return 0 == setsockopt( mSocket, SOL_SOCKET, SO_RCVBUFFORCE, (const char*)&size, sizeof(size));
#else
        return 0 == setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
#endif
	}

}//namespace LibAsync
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312
