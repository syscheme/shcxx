#include "worker.h"





void Transport::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		if (nread != elpe__EOF)
			fprintf(stderr, "Read error %s\n", errName(nread));
		close();
		return;
	}


	if (!pending_count()) {
		fprintf(stderr, "No pending count\n");
		return;
	}

	eloop_handle_type  pending = pending_type();
	if (pending_type() == ELOOP_TCP)
	{
		printf("pending type is tcp\n");
		return;
	}
	
	worker* client = new worker();
	client->init(get_loop());

	if (accept(client) == 0) {
		Handle::fd_t fd;
		client->fileno(&fd);
		fprintf(stderr, "Worker %d: Accepted fd %d\n", getpid(), fd);
		client->read_start();
	}
	else {
		client->close();
	}
}

void worker::OnRead(ssize_t nread, const char *buf)
{
	if (nread < 0) {
		if (nread != elpe__EOF)
			fprintf(stderr, "Read error %s\n", errName(nread));
		close();
		return;
	}
	std::string wbuf = "worker:";
	wbuf.append(buf);
	write(wbuf.c_str(),wbuf.size());
}