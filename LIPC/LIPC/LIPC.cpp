#include "ZQ_common_conf.h"
#include "LIPC.h"

#ifdef ZQ_OS_LINUX
	#include <dirent.h>
#endif

namespace ZQ {
	namespace LIPC {

// -------------------------------------------------
// class Service
// -------------------------------------------------
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

// ------------------------------------------------
// class Client
// ------------------------------------------------
int Client::sendHandlerRequest(std::string method,ZQ::LIPC::Arbitrary param,RpcCB cb,void* data,ZQ::eloop::Handle* send_Handler)
{
	ZQ::LIPC::Arbitrary req;
	int seqId = 0;
	req[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	req[JSON_RPC_METHOD] = method;
	if (param != ZQ::LIPC::Arbitrary::null)
		req[JSON_RPC_PARAMS] = param;
	if (cb !=NULL)
	{
		seqId = lastCSeq();
		Addcb(seqId,cb,data);
		req[JSON_RPC_ID] = seqId;
	}

	int ret = send(req, send_Handler); 
	if (ret < 0)
		return ret;
	return seqId;
}

int Client::sendRequest(std::string method,ZQ::LIPC::Arbitrary param,RpcCB cb,void* data,int fd)
{
	ZQ::LIPC::Arbitrary req;
	int seqId = 0;
	req[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	req[JSON_RPC_METHOD] = method;
	if (param != ZQ::LIPC::Arbitrary::null)
		req[JSON_RPC_PARAMS] = param;
	if (cb !=NULL)
	{
		seqId = seqId = lastCSeq();
		Addcb(seqId,cb,data);
		req[JSON_RPC_ID] = seqId;
	}
	int ret = sendfd(req, fd);
	if (ret < 0)
		return ret;
	return seqId;
}

void Client::OnMessage(std::string& msg)
{
	Process(msg, *this);
}

}}//ZQ::LIPC
