// ftpsclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ssllib.h"

#define BUFFER_SIZE		1024

/* BIO* err = NULL; */
SSL_CTX* ctx = NULL;
char localroot[260] = "C:\\";

#define END_FTP_CMDS	{0, 0}

typedef struct _ftp_cmd_arg ftp_cmd_arg;

int ftp_cmd_lcd(char* cmdline, ftp_cmd_arg* arg);
int ftp_cmd_quit(char* cmdline, ftp_cmd_arg* arg);
int ftp_cmd_cls(char* cmdline, ftp_cmd_arg* arg);

int ftp_cmd_cd(char* cmdline, ftp_cmd_arg* arg);
int ftp_cmd_dir(char* cmdline, ftp_cmd_arg* arg);
int ftp_cmd_get(char* cmdline, ftp_cmd_arg* arg);

typedef struct _ftp_cmd_arg {
	SSL* ssl_cmd;
}ftp_cmd_arg;

struct ftp_cmd {
	char* cmd;
	int (* cmd_handler)(char* cmdline, ftp_cmd_arg* arg);
}ftp_cmds[] = {
	
	/* local command */
	{"LCD", ftp_cmd_lcd}, 
	{"QUIT", ftp_cmd_quit}, 
	{"CLS", ftp_cmd_cls}, 
	
	/* remote command */
	{"CD", ftp_cmd_cd}, 
	{"DIR", ftp_cmd_dir}, 
	{"LS", ftp_cmd_dir}, 
	{"GET", ftp_cmd_get}, 
	END_FTP_CMDS
};

/*
int err_printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "---");
	return vfprintf(stderr, format, args);
	
}

void _ERR_print_errors(BIO* p)
{
	//fprintf(stderr, "-----------------------------------------\n");
	ERR_print_errors(p);
	//fprintf(stderr, "-----------------------------------------\n");
}
*/
int get_data_hostport(char* respond, char* ipaddr, unsigned short* port)
{
	char* c = respond, *p, *start;
	unsigned short part1, part2;
	char buf[16];
	int i = 0;

	if (respond == NULL)
		return 0;

	while(*c) {
		if (*c == '(') {
			start = ++c;
			break;
		}
		c++;
	}

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

int do_ftp_cmd(SOCKET fd, char* cmd, char* outbuf, int outlen)
{
	int len;

	/* printf("---%s\n", cmd); */
	if (send(fd, cmd, strlen(cmd), 0) < 0) {
		SSLDebug::print("do_ftp_cmd(): send() failed.\n");
		return 0;
	}

	len = recv(fd, outbuf, outlen, 0);
	if (len < 0) {
		SSLDebug::print("do_ftp_cmd(): recv() failed.\n");
		return 0;
	}

	outbuf[len] = 0;
	printf(outbuf);

	return len;
}

int do_ftps_cmd(SSL* ssl, char* cmd, char* outbuf, int outlen)
{
	int len;

	/* printf("---%s\n", cmd); */
	if (SSL_write(ssl, cmd, strlen(cmd)) < 0) {
		SSLDebug::print("do_ftp_cmd(): SSL_write() failed.\n");
		SSLDebug::dumpInternalError();
		return 0;
	}

	len = SSL_read(ssl, outbuf, outlen);
	if (len < 0) {
		SSLDebug::print("do_ftp_cmd(): SSL_read() failed.\n");
		SSLDebug::dumpInternalError();
		return 0;
	}

	outbuf[len] = 0;
	printf(outbuf);

	return len;
}

int do_ftps_list(SSL* ssl, char* outbuf, int outlen)
{
	SSL* ssl2 = NULL;
	BIO* bio2;
	unsigned short dport;
	char host[20];
	int len = 0;
	
	sprintf(outbuf, "PASV\n");
	if (do_ftps_cmd(ssl, outbuf, outbuf, outlen) <= 0)
		goto l_exit;
	
	if (get_data_hostport(outbuf, host, &dport) == 0) {
		SSLDebug::print("PASV command failed.\n");
		SSLDebug::dumpInternalError();
		goto l_exit;
	}

	sprintf(outbuf, "LIST\n");
	if (do_ftps_cmd(ssl, outbuf, outbuf, outlen) <= 0)
		goto l_exit;

	ssl2 = SSL_new(ctx);
	sprintf(outbuf, "%s:%d", host, dport);
	bio2 = BIO_new_connect(outbuf);
	SSL_set_bio(ssl2, bio2, bio2);
	if (SSL_connect(ssl2) <= 0) {
		SSLDebug::print("do_ftps_list(): SSL_connect() failed.\n");
		SSLDebug::dumpInternalError();
		goto l_exit;
	}

	do {
		len = SSL_read(ssl2, outbuf, outlen);
		outbuf[len] = 0;
		printf(outbuf);
	}while(SSL_get_error(ssl2, len) == SSL_ERROR_NONE);

	len = SSL_read(ssl, outbuf, outlen);
	outbuf[len] = 0;
	printf(outbuf);

l_exit:
	
	if (ssl2) {
		SSL_shutdown(ssl2);
		SSL_free(ssl2);
	}
	return len;
}

int do_ftps_get(SSL* ssl, char* filename, char* outbuf, int outlen)
{
	SSL* ssl2 = NULL;
	BIO* bio2;
	unsigned short dport;
	char host[20];
	int len = 0;

	sprintf(outbuf, "TYPE I\n");
	if (do_ftps_cmd(ssl, outbuf, outbuf, outlen) <= 0)
		goto l_exit;
	
	if (strncmp(outbuf, "200", 3) != 0) {
		SSLDebug::print("server TYPE command failed.\n");
		goto l_exit;
	}

	sprintf(outbuf, "PASV\n");
	if (do_ftps_cmd(ssl, outbuf, outbuf, outlen) <= 0)
		goto l_exit;
	
	if (get_data_hostport(outbuf, host, &dport) == 0) {
		SSLDebug::print("PASV command failed.\n");
		SSLDebug::dumpInternalError();
		goto l_exit;
	}

	sprintf(outbuf, "RETR %s\n", filename);
	if (do_ftps_cmd(ssl, outbuf, outbuf, outlen) <= 0)
		goto l_exit;

	ssl2 = SSL_new(ctx);
	sprintf(outbuf, "%s:%d", host, dport);
	bio2 = BIO_new_connect(outbuf);
	SSL_set_bio(ssl2, bio2, bio2);
	if (SSL_connect(ssl2) <= 0) {
		SSLDebug::print("main(): SSL_connect() failed.\n");
		SSLDebug::dumpInternalError();
		goto l_exit;
	}
	
	do {
		len = SSL_read(ssl2, outbuf, outlen);
		outbuf[len] = 0;
		printf(outbuf);
	}while(SSL_get_error(ssl2, len) == SSL_ERROR_NONE);

	len = SSL_read(ssl, outbuf, outlen);
	outbuf[len] = 0;

l_exit:
	if (ssl2) {
		SSL_shutdown(ssl2);
		SSL_free(ssl2);
	}
	return len;
}

void usage()
{
	printf("usage:\n\tcmd host port\n");
}

int dispatch_cmd(char* cmdline, ftp_cmd_arg* arg)
{
	char cmdname[32];
	char* c= cmdline;
	struct ftp_cmd* ftpcmd = ftp_cmds;
	int n;

	while(*c && *c != ' ' && *c != '\t')
		c++;
	n = (unsigned long )c - (unsigned long )cmdline;
	strncpy(cmdname, cmdline, n);
	cmdname[n] = 0;
	
	while(ftpcmd->cmd != NULL) {
		if (stricmp(ftpcmd->cmd, cmdname) == 0)
			return ftpcmd->cmd_handler(cmdline, arg);
		
		ftpcmd ++;
	}

	printf("Unknown command.\n");
	return 0;
}

void process_cmd(ftp_cmd_arg* arg)
{
	char cmdline[500];
	while(1) {
		printf("FTP>");
		gets(cmdline);
		if (strlen(cmdline))
			if (dispatch_cmd(cmdline, arg) < 0)
				return;
	}
}

int ftp_cmd_lcd(char* cmdline, ftp_cmd_arg* arg)
{
	char* c;
	if (strlen(cmdline) == 3) {
		printf("%s\n", localroot);
		return 0;
	}

	c = cmdline + 3;
	while(*c != ' ' && *c != '\t')
		c++;
	strcpy(localroot, c);
	return 0;
}

int ftp_cmd_quit(char* cmdline, ftp_cmd_arg* arg)
{
	return -1;
}

int ftp_cmd_cls(char* cmdline, ftp_cmd_arg* arg)
{
	system("cls");
	return 0;
}

int ftp_cmd_cd(char* cmdline, ftp_cmd_arg* arg)
{
	char buf[BUFFER_SIZE];
	char* c;
	if (strlen(cmdline) == 2) {
		sprintf(buf, "PWD\n");
		return do_ftps_cmd(arg->ssl_cmd, buf, buf, sizeof(buf) - 1);
	}
	c = cmdline + 2;
	while(*c != ' ' && *c != '\t')
		c++;
	sprintf(buf, "CWD %s\n", c + 1);
	return do_ftps_cmd(arg->ssl_cmd, buf, buf, sizeof(buf) - 1);
}

int ftp_cmd_dir(char* cmdline, ftp_cmd_arg* arg)
{
	char buf[BUFFER_SIZE];
	do_ftps_list(arg->ssl_cmd, buf, sizeof(buf) - 1);
	
	return 0;
}

int ftp_cmd_get(char* cmdline, ftp_cmd_arg* arg)
{
	char buf[BUFFER_SIZE];
	char filename[MAX_PATH];
	char* c;
	
	c = cmdline + 3;
	while(*c != ' ' && *c != '\t')
		c++;
	strcpy(filename, ++c);
	do_ftps_get(arg->ssl_cmd, filename, buf, sizeof(buf) - 1);
	return 0;
}

int check_cert(SSL* ssl, char* comm_name)
{
	X509* peer;
	char cn[64];
	BIO* bioout;

	bioout = BIO_new_fd(fileno(stdout),	BIO_NOCLOSE);

	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		SSLDebug::print("server certificate is valid.\n");
		return -1;
	}

	peer = SSL_get_peer_certificate(ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, 
		cn, sizeof(cn));
	X509_print(bioout, peer);
	BIO_free(bioout);
	
	// printf("The server's common name is: %s\n", cn);
	if (stricmp(cn, comm_name) != 0) {
		SSLDebug::print("common name doesn't match.\n");
		return -1;
	}

	return 0;
}

u_long gethostaddr(char* hostname)
{
	u_long result;
	result = inet_addr(hostname);
	if (result == INADDR_NONE) {
		hostent* ht = gethostbyname(hostname);
		if (ht != NULL, ht->h_addr_list[0] != NULL)
			memcpy(&result, ht->h_addr_list[0], ht->h_length);
	}

	return result;
}

int raw_api_main(int argc, char* argv[])
{
	SSL* ssl = NULL;
	BIO* bio = NULL;
	SOCKET fd;
	struct sockaddr_in addr;
	char buf[BUFFER_SIZE];
	char userpwd[50];
	int len;
	ftp_cmd_arg cmdarg;

	if (argc != 3) {
		usage();
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
		SSLDebug::print("main(): connect() faind.\n");
		goto l_exit;
	}
	
	// recv welcome information
	len = recv(fd, buf, sizeof(buf) - 1, 0);
	buf[len] = 0;
	printf("%s", buf);

	// send auth command
	sprintf(buf, "AUTH SSL\n");
	if (do_ftp_cmd(fd, buf, buf, sizeof(buf) - 1) <= 0)
		goto l_exit;

	if (strncmp(buf, "234", 3) != 0) {
		printf("server can't upgrade\n");
		goto l_exit;
	}

	// upgrading to ssl
	SSL_library_init();
	SSL_load_error_strings();
	SSLDebug::init(cerr);
	// err = BIO_new_fd(fileno(stderr), BIO_NOCLOSE);

	ctx = SSL_CTX_new(SSLv3_method());
	if (!SSL_CTX_load_verify_locations(ctx, "ftps_sslcert.pem", 0)) {
		SSLDebug::print("main(): SSL_CTX_load_verify_locations() failed.\n");
		goto l_exit;
	}

	if (!RAND_load_file("ftps_sslcert.pem", 1024 * 1024)) {
		SSLDebug::print("main(): RAND_load_file() failed.\n");
		goto l_exit;
	}

	ssl = SSL_new(ctx);
	bio = BIO_new_socket(fd, BIO_NOCLOSE);
	SSL_set_bio(ssl, bio, bio);
	Sleep(100);
	if (SSL_connect(ssl) <= 0) {
		SSLDebug::print("main(): SSL_connect() failed.\n");
		SSLDebug::dumpInternalError();
		goto l_exit;
	}
	
	if (check_cert(ssl, "myserver.com") != 0)
		goto l_exit;

	printf("User: ");
	gets(userpwd);
	sprintf(buf, "USER %s\n", userpwd);
	if (do_ftps_cmd(ssl, buf, buf, sizeof(buf) - 1) <= 0)
		goto l_exit;

	printf("Password: ");
	gets(userpwd);
	sprintf(buf, "PASS %s\n", userpwd);
	if (do_ftps_cmd(ssl, buf, buf, sizeof(buf) - 1) <= 0)
		goto l_exit;

	cmdarg.ssl_cmd = ssl;
	process_cmd(&cmdarg);
	
l_exit:

	closesocket(fd);
	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
	if (ctx)
		SSL_CTX_free(ctx);
	/*
	if (err)
		BIO_free(err);
	*/
	return 0;
}

int cpp_main(int argc, char* argv[]);
int echo_server_main(int argc, char* argv[]);
int main(int argc, char* argv[])
{
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 0), &wsad);
	init_ssl_library();
	return echo_server_main(argc, argv);
}
