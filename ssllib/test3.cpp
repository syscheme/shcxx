#include "stdafx.h"
#include "ssllib.h"
#define BUFFER_SIZE		1024
extern u_long gethostaddr(char* hostname);

int ssh_main(int argc, char* argv[])
{
	SSLContext sctx(VER_SSLv2);
	SOCKET fd;
	SSLSocket ssock(sctx);
	struct sockaddr_in addr;
	char buf[BUFFER_SIZE];
	char userpwd[50];
	int len;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = gethostaddr("newsmth.org");
	addr.sin_port = htons(22);
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
		SSLDebug::print("main(): connect() faind.\n");
		goto l_exit;
	}
	
	// recv welcome information
	len = recv(fd, buf, sizeof(buf) - 1, 0);
	buf[len] = 0;
	printf("%s", buf);

	sctx.loadRootCertificates("ftps_sslcert.pem");
	sctx.loadRandomessFile("ftps_sslcert.pem");
	ssock.attach(fd);
	Sleep(100);
	if (ssock.connect() <= 0) {
		SSLDebug::dumpInternalError();
		goto l_exit;
	}
	ssock.write(buf, 100);
	len = ssock.read(buf, sizeof(buf) - 1);
	buf[len] = 0;
	printf("%s", buf);
	

l_exit:
	return -1;

}