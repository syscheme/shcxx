#ifndef _FTPS_SERVER_H_
#define _FTPS_SERVER_H_
#include <process.h>

#define DMODE_NONE					0
#define DMODE_PORT					1
#define DMODE_PASV					2

#define CRLF	"\r\n"

class ftpsvr_channel{
public:
	virtual ftpsvr_channel* createDataChannel(u_long host , u_short port, u_char mode) = 0;
	virtual int connect() = 0;
	virtual int accept() = 0;
	virtual void destroyDataChannel(ftpsvr_channel* channel) = 0;
	virtual int send(const char* buf , int num) = 0;
	virtual int recv(char* buf, int num) = 0;
	virtual void close() = 0;
	virtual void getHostPort(char* buf) = 0;
};

class ftpsvr_tcp_channel: public ftpsvr_channel {
public:
	ftpsvr_tcp_channel()
	{
		_sock = INVALID_SOCKET;
	}

	void attach(SOCKET sock)
	{
		_sock = sock;
	}

	SOCKET detach()
	{
		SOCKET s = _sock;
		_sock = INVALID_SOCKET;
		return s;
	}
	
	virtual ftpsvr_channel* createDataChannel(u_long host , u_short port, u_char mode)
	{
		ftpsvr_tcp_channel* channel = new ftpsvr_tcp_channel;
		channel->_host = host;
		channel->_port = port;
		if (mode == DMODE_PASV)
			channel->listen(host, port);
		return channel;
	}

	virtual void destroyDataChannel(ftpsvr_channel* channel)
	{
		if (channel) {
			channel->close();
			delete channel;
		}
	}

	virtual int accept()
	{
		SOCKET sock;
		sockaddr_in clientaddr;
		int len = sizeof(clientaddr);
		sock = ::accept(_sock, (sockaddr* )&clientaddr, &len);
		closesocket(_sock);
		if (sock != INVALID_SOCKET)
			printf("DEBUG(D) %s:%d connected.\n", 
				inet_ntoa(clientaddr.sin_addr), 
				clientaddr.sin_port);

		_sock = sock;
		return _sock;
	}

	virtual int connect()
	{
		sockaddr_in addr;
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = _host;
		addr.sin_port = htons(_port);
		return ::connect(_sock, (sockaddr* )&addr, sizeof(addr));
	}

	virtual int send(const char* buf , int num)
	{
		return ::send(_sock, buf, num , 0);

	}

	virtual int recv(char* buf, int num)
	{
		return ::recv(_sock, buf, num , 0);
	}

	virtual void close()
	{
		if (_sock != INVALID_SOCKET)
			closesocket(_sock);		
	}

	virtual void getHostPort(char* buf)
	{
		sockaddr_in addr;
		int len = sizeof(addr);
		getsockname(_sock, (sockaddr* )&addr, &len);
		u_long host = addr.sin_addr.s_addr;
		u_short port = addr.sin_port;

		sprintf(buf, "%d,%d,%d,%d,%d,%d", 
			LOBYTE(LOWORD(host)), HIBYTE(LOWORD(host)),
			LOBYTE(HIWORD(host)), HIBYTE(HIWORD(host)),  
			LOBYTE(port), HIBYTE(port));
	}

protected:

	int listen(u_long host, u_short port)
	{
		sockaddr_in addr;

		if (_sock != INVALID_SOCKET)
			return -1;
		
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = host;
		addr.sin_port = htons(port);
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (bind(_sock, (sockaddr* )&addr, sizeof(addr)) == SOCKET_ERROR)
			return -1;
		return ::listen(_sock, SOMAXCONN);
	}

protected:
	SOCKET		_sock;
	u_long		_host;
	u_short		_port;
};

class ftpsvr_ssl_channel: public ftpsvr_channel{
public:
	
	ftpsvr_ssl_channel(SSLContext& ctx): 
	  _ctx(ctx), _ssock(INVALID_SOCKET, &ctx)
	{
		  _listensock = INVALID_SOCKET;
	}

	virtual ftpsvr_channel* createDataChannel(u_long host , u_short port, u_char mode)
	{
		in_addr addr;;
		ftpsvr_ssl_channel* channel = new ftpsvr_ssl_channel(_ctx);
		channel->_host = host;
		channel->_port = port;
		addr.S_un.S_addr = host;
		// channel->_ssock.setHostPort(inet_ntoa(addr), port);
		if (mode == DMODE_PASV)
			channel->listen();
			
		return channel;
	}

	virtual int accept()
	{
		SOCKET sock;
		/*
		SSLSocket* ssock = _ssock.accept();
		SOCKET s = _ssock.detach();
		closesocket(s);
		_ssock.attach(ssock->detach());
		delete ssock;
		return 1;
		(*/
		sock = ::accept(_listensock, NULL, NULL);
		closesocket(_listensock);
		_listensock = INVALID_SOCKET;
		return _ssock.s_accept(sock);
	}

	virtual int connect()
	{
		return _ssock.s_connect();
	}

	virtual void destroyDataChannel(ftpsvr_channel* channel)
	{
		if (channel) {
			channel->close();
			delete channel;
		}
	}

	virtual int send(const char* buf , int num)
	{
		return _ssock.send(buf, num);
	}

	virtual int recv(char* buf, int num)
	{
		return _ssock.recv(buf, num);
	}

	virtual void close()
	{
		_ssock.close();
	}

	virtual void getHostPort(char* buf)
	{
		sockaddr_in addr;
		int len = sizeof(addr);
		getsockname(_listensock, (sockaddr* )&addr, &len);
		u_long host = addr.sin_addr.s_addr;
		u_short port = addr.sin_port;

		sprintf(buf, "%d,%d,%d,%d,%d,%d", 
			LOBYTE(LOWORD(host)), HIBYTE(LOWORD(host)),
			LOBYTE(HIWORD(host)), HIBYTE(HIWORD(host)),  
			LOBYTE(port), HIBYTE(port));

	}

	virtual void attach(SOCKET s)
	{
		_ssock.attach(s);
	}

	int accept(SOCKET s)
	{
		return _ssock.s_accept(s);
	}


protected:
	int listen()
	{
		sockaddr_in addr;
		_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = _host;
		addr.sin_port = htons(_port);
		bind(_listensock, (sockaddr* )&addr, sizeof(addr));
		return ::listen(_listensock, SOMAXCONN);
	}
protected:
	SOCKET			_listensock;
	u_long			_host;
	u_short			_port;
	SSLSocket		_ssock;
	SSLContext&		_ctx;
};

#define MAX_FTP_BUFF				2048
#define MAX_CMD_LEN					260
#define MAX_CMDARGS_COUNT			5
#define MAX_FTPCMD_ARGS				255
#define FTP_STATUS_INVALID			-1
#define FTP_STATUS_START			0
#define FTP_STATUS_WELCOME			1
#define FTP_STATUS_AUTH				2
#define FTP_STATUS_USER				3
#define FTP_STATUS_PASS				4
#define FTP_STATUS_LOGIN			5
#define FTP_STATUS_LOGOUT			6

#define FTP_CMD_NONE				1
#define FTP_CMD_LIST				1
#define FTP_CMD_RETR				0

#define DEFINE_FTP_RCMD(cmdname)	{#cmdname, ftp_session::on##cmdname}

class ftp_session {
public:
	ftp_session()
	{
		_status = FTP_STATUS_INVALID;
		_channel = NULL;
		_bufStart = _bufEnd = -1;
		_bufFree = MAX_FTP_BUFF;
		_dport = 10000;
		_dmode = DMODE_NONE;
		strcpy(_rootPath, "D:\\ftproot");
		strcpy(_curPath, "/");
		_debug = true;
		_secure = false;
	}

	int createSession(ftpsvr_channel* channel)
	{
		if (channel == NULL)
			return 0;

		_channel = channel;
		_dataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		_status = FTP_STATUS_START;
		return 1;
	}

	int processRemoteCmd()
	{
		int result;
		welcome();
		while(true) {
			result = processCommand();
			if (result <= 0)
				break;
		}
		
		return result;
	}

	ftpsvr_channel* getCmdChannel()
	{
		return _channel;
	}
	
protected:

	struct rftpcmd {
		char* cmd;
		int (ftp_session::*handler)(const char* cmdline);
	};
	
	rftpcmd* get_rftpcmds() {
		static struct rftpcmd _rftpcmds[] = {
			DEFINE_FTP_RCMD(NOOP), 
			DEFINE_FTP_RCMD(AUTH), 
			DEFINE_FTP_RCMD(USER), 
			DEFINE_FTP_RCMD(PASS), 
			DEFINE_FTP_RCMD(CWD), 
			DEFINE_FTP_RCMD(PWD), 
			DEFINE_FTP_RCMD(PORT), 
			DEFINE_FTP_RCMD(LIST), 
			DEFINE_FTP_RCMD(PASV), 
			DEFINE_FTP_RCMD(TYPE), 
			DEFINE_FTP_RCMD(RETR),
			DEFINE_FTP_RCMD(QUIT),
			{0, 0}
		};

		return _rftpcmds;
	}

	virtual void welcome()
	{
		char msg[] = "220 SeaChange China. FTP Server" CRLF
			"220 Version 1.0"  CRLF
			"220 Writte by Cary"  CRLF;
		if (_status != FTP_STATUS_START) {
			assert(false);
			return;
		}

		_channel->send(msg, strlen(msg));
		_status = FTP_STATUS_WELCOME;
	}
	
	int sendResponse(const char* response);
	int doCommand(const char* cmdline);

	int processCommand()
	{
		char buf[MAX_FTP_BUFF];
		int cur, next;
		int i = 0;
		int len;
		len = _channel->recv(buf, _bufFree - 1);
		if (len > 0) {

			buf[len] = 0;
			appendBuffer(buf, len);
			
			buf[0] = 0;
			cur = _bufStart;
			do {
				next = (cur + 1) % sizeof(_buff);
				if (_buff[cur] == '\r' && _buff[next] == '\n') {
					if (i != 0) {
						buf[i] = 0;
						if (doCommand(buf) == 0)
							return 0;
					}
					_bufFree += i + 2;
					_bufStart = (next + 1) % sizeof(_buff);
					if (_bufStart == _bufEnd)
						_bufStart = _bufEnd = -1;
				
					break;
				}
				buf[i] = _buff[cur];

				cur = next;
				i ++;
				
			}while(cur != _bufEnd);

			return 1;

		} else 
			return 0;
	}

	int onNOOP(const char* cmdline);
	int onAUTH(const char* cmdline);
	int onUSER(const char* cmdline);
	int onPASS(const char* cmdline);
	int onCWD(const char* cmdline);
	int onPWD(const char* cmdline);
	int onPORT(const char* cmdline);
	int onLIST(const char* cmdline);
	int onPASV(const char* cmdline);
	int onTYPE(const char* cmdline);
	int onRETR(const char* cmdline);
	int onQUIT(const char* cmdline);

protected:
	static void __cdecl dchannel_thread(void* p);

	int parseComline(char* cmdline);

	int appendBuffer(char* buf, int len)
	{
		int n;
		if (_bufFree < len)
			return -1;
		if (_bufEnd == -1) {
			memcpy(_buff, buf, len);
			_bufStart = 0;
			_bufEnd = len;
		} else if (_bufEnd + len < sizeof(_buff)) {
			memcpy(&_buff[_bufEnd], buf, len);
			_bufEnd += len;
		} else {
			n = sizeof(_buff) - _bufEnd;
			memcpy(_buff + _bufEnd, buf, n);
			memcpy(_buff, buf + n, len - n);
			_bufEnd = len - n;
		}
		return _bufFree -= len;
	}
	
	int sendData();
	int sendFileList();
	int sendFile(char* pathname);

	int getLocalPath(const char* path, char* locpath);

protected:
	sockaddr_in		_remoteAddr;
	ftpsvr_channel*	_channel;
	ftpsvr_channel*	_dchannel;

	int				_status;

	int				_cmdargc;
	char			_cmdline[MAX_CMD_LEN];
	char*			_cmdargv[MAX_CMDARGS_COUNT];

	char			_buff[MAX_FTP_BUFF];
	int				_bufStart, _bufEnd;
	int				_bufFree;

	u_char			_dmode;
	//u_long			_dhost;
	u_short			_dport;
	
	int				_dthread;

	char			_rootPath[MAX_PATH];
	char			_curPath[MAX_PATH];

	HANDLE			_dataEvent;
	bool			_debug;
	bool			_secure;
};

#endif // #ifndef _FTPS_SERVER_H_
