#include "stdafx.h"
#include "ssllib.h"
#include "ftpsclient.h"

using namespace ZQ;
#define BUFFER_SIZE		1024

extern void usage();
extern int do_ftp_cmd(ftp_channel* channel, char* cmd, char* outbuf, int outlen);
extern int get_data_hostport(char* respond, char* ipaddr, unsigned short* port);

class ftp_sslsocket_channel : public ftp_channel {

public:
	ftp_sslsocket_channel(SSLContext& ctx) :
	  _ctx(ctx), _sslsock(INVALID_SOCKET, &ctx)
	{

	}
	
	int connect(SOCKET s)
	{
		_sslsock.attach(s);
		return _sslsock.s_connect();
	}

	int connect(char* host, u_short port)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = gethostaddr(host);
		addr.sin_port = htons(port);
		_sslsock.socket();
		return _sslsock.connect((sockaddr* )&addr, sizeof(addr));
	}

	virtual void close()
	{
		_sslsock.close();
	}

	virtual ftp_channel* createDataChannel(char* host, u_short port)
	{
		ftp_sslsocket_channel* channel = new ftp_sslsocket_channel(_ctx);
		channel->connect(host, port);
		return channel;
	}

	virtual void destroyDataChannel(ftp_channel* channel)
	{
		delete channel;
	}

	virtual int send(char* buf , int num)
	{
		return _sslsock.send(buf, num);
	}

	virtual int recv(char* buf, int num)
	{
		return _sslsock.recv(buf, num);
	}

protected:
	SSLClientSocket _sslsock;
	SSLContext&		_ctx;
};

extern SSLContext* sslctx;
int ftp_main(ftp_channel* channel);

int cpp_main(int argc, char* argv[])
{
	SSL* ssl = NULL;
	BIO* bio = NULL;
	SOCKET fd;
	struct sockaddr_in addr;
	char buf[BUFFER_SIZE];
	char userpwd[50];
	int len;
	ftp_sslsocket_channel ssl_channel(*sslctx);
	ftp_tcp_channel tcp_channel;
	ftp_channel* channel;

	if (argc < 3) {
		usage();
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = gethostaddr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
		printf("main(): connect() faind.\n");
		goto l_exit;
	}
	
	// recv welcome information
	len = recv(fd, buf, sizeof(buf) - 1, 0);
	buf[len] = 0;
	printf("%s", buf);

	tcp_channel.attach(fd);

	if (argc == 4 && stricmp(argv[3], "ssl") == 0) {
		// send auth command
		sprintf(buf, "AUTH SSL"CRLF);
		if (do_ftp_cmd(&tcp_channel, buf, buf, sizeof(buf) - 1) <= 0)
			goto l_exit;

		if (strncmp(buf, "234", 3) != 0) {
			printf("server can't upgrade\n");
			goto l_exit;
		}
		
		Sleep(100);

		if (ssl_channel.connect(fd) < 0) {
			goto l_exit;
		}
		channel = &ssl_channel;
	} else
		channel = &tcp_channel;

	ftp_main(channel);

l_exit:
	return 0;

}
