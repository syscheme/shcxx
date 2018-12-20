#include "stdafx.h"
#include "ServiceFrame.h"
#include <process.h>
#include <io.h>

using namespace ZQ;

#define DMODE_NONE					0
#define DMODE_PORT					1
#define DMODE_PASV					2

#define CRLF						"\r\n"

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
	virtual int upgreadSecurity() = 0;
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

	int processRequest(char* data, int size)
	{
		char buf[MAX_FTP_BUFF];
		int cur, next;
		int i = 0;
		int len;
		int ptr = 0;
		
		if (size <= 0)
			return 0;

		// len = _channel->recv(buf, _bufFree - 1);
		while (ptr < size) {
			len = size - ptr >= _bufFree - 1 ? _bufFree - 1 : size - ptr;
			memcpy(buf, data + ptr, len);
			ptr += len;

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
		}
		return 1;
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


const char* MSG_110 = "110 Restart marker reply.";
const char* MSG_200 = "200 Command successful.";
const char* MSG_220 = "220 Service ready for new user.";
const char* MSG_331 = "331 User name okay, need password.";
const char* MSG_226 = "226 Closing data connection.";
const char* MSG_150 = "150 File status okay; about to open data connection.";
const char* MSG_227 = "227 Entering Passive Mode (%s).";
const char* MSG_230 = "230 User logged in, proceed.";
const char* MSG_530 = "530 Not logged in.";
const char* MSG_202 = "202 Command not implemented, superfluous at this site.";
const char* MSG_502 = "502 Command not implemented.";
const char* MSG_234 = "234 AUTH command accepted. Setting up SSL/TLS connection.";
const char* MSG_533 = "533 The control connection is not protected with TLS.";
const char* MSG_221 = "221 Service closing control connection.";
const char* MSG_257 = "257 \"%s\" is current directory.";
const char* MSG_250 = "250 Path changed to \"%s\".";
const char* MSG_550 = "550 Requested action not taken.";
//////////////////////////////////////////////////////////////////////////

void __cdecl ftp_session::dchannel_thread(void* p)
{
	ftp_session* session = (ftp_session* )p;
	if (session->_dmode == DMODE_PASV)
		session->_dchannel->accept();
	
	WaitForSingleObject(session->_dataEvent, INFINITE);
	if (session->_dmode == DMODE_PORT)
		session->_dchannel->connect();
	session->sendData();
	session->sendResponse(MSG_226);
	session->_channel->destroyDataChannel(session->_dchannel);
	session->_dchannel = NULL;
}

int ftp_session::sendResponse(const char* response)
{
	char buf[128];
	sprintf(buf, "%s" CRLF, response);
	if (_debug)
		printf("(R): %s\n", response);
	
	return _channel->send(buf, strlen(buf));
}

int ftp_session::parseComline(char* cmdline)
{
	char* c = cmdline;
	bool blank;
	if (cmdline == NULL || strlen(cmdline) <= 0)
		return 0;

	while(*c == ' ' || *c == '\t')
		c++;
	
	_cmdargc = 0;

	blank = true;
	while(*c && (*c != '\n' && *c != '\r')) {
		if (*c == ' ' || *c == '\t') {
			if (!blank) {
				*c = 0;		
				blank = true;
			}
		} else {
			if (blank) {
				_cmdargc ++;
				blank = false;
				_cmdargv[_cmdargc - 1] = c;
			}
		}
		c++;
	}
	
	*c = 0;

	return 1;
}

int ftp_session::doCommand(const char* cmdline)
{
	struct rftpcmd* rftpcmds = get_rftpcmds();
	strcpy(_cmdline, cmdline);
	if (_debug)
		printf("(C): %s\n", cmdline);

	parseComline(_cmdline);
	while(rftpcmds->cmd != NULL) {
		if (stricmp(rftpcmds->cmd, _cmdargv[0]) == 0)
			return (this->*rftpcmds->handler)(cmdline);
		
		rftpcmds ++;
	}

	return sendResponse(MSG_502);
}

int ftp_session::onNOOP(const char* cmdline)
{
	return sendResponse(MSG_200);
}

int ftp_session::onAUTH(const char* cmdline)
{
	if (_cmdargc != 2 || _secure)
		return sendResponse(MSG_533);

	if (stricmp(_cmdargv[1], "SSL") == 0) {
		sendResponse(MSG_234);
		_channel->upgreadSecurity();
		_secure = true;
		return 1;
	} else {
		return sendResponse(MSG_533);
	}

	return 0;
}

int ftp_session::onUSER(const char* cmdline)
{
	_status = FTP_STATUS_USER;
	return sendResponse(MSG_331);
}

int ftp_session::onPASS(const char* cmdline)
{
	_status = FTP_STATUS_PASS;
	return sendResponse(MSG_230);
}

int ftp_session::onCWD(const char* cmdline)
{
	char buf[512];
	char* c;

	if (_cmdargc != 2)
		return 0;

	if (_cmdargv[1][0] == '/') {
		strcpy(_curPath, _cmdargv[1]);

	} else if (strcmp(_cmdargv[1], "..") == 0) {
		if (strcmp(_curPath, "/") != 0) {
			c = _curPath + strlen(_curPath);
			while(c != _curPath) {
				if (*c == '/')
					break;
				else
					*c = 0;
				c--;
			}

			if (strlen(_curPath) > 1)
				*c = 0;
		}
	} else {
		strcpy(buf, _curPath);
		if (buf[strlen(buf) - 1] != '/')
			strcat(buf, "/");
		strcat(buf, _cmdargv[1]);
		if (_access(buf, 0) == -1) {
			return sendResponse(MSG_550);
		}

		strcpy(_curPath, buf);
	}

	sprintf(buf, MSG_250, _curPath);
	sendResponse(buf);
	
	return 1;
}

int ftp_session::onPWD(const char* cmdline)
{
	char buf[256];
	sprintf(buf, MSG_257, _curPath);
	sendResponse(buf);
	return 1;
}

int get_data_hostport(char* respond, char* ipaddr, unsigned short* port)
{
	char* c = respond, *p;
	unsigned short part1, part2;
	char buf[16];
	int i = 0;

	if (respond == NULL)
		return 0;

	while(*c) {
		if (*c == ',') {
			i++;
			if (i == 4)
				break;
			*ipaddr = '.';
		} else 
			*ipaddr = *c;
		c++;
		ipaddr ++;
	}
	*ipaddr = 0;
	
	i = 0;
	while(*c) {
		if (*c == ',') {
			i ++;
			if (i == 1)
				p = c + 1;
			else if (i == 2) {
				memset(buf, 0, sizeof(buf));
				strncpy(buf, p, (unsigned long )c - (unsigned long )p);
				part1 = atoi(buf);
				p = c + 1;
				strcpy(buf, p);
				part2 = atoi(buf);
				*port = part1 * 256 + part2;
				return *port;
			}
		}
		c++;
	}
	return 0;
}

int ftp_session::onPORT(const char* cmdline)
{
	char host[20];
	u_short port;
	_dmode = DMODE_PORT;
	get_data_hostport(_cmdargv[1], host, &port);
	_dchannel = _channel->createDataChannel(inet_addr(host), port, _dmode);
	_beginthread(dchannel_thread, 0, this);
	// Sleep(100);
	return sendResponse(MSG_200);
}

int ftp_session::onLIST(const char* cmdline)
{
	SetEvent(_dataEvent);
	// WaitForSingleObject(_dataEvent, INFINITE);
	return sendResponse(MSG_150);
}

int ftp_session::onPASV(const char* cmdline)
{
	char buf1[256];
	char buf2[256];
	_dmode = DMODE_PASV;
	hostent* ht;
	u_long addr;

	_dport = 10000;
	do {
		gethostname(buf1, sizeof(buf1));
		ht = gethostbyname(buf1);
		memcpy(&addr, ht->h_addr_list[0], ht->h_length);
		
		_dchannel = _channel->createDataChannel(addr, _dport, _dmode);
		_dport ++;
	}while (_dchannel == NULL);
	_dchannel->getHostPort(buf1);
	sprintf(buf2, MSG_227, buf1);
	_beginthread(dchannel_thread, 0, this);
	Sleep(100);
	sendResponse(buf2);
	return 1;		
}

int ftp_session::onTYPE(const char* cmdline)
{
	sendResponse(MSG_200);
	return 1;
}

int ftp_session::onRETR(const char* cmdline)
{
	SetEvent(_dataEvent);
	sendResponse(MSG_150);
	return 1;
}

int ftp_session::onQUIT(const char* cmdline)
{
	sendResponse(MSG_221);
	return 0;
}

int ftp_session::sendData()
{
	struct rftpcmd* rftpcmds = get_rftpcmds();

	if (stricmp(_cmdargv[0], "LIST") == 0){
		return sendFileList();
	} else if (_cmdargc == 2 && stricmp(_cmdargv[0], "RETR") == 0) {
		return sendFile(_cmdargv[1]);
	}

	return 0;
}

int ftp_session::sendFileList()
{
	char tmp[256];
	char line[1024];
	char buf[1024 * 4];
	WIN32_FIND_DATA fdata;
	char filter[MAX_PATH];
	SYSTEMTIME st;
	if (_cmdargc == 2) {
		getLocalPath(_cmdargv[1], filter);	
	} else {
		getLocalPath(_curPath, buf);
		sprintf(filter, "%s\\*.*", buf);
	}
	BOOL cont = true;
	HANDLE handle = FindFirstFile(filter, &fdata);
	if (handle != INVALID_HANDLE_VALUE) {
		buf[0] = 0;
		while (cont){
			if (strcmp(fdata.cFileName, ".") == 0 || 
				strcmp(fdata.cFileName, "..") == 0)
				goto findnext;
			
			line[0] = 0;
			if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				strcat(line, "drwx------");
			else
				strcat(line, "-rwx------");
			strcat(line, " 1 user group ");
			sprintf(tmp, " %8d ", fdata.nFileSizeLow);
			strcat(line, tmp);
			FileTimeToSystemTime(&fdata.ftLastWriteTime, &st);
			GetDateFormat(
				MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 
				0, &st, "MMM dd", tmp, sizeof(tmp));
			strcat(line, tmp);
			strcat(line, " ");
			GetTimeFormat(
				MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 
				TIME_NOSECONDS | TIME_NOTIMEMARKER, &st, "HH:mm  ", tmp, sizeof(tmp));
			strcat(line, tmp);
			strcat(line, fdata.cFileName);
			if (strlen(buf) + strlen(line) >= sizeof(buf)) {
				_dchannel->send(buf, strlen(buf));
				buf[0] = 0;
			}
			strcat(buf, line);
			strcat(buf, CRLF);
findnext:
			cont = FindNextFile(handle, &fdata);
		}
		
		_dchannel->send(buf, strlen(buf));
	}

	return 1;
}

int ftp_session::sendFile(char* filename)
{
	char pathname[MAX_PATH];
	char buf[1024 * 4];
	bool cont = true;
	int len;
	getLocalPath(_curPath, buf);
	sprintf(pathname, "%s\\%s", buf, filename);
	FILE* fp = fopen(pathname, "rb");
	if (fp) {
		while (cont) {
			len = fread(buf, 1, sizeof(buf), fp);
			_dchannel->send(buf, len);
			if (feof(fp))
				break;
		}
	}
	return 1;
}

int ftp_session::getLocalPath(const char* path, char* locpath)
{
	const char* c = path;
	char* p;
	if (strcmp(path, "/") == 0) {
		strcpy(locpath, _rootPath);
		return 1;
	}

	strcpy(locpath, _rootPath);
	if (path[0] != '/')
		strcat(locpath, "\\");
	p = locpath + strlen(locpath);
	while(*c) {
		if (*c == '/')
			*p = '\\';
		else
			*p = *c;
		c++;
		p++;
	}
	*p = 0;
	
	return 1;
}

//////////////////////////////////////////////////////////////////////////
#define MAIN_CHANNEL		0
#define SECOND_CHANNEL		1

class ftpsvr_channelImpl : public ftpsvr_channel{
public:
	ftpsvr_channelImpl(IMainConn* mainConn, int type) :
	  _mainConn(mainConn)
	{
		  if (type == MAIN_CHANNEL)
			  _conn = mainConn;
		  else
			_conn = NULL;
		  
		  _channelType = type;
	}
	
	virtual ftpsvr_channel* createDataChannel(u_long host , 
		u_short port, u_char mode)
	{
		ftpsvr_channelImpl* channel = new ftpsvr_channelImpl(_mainConn, 
			SECOND_CHANNEL);
		channel->_host = host;
		channel->_port = port;
		channel->_connMode = mode;
		return channel;
	}

	virtual void destroyDataChannel(ftpsvr_channel* channel)
	{
		if (channel) {
			channel->close();
			delete channel;
		}
	}

	virtual int connect()
	{
		if (_channelType == MAIN_CHANNEL) {
			SF_ASSERT(false);
			return SOCKET_ERROR;
		}

		ConnID connID;
		sockaddr_in* addr;
		connID.type = CONN_TYPE_SOCKET_TCP;
		connID.addrlen = sizeof(*addr);
		addr = (sockaddr_in* )&connID.caddr;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = _host;
		addr->sin_port = htons(_port);
		_conn = _mainConn->createConn(&connID, CONN_MODE_ACTIVE, true, 0);
		if (_conn != NULL)
			return 0;
		else
			return SOCKET_ERROR;
	}

	virtual int accept()
	{
		if (_channelType == MAIN_CHANNEL) {
			SF_ASSERT(false);
			return INVALID_SOCKET;
		}

		ConnID connID;
		sockaddr_in* addr;
		connID.type = CONN_TYPE_SOCKET_TCP;
		connID.addrlen = sizeof(*addr);
		addr = (sockaddr_in* )&connID.caddr;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = htonl(ADDR_ANY);
		addr->sin_port = htons(_port);
		_conn = _mainConn->createConn(&connID, CONN_MODE_PASSIVE, true, 0);
		if (_conn != NULL)
			return 1;
		else
			return INVALID_SOCKET;
	}
	
	virtual int send(const char* buf , int num)
	{
		return _conn->send(buf, num);
	}

	virtual int recv(char* buf, int num)
	{
		return _conn->recv(buf, num);
	}

	virtual void close()
	{
		_conn->close();
	}

	virtual void getHostPort(char* buf)
	{
		u_long host = _host;
		u_short port = htons(_port);
		sprintf(buf, "%d,%d,%d,%d,%d,%d", 
			LOBYTE(LOWORD(host)), HIBYTE(LOWORD(host)),
			LOBYTE(HIWORD(host)), HIBYTE(HIWORD(host)),  
			LOBYTE(port), HIBYTE(port));
	}

	virtual int upgreadSecurity()
	{
		if (_channelType != MAIN_CHANNEL) {
			assert(false);
			
			return SOCKET_ERROR;
		}

		_mainConn->upgradeSecurity(0);
		return 0;
	}

protected:
	int					_channelType;
	
	IConnection*		_conn;
	IMainConn*			_mainConn;

	u_char				_connMode;
	u_long				_host;
	u_short				_port;
	bool				_secure;
	bool				_isMain;
};


class FtpDialogue: public IDialogue, 
	public ftp_session {

public:
	FtpDialogue()
	{
		 
	}

	virtual void onConnected(IN IMainConn* conn)
	{
		printf("(D): onConnected\n");
		_channel = new ftpsvr_channelImpl(conn, MAIN_CHANNEL);
		createSession(_channel);
		welcome();
	}

	virtual void onRequest(IN IMainConn* conn, IN const void* buf, 
		IN int size)
	{
		printf("(D): onRequest\n");
		if (processRequest((char* )buf, size) == 0)
			_channel->close();
	}

	virtual void onIdlenessTimeout(IN IMainConn* conn)
	{
		printf("(D): Timeout\n");
		conn->close();
	}

	virtual void onDialogueDestroyed()
	{
		printf("(D): onDialogueDestroyed\n");
	}

protected:
	ftpsvr_channelImpl*		_channel;
};

class FtpDialogueCreator: public IDialogueCreator {
public:
	virtual IDialogue* createDialogue()
	{
		return new FtpDialogue;
	}

	virtual void releaseDialogue(IN IDialogue* session)
	{
		delete session;
	}
};

#define FTP_PORT			21

//////////////////////////////////////////////////////////////////////////

int DbgPrint(char* format, ...)
{
	char buf[1025];
	va_list vlist;
	int ret;
	va_start(vlist, format);
	ret = vsprintf(buf, format, vlist);
	OutputDebugString(buf);
	return ret;
}

void startup()
{
	printf("---------------------------------------------\n");
	printf("|         SeaChina Secure FTP Server        |\n");
	printf("|                         SeaChange China.  |\n");
	printf("|                                           |\n");
	printf("|                                           |\n");
	printf("|                          Written by Cary. |\n");
	printf("---------------------------------------------\n");
	printf("Server is working...\n");

}

int main(int argc, char* argv[])
{
	ConnID connID;
	sockaddr_in* addr;
	ServiceFrm frm;
	FtpDialogueCreator dlgCtor;
	frm.setDialogureCreator(&dlgCtor);
	connID.type = CONN_TYPE_SOCKET_TCP;
	connID.addrlen = sizeof(*addr);
	addr = (sockaddr_in* )&connID.caddr;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(ADDR_ANY);
	addr->sin_port = htons(FTP_PORT);
	frm.setServiceThread(false);
	frm.init(NULL);
	startup();
	frm.begin(&connID);
	return 0;
}

