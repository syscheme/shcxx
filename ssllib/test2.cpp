#include "stdafx.h"
#include "ssllib.h"
using namespace ZQ;

int echo_server_main2(int argc, char* argv[])
{
	int len;
	SSLContext ctx;
	SSLSocket ssock(INVALID_SOCKET, &ctx);

	if (!ctx.loadCertificate("ftps_sslcert.pem")){
		return -1;
	}

	if (!ctx.loadPrivateKeyFile("ftps_sslcert.pem", "")) {
		return -1;
	}

	if (!ctx.loadRootCertificates("ftps_sslcert.pem")) {
		return -1;
	}
	
	if (!load_randomess_file("ftps_sslcert.pem")) {
		return -1;
	}

	char buf[1024];
	SSLSocket* ssock2 = (SSLSocket* )ssock.accept(NULL, NULL);
	while(true) {
		len = ssock2->recv(buf, sizeof(buf) - 1);
		if (len > 0) {
			buf[len] = 0;
			ssock2->send(buf, strlen(buf));
		}
	}
	delete ssock2;
	return 0;	
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	return 1;
}
extern SSL_CTX* ctx;
int echo_server_main(int argc, char* argv[])
{
	SSL* ssl = NULL;
	BIO* bio = NULL;
	SOCKET fd, newfd;
	struct sockaddr_in addr;
	char buf[1024];
	char userpwd[50];
	int len;
	X509   *peer;
	BIO* bioerr = BIO_new_fd(fileno(stderr), BIO_NOCLOSE);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(4433);
	bind(fd, (sockaddr* )&addr, sizeof(addr));
	listen(fd, 10);
	newfd = accept(fd, NULL, NULL);

	ctx = SSL_CTX_new(SSLv3_method());
	if (!SSL_CTX_use_PrivateKey_file(ctx, "ftps_sslcert.pem", SSL_FILETYPE_PEM)) {
		goto l_exit;
	}

	if(!(SSL_CTX_use_certificate_file(ctx, "ftps_sslcert.pem", SSL_FILETYPE_PEM))) {
		printf("Couldn't read certificate file");
		goto l_exit;
	}

	// SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);

	if (!SSL_CTX_load_verify_locations(ctx, "ftps_sslcert.pem", 0)) {
		printf("main(): SSL_CTX_load_verify_locations() failed.\n");
		goto l_exit;
	}

	if (!RAND_load_file("ftps_sslcert.pem", 1024 * 1024)) {
		printf("main(): RAND_load_file() failed.\n");
		goto l_exit;
	}

	ssl = SSL_new(ctx);
	bio = BIO_new_socket(newfd, BIO_NOCLOSE);
	SSL_set_bio(ssl, bio, bio);
	SSL_set_verify(ssl, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);
	Sleep(100);
	if (SSL_accept(ssl) <= 0) {
		goto l_exit;
	}

	peer = SSL_get_peer_certificate(ssl);
	if (peer)
		X509_print(bioerr, peer);

	while(true) {
		len = SSL_read(ssl, buf, sizeof(buf));
		if (len > 0) {
			buf[len] = 0;
			SSL_write(ssl, buf, strlen(buf));
		}
	}
l_exit:
	return 0;
}

//////////////////////////////////////////////////////////////////////////

// 直接安全连接
int echocli_main1(int argc, char* argv[])
{
	SSLContext ctx(VER_SSLv3);
	SSLSocket ssock(INVALID_SOCKET, &ctx);
	sockaddr_in addr;
	char buf[1024];
	int len;
	ctx.loadRootCertificates("roots.pem");
	ssock.socket();
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9090);
	ssock.connect((sockaddr* )&addr, sizeof(addr), false);
	printf(">");
	gets(buf);
	len = ssock.send(buf, strlen(buf));
	if (len > 0) {
		len = ssock.recv(buf, sizeof(buf));
		if (len > 0)
			printf("%s", buf);
	}
	ssock.close();

	return 0;
}

// 磋商升级
int echocli_main2(int argc, char* argv[])
{
	SSLContext ctx(VER_SSLv3);
	SSLSocket ssock(INVALID_SOCKET, &ctx);
	sockaddr_in addr;
	char buf[1024];
	int len;
	ctx.loadRootCertificates("roots.pem");
	ssock.socket();
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9090);
	ssock.connect((sockaddr* )&addr, sizeof(addr), false);
	ssock.send("auth", 4);
	len = ssock.recv(buf, sizeof(buf) - 1);
	buf[len] = 0;
	if (stricmp(buf, "okay") == 0)
		ssock.s_connect();
	
	printf(">");
	gets(buf);
	len = ssock.send(buf, strlen(buf));
	if (len > 0) {
		len = ssock.recv(buf, sizeof(buf));
		if (len > 0)
			printf("%s", buf);
	}
	ssock.close();

	return 0;
}

// 恢复连接
int echocli_main(int argc, char* argv[])
{
	SSLContext ctx(VER_SSLv3);
	SSLSocket ssock(INVALID_SOCKET, &ctx);
	sockaddr_in addr;
	char buf[1024];
	int len;
	
	ctx.loadCertificate("client.pem");
	ctx.loadPrivateKeyFile("client.pem", "xiao");
	ctx.loadRootCertificates("client.pem");
	
	ssock.socket();
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9090);
	ssock.connect((sockaddr* )&addr, sizeof(addr), false);
	ssock.send("auth", 4);
	len = ssock.recv(buf, sizeof(buf) - 1);
	buf[len] = 0;
	if (stricmp(buf, "okay") == 0)
		ssock.s_connect();
	
	printf(">");
	gets(buf);
	len = ssock.send(buf, strlen(buf));
	if (len > 0) {
		len = ssock.recv(buf, sizeof(buf));
		if (len > 0)
			printf("%s\n", buf);
	}
	ssock.close();

	ssock.socket();
	ssock.connect((sockaddr* )&addr, sizeof(addr), false);
	ssock.send("auth", 4);
	len = ssock.recv(buf, sizeof(buf) - 1);
	buf[len] = 0;
	if (stricmp(buf, "okay") == 0)
		ssock.s_connect();
		printf(">");
	gets(buf);
	len = ssock.send(buf, strlen(buf));
	if (len > 0) {
		len = ssock.recv(buf, sizeof(buf));
		if (len > 0)
			printf("%s\n", buf);
	}

	ssock.close();

	return 0;
}

