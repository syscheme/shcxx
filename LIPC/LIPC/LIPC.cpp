#include "ZQ_common_conf.h"
#include "LIPC.h"
#include "TransferFd.h"

#ifdef ZQ_OS_LINUX
	#include <dirent.h>
#endif

namespace ZQ {
	namespace LIPC {

// -------------------------------------------------
// class Service
// -------------------------------------------------
Service::Service()
{
}
Service::~Service()
{
}

int Service::init(ZQ::eloop::Loop &loop, int ipc)
{
	_ipc = ipc;
	return ZQ::eloop::Pipe::init(loop,ipc);
}

void Service::addConn(PipePassiveConn* conn)
{
	_ClientList.push_back(conn);
}

void Service::delConn(PipePassiveConn* conn)
{
	PipeClientList::iterator iter = _ClientList.begin();
	while(iter != _ClientList.end())
	{
		if (*iter == conn)
			iter = _ClientList.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
}

void Service::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n",Handle::errDesc(status));
		return;
	}

	PipePassiveConn *client = new PipePassiveConn(*this);
	client->init(get_loop(),_ipc);

	int ret = accept(client);
	if (ret == 0) {
		client->start();
	}
	else {
		client->close();
		printf("accept error,code = %d,desc:%s\n",ret,ZQ::eloop::Handle::errDesc(ret));
	}
}

void Service::OnMessage(std::string& req, PipePassiveConn* conn)
{
	_conn = conn;
	Arbitrary respon = Arbitrary::null;
	Process(req, respon);
	if(respon != Arbitrary::null)
	{
		std::string resp = GetString(respon).c_str();
		conn->send(resp);		
	}
}

int Service::acceptPendingHandle(ZQ::eloop::Handle* h)
{
	if(_conn == NULL)
		return -1;
	return _conn->accept(h);
}

ZQ::eloop::Handle::eloop_handle_type Service::getPendingHandleType()
{
	if(_conn == NULL)
		return ZQ::eloop::Handle::ELOOP_UNKNOWN_HANDLE;
	return _conn->pending_type();
}

int Service::getPendingCount()
{
	if(_conn == NULL)
		return -1;
	return _conn->pending_count();
}


// ------------------------------------------------
// class Client
// ------------------------------------------------
int Client::sendRequest(ZQ::LIPC::Arbitrary& value, ZQ::eloop::Handle* send_Handler)
{
	std::string src = GetString(value);
	return send(src, send_Handler);
}

int Client::sendRequest(ZQ::LIPC::Arbitrary& value,int fd)
{
	std::string src = GetString(value);
	return sendfd(src, fd);
}

void Client::OnMessage(std::string& req)
{
	Arbitrary respon = Arbitrary::null;
	Process(req, respon);
	if(respon != Arbitrary::null)
	{
		std::string resp = GetString(respon);
		send(resp);		
	}
}

}}//ZQ::LIPC
