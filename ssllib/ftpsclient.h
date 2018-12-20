#ifndef _FTPS_CLIENT_H_
#define _FTPS_CLIENT_H_

#define CRLF	"\r\n"
u_long gethostaddr(const char* hostname);

class ftp_channel {
public:
	virtual ftp_channel* createDataChannel(char* host, u_short port) = 0;
	virtual void destroyDataChannel(ftp_channel* channel) = 0;
	virtual int send(char* buf , int num) = 0;
	virtual int recv(char* buf, int num) = 0;
	virtual void close() = 0;
};

class ftp_tcp_channel : public ftp_channel {
public:
	ftp_tcp_channel()
	{
		_sock = INVALID_SOCKET;

	}
	
	ftp_tcp_channel(SOCKET s)
	{
		_sock = s;
	}
	
	void attach(SOCKET s)
	{
		assert(_sock == INVALID_SOCKET);
		_sock = s;
	}

	SOCKET detach()
	{
		SOCKET s = _sock;
		_sock = 0;
		return s;
	}

	int connect(char* host, u_short port)
	{
		sockaddr_in addr;
		_sock = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP);
		addr.sin_addr.s_addr = gethostaddr(host);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);				
		return ::connect(_sock, (sockaddr* )&addr, sizeof(addr));
	}

	~ftp_tcp_channel()
	{
		close();
	}

	virtual void close()
	{
		if (_sock != INVALID_SOCKET) {
			closesocket(_sock);
			_sock = INVALID_SOCKET;
		}
	}

	virtual ftp_channel* createDataChannel(char* host, u_short port)
	{
		ftp_tcp_channel* channel = new ftp_tcp_channel;
		if (channel->connect(host, port) < 0) {
			delete channel;
			return NULL;
		}
		return channel;
	}

	virtual void destroyDataChannel(ftp_channel* channel)
	{
		if (channel)
			delete channel;
	}

	virtual int send(char* buf , int num)
	{
		return ::send(_sock, buf, num, 0);
	}

	virtual int recv(char* buf, int num)
	{
		return ::recv(_sock, buf, num, 0);

	}

protected:
	SOCKET _sock;
};

#endif // #ifndef _FTPS_CLIENT_H_
