#include "ZQ_common_conf.h"
#include "LIPC.h"
#include "TransferFd.h"

#ifdef ZQ_OS_LINUX
	#include <dirent.h>
#endif

namespace ZQ {
	namespace LIPC {

class TCPtemp:public ZQ::eloop::TCP
{
public:
	virtual void OnRead(ssize_t nread, const char *buf)
	{
		printf("----------recv data:%s,len = %d\n", buf,nread);

		write(buf,nread);
	}
};
// ------------------------------------------------
// class Dispatcher
// ------------------------------------------------
Dispatcher::Dispatcher()
{
}

Dispatcher::~Dispatcher()
{
}

void Dispatcher::scan(const char* pathname)
{
#ifdef ZQ_OS_LINUX
	DIR *dirptr=NULL;
	struct dirent *entry;
	std::string filename;
	if((dirptr = opendir(pathname))==NULL)
	{
		printf("opendir failed!");
		return;
	}
	else
	{
		while(entry=readdir(dirptr))
		{
			if (0==strcmp(entry->d_name,".") || 0==strcmp(entry->d_name,".."))
				continue;

			filename = pathname + std::string(entry->d_name);
			printf("filename=%s\n",filename.c_str());
			TransferFdClient* client = new TransferFdClient(*this);
			client->init(get_loop());
			client->connect(filename.c_str());
		}
		closedir(dirptr);
	}
#endif
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

	//ZQ::eloop::TCP *temp = new ZQ::eloop::TCP();
	TCPtemp *temp = new TCPtemp();
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
			temp->write(dummy_buf,1);
			//temp->read_start();
			if(ret != 0)
				printf("write ret = %d,errDesc:%s\n",ret,ZQ::eloop::Handle::errDesc(ret));
		}
	}
	else {
		temp->close();
	}
}


// -------------------------------------------------
// class JsonRpcService
// -------------------------------------------------
JsonRpcService::JsonRpcService()
{
}
JsonRpcService::~JsonRpcService()
{
}

void JsonRpcService::onRequest(std::string& req,PipePassiveConn* conn)
{
	_conn = conn;
	Arbitrary respon = Arbitrary::null;
	Process(req,respon);
	if(respon != Arbitrary::null)
	{
		std::string resp = GetString(respon).c_str();
		conn->send(resp.c_str(),resp.size());		
	}
}

int JsonRpcService::acceptPendingHandle(ZQ::eloop::Handle* h)
{
	if(_conn == NULL)
		return -1;
	return _conn->accept(h);
}

ZQ::eloop::Handle::eloop_handle_type JsonRpcService::getPendingHandleType()
{
	if(_conn == NULL)
		return ZQ::eloop::Handle::ELOOP_UNKNOWN_HANDLE;
	return _conn->pending_type();
}

int JsonRpcService::getPendingCount()
{
	if(_conn == NULL)
		return -1;
	return _conn->pending_count();
}


// ------------------------------------------------
// class JsonRpcClient
// ------------------------------------------------
int JsonRpcClient::sendRequest(ZQ::LIPC::Arbitrary& value,ZQ::eloop::Handle* send_Handler)
{
	std::string src = GetString(value);
	return send(src,send_Handler);
}
void JsonRpcClient::OnRequest(std::string& req)
{
	Arbitrary respon = Arbitrary::null;
	Process(req,respon);
	if(respon != Arbitrary::null)
	{
		std::string resp = GetString(respon);
		send(resp);		
	}
}

}}//ZQ::LIPC
