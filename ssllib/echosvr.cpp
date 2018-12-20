#include "stdafx.h"
#include "ssllib.h"
using namespace ZQ;

// 直接安全连接
int echosvr1_main(int argc , char* argv[])
{
	WSADATA wsad;
	sockaddr_in addr;
	char buf[1024];
	int len;
	WSAStartup(MAKEWORD(2, 0), &wsad);
	init_ssl_library();
	SSLContext ctx(VER_SSLv3);
	SSLSocket listensock(INVALID_SOCKET, &ctx),* ssock;
	ctx.loadCertificate("server.pem");
	ctx.loadPrivateKeyFile("server.pem", "xiao");
	listensock.socket();
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(ADDR_ANY);
	addr.sin_port = htons(9090);
	listensock.bind((sockaddr* )&addr, sizeof(addr));
	listensock.listen();
	while(true) {
		ssock = listensock.accept(NULL, 0);
		len = ssock->recv(buf, sizeof(buf));
		if (len > 0)
			len = ssock->send(buf, len);
		ssock->close();
		delete ssock;
	}
	
	return 0;
}

// 磋商升级
int echosvr_main(int argc , char* argv[])
{
	WSADATA wsad;
	sockaddr_in addr;
	char buf[1024];
	int len;
	WSAStartup(MAKEWORD(2, 0), &wsad);
	init_ssl_library();
	SSLContext ctx(VER_SSLv3);
	SSLSocket listensock(INVALID_SOCKET, &ctx),* ssock;
	ctx.loadCertificate("server.pem");
	ctx.loadPrivateKeyFile("server.pem", "xiao");
	ctx.loadRootCertificates("roots.pem");
	ctx.enableClientVerify(true);
	listensock.socket();
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(ADDR_ANY);
	addr.sin_port = htons(9090);
	listensock.bind((sockaddr* )&addr, sizeof(addr));
	listensock.listen();
	while(true) {
		ssock = listensock.accept(NULL, 0, false);
		ssock->setClientAuth(true);
		len = ssock->recv(buf, sizeof(buf) - 1);
		if (len > 0) {
			buf[len] = 0;
			if (stricmp(buf, "auth") == 0) {
				ssock->send("okay", 4);
				ssock->s_accept();
				len = ssock->recv(buf, sizeof(buf) - 1);
			}
			if (len > 0)
				len = ssock->send(buf, len);
		}
		ssock->close();
		delete ssock;
	}
	
	return 0;
}

