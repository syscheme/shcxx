#include "LIPC.h"
#include "TransferFd.h"
#include <dirent.h>

namespace ZQ {
	namespace LIPC {

// -----------------------------
// class Dispatcher
// -----------------------------
Dispatcher::Dispatcher(const char* pathname)
{
	scan(pathname);
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::scan(const char* pathname)
{
	DIR *dirptr=NULL;
	int i=1;
	struct dirent *entry;
	if((dirptr = opendir(pathname))==NULL)
	{
		printf("opendir failed!");
		return;
	}
	else
	{
		while(entry=readdir(dirptr))
		{
			printf("filename%d=%s\n",i,entry->d_name);
			i++;
		}
		closedir(dirptr);
	}
}

void Dispatcher::addServant(TransferFdClient* client)
{
	_servantVec.push_back(client);
}

void Dispatcher::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n", ZQ::eloop::Handle::errDesc(status));
		return;
	}

	ZQ::eloop::TCP *temp = new ZQ::eloop::TCP();
	temp->init(get_loop());

	if (accept(temp) == 0) {

		if (_servantVec.empty())
		{
			char* dummy_buf = "can not find server";
			temp->write(dummy_buf,sizeof(dummy_buf));
			temp->shutdown();
		}
		else
		{
			char* dummy_buf = "a";
			printf("send fd\n");
			int ret = _servantVec[0]->write(dummy_buf,1,temp);
			if(ret != 0)
				printf("write ret = %d,errDesc:%s\n",ret,ZQ::eloop::Handle::errDesc(ret));
		}
	}
	else {
		temp->close();
	}
}

void Dispatcher::OnWrote(ZQ::eloop::Handle::ElpeError status)
{
	printf("CentralServer OnWrote.\n");
}
// called after buffer has been read from the stream
void Dispatcher::OnRead(ssize_t nread, const char *buf)
{
	printf("CentralServer OnRead.\n");
}









}}//ZQ::LIPC