// ftpsserver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ssllib.h>
using namespace ZQ;
#include "ftpsserver.h"

SSLContext* ssl_ctx;
//////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////
SERVICE_STATUS_HANDLE service_status_handle;
SERVICE_STATUS service_status;
//////////////////////////////////////////////////////////////////////////

static VOID WINAPI ServiceHandler(DWORD dwControl)
{
	
	switch (dwControl) {
	case SERVICE_CONTROL_STOP:
		service_status.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(service_status_handle, &service_status);
		OutputDebugString("ServiceHandler(): SERVICE_CONTROL_STOP\n");
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		service_status.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(service_status_handle, &service_status);
		OutputDebugString("ServiceHandler(): SERVICE_CONTROL_SHUTDOWN\n");
		break;
	case SERVICE_CONTROL_PAUSE:
		service_status.dwCurrentState = SERVICE_PAUSED;
		SetServiceStatus(service_status_handle, &service_status);
		OutputDebugString("ServiceHandler(): SERVICE_CONTROL_PAUSE\n");
		break;
	case SERVICE_CONTROL_CONTINUE:
		service_status.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(service_status_handle, &service_status);
		OutputDebugString("ServiceHandler(): SERVICE_CONTROL_CONTINUE\n");
		break;
	case SERVICE_CONTROL_INTERROGATE:
		SetServiceStatus(service_status_handle, &service_status);
		OutputDebugString("ServiceHandler(): SERVICE_CONTROL_INTERROGATE\n");
		break;
   }
}

char* service_name = "SecureFtpd";

int ftpserver_main(int argc, char* argv[]);
void startup();

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

void WINAPI ServiceMain(DWORD argc, LPSTR* argv)
{
	service_status.dwCurrentState = SERVICE_RUNNING;

	service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | 
		SERVICE_INTERACTIVE_PROCESS;
	service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
		SERVICE_ACCEPT_PAUSE_CONTINUE;
	service_status.dwWin32ExitCode = 0;
	service_status.dwServiceSpecificExitCode = NO_ERROR;
	service_status.dwCheckPoint = 0;
	service_status.dwWaitHint = 0;
	service_status_handle = RegisterServiceCtrlHandler(service_name, ServiceHandler);
	if (service_status_handle == 0) {
		DbgPrint("main(): service_status_handle == 0 (err %x)\n", GetLastError());

		return;
	}
	
	DbgPrint("main(): service_status_handle is %d\n", service_status_handle);
	service_status.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(service_status_handle, &service_status);
	service_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(service_status_handle, &service_status);
	DbgPrint("main(): ok.\n");
	ftpserver_main(argc, argv);
}

int echosvr_main(int argc , char* argv[]);

int testmain(int argc, char* argv[])
{
	WSADATA wsad;
	int r;
	WSAStartup(MAKEWORD(2, 0), &wsad);
	init_ssl_library();

	ssl_ctx = new SSLContext(VER_SSLv2v3);
	r = ssl_ctx->loadCertificateChain("server.pem");
	r = ssl_ctx->loadPrivateKeyFile("server.pem", "xiao");
	r = ssl_ctx->loadDHParams("dh1024.pem");

	SOCKET listsock, sock, remosock;
	sockaddr_in addr;
	fd_set fdsetr;
	char buf[4096];
	int len;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(ADDR_ANY);
	addr.sin_port = htons(2323);
	listsock = socket(AF_INET, SOCK_STREAM, 0);
	bind(listsock, (sockaddr* )&addr, sizeof(addr));
	listen(listsock, 3);
	sock = accept(listsock, NULL, NULL);
	remosock = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("202.120.225.9");
	addr.sin_port = htons(23);
	connect(remosock, (sockaddr* )&addr, sizeof(addr));
	while(true) {
		FD_ZERO(&fdsetr);
		FD_SET(remosock, &fdsetr);
		FD_SET(sock, &fdsetr);
		select(FD_SETSIZE, &fdsetr, NULL, NULL, NULL);
		if (FD_ISSET(remosock, &fdsetr)) {
			printf("FD_ISSET(remosock, &fdsetr)\n");
			//len = recv(remosock, buf, sizeof(buf), 0);
			//if (len > 0)
			//	send(sock, buf, len, 0);
		}

		if (FD_ISSET(sock, &fdsetr)) {
			printf("FD_ISSET(sock, &fdsetr)\n");
			//len = recv(sock, buf, sizeof(buf), 0);
			//if (len > 0)
			//	send(remosock, buf, len, 0);
		}
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	// return testmain(argc, argv);
	// return echosvr_main(argc, argv);

	SERVICE_TABLE_ENTRY svcentry[2];
	if (argc > 1 && stricmp(argv[1], "/service") == 0) {
		svcentry[0].lpServiceName = service_name;
		svcentry[0].lpServiceProc = ServiceMain;
		svcentry[1].lpServiceName = NULL;
		svcentry[1].lpServiceProc = NULL;     
		StartServiceCtrlDispatcher(svcentry);
		return 0;
	} else
		return ftpserver_main(argc, argv);
}

void __cdecl session_thread(void* p)
{
	ftpsvr_channel* channel;
	ftp_session* session = (ftp_session* )p;
	session->processRemoteCmd();
	channel = session->getCmdChannel();
	delete channel;
	delete session;
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

int ftpserver_main(int argc, char* argv[])
{
	sockaddr_in addr, addrClient;
	int len = sizeof(sockaddr);
	SOCKET s, sockClient;
	ftpsvr_tcp_channel* channel;
	ftp_session* session;

	int r;
	WSADATA wsad;
	
	WSAStartup(MAKEWORD(2, 0), &wsad);
	init_ssl_library();
	startup();

	int n = OBJ_txt2nid(LN_commonName);
	ssl_ctx = new SSLContext(VER_SSLv2v3);
	r = ssl_ctx->loadCertificateChain("server.pem");
	r = ssl_ctx->loadPrivateKeyFile("server.pem", "xiao");
	r = ssl_ctx->loadDHParams("dh1024.pem");

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ADDR_ANY;
	addr.sin_port = htons(21);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	bind(s, (sockaddr* )&addr, sizeof(addr));
	listen(s, SOMAXCONN);
	while (true) {
		sockClient = accept(s, (sockaddr* )&addrClient, &len);
		if (service_status.dwCurrentState == SERVICE_PAUSED) {
			if (sockClient != INVALID_SOCKET)
				closesocket(sockClient);
			continue;
		}
		
		if (sockClient != INVALID_SOCKET) {
			printf("(D): %s:%d connected.\n", 
				inet_ntoa(addrClient.sin_addr), 
				addrClient.sin_port);
			session = new ftp_session;
			channel = new ftpsvr_tcp_channel;
			channel->attach(sockClient);
			session->createSession(channel);
			_beginthread(session_thread, 0, session);
		} else
			return -1;
	}
	return 0;
}

int ftp_session::onNOOP(const char* cmdline)
{
	return sendResponse(MSG_200);
}

int ftp_session::onAUTH(const char* cmdline)
{
	ftpsvr_ssl_channel* channel;
	ftpsvr_tcp_channel* tcpchannel;
	if (_cmdargc != 2 || _secure)
		return sendResponse(MSG_533);

	if (stricmp(_cmdargv[1], "SSL") == 0) {
		sendResponse(MSG_234);
		tcpchannel = (ftpsvr_tcp_channel* )_channel;
		channel = new ftpsvr_ssl_channel(*ssl_ctx);
		channel->accept(tcpchannel->detach());
		delete tcpchannel;
		_channel = channel;
		_secure = true;
		return 1;
	} else {
		return sendResponse(MSG_533);
	}
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
