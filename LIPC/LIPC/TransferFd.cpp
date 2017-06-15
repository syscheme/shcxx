#include "TransferFd.h"
#include "LIPC.h"

namespace ZQ {
	namespace LIPC {
// -----------------------------
// class TransferFdClient
// -----------------------------
TransferFdClient::TransferFdClient(Dispatcher& disp)
:_disp(disp)
{
}
	
TransferFdClient::~TransferFdClient()
{
}
	 
void TransferFdClient::OnConnected(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
			fprintf(stderr, "on_connect error %s\n", Handle::errDesc(status));
			return;
		}
//	read_start();
	_disp.addServant(this);
}


// -----------------------------
// class PipePassiveConn
// -----------------------------
PipePassiveConn::PipePassiveConn(TransferFdService& service)
:_service(service)
{

}

PipePassiveConn::~PipePassiveConn()
{

}

void PipePassiveConn::start()
{
	read_start();
	_service.addConn(this);
}

void PipePassiveConn::OnRead(ssize_t nread, const char *buf)
{
		if (nread < 0) {
//			if (nread != elpe__EOF)
			fprintf(stderr, "Read error %s\n", errName(nread));
			close();
			return;
		}

/*
		if (!pending_count()) {
			fprintf(stderr, "No pending count\n");
			return;
		}

		eloop_handle_type  pending = pending_type();
		if (pending_type() != ELOOP_TCP)
		{
			printf("pending type is tcp\n");
			return;
		}
*///		printf("accepted_fd = %d\n",);
		Servant* client = new Servant(_service);
		client->init(get_loop());
		int ret = accept(client);
		if (ret == 0) {
			Handle::fd_t fd;
			client->fileno(&fd);
			fprintf(stderr, "Worker %d: Accepted fd %d\n", getpid(), fd);
			client->read_start();
		}
		else {
			printf("accept error ret = %d,errDesc:%s\n",ret,Handle::errDesc(ret));
			client->close();
		}
}

void PipePassiveConn::OnWrote(ZQ::eloop::Handle::ElpeError status)
{

}

void PipePassiveConn::OnClose()
{
	_service.delConn(this);
	delete this;
}

// ------------------------------------------------
// class ServantManager
// ------------------------------------------------
ServantManager::ServantManager()
{

}
ServantManager::~ServantManager()
{

}
void ServantManager::addServant(Servant* svt)
{
	_servantMgr.push_back(svt);
}

void ServantManager::delServant(Servant* svt)
{
	std::list<Servant*>::iterator iter = _servantMgr.begin();
	while(iter != _servantMgr.end())
	{
		if (*iter == svt)
			iter = _servantMgr.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
}


// -----------------------------
// class TransferFdService
// -----------------------------
TransferFdService::TransferFdService()
{

}

TransferFdService::~TransferFdService()
{

}

void TransferFdService::addConn(PipePassiveConn* conn)
{
	_PipeConn.push_back(conn);
}

void TransferFdService::delConn(PipePassiveConn* conn)
{
	std::list<PipePassiveConn*>::iterator iter = _PipeConn.begin();
	while(iter != _PipeConn.end())
	{
		if (*iter == conn)
			iter = _PipeConn.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
}

void TransferFdService::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
		fprintf(stderr, "New connection error %s\n",Handle::errDesc(status));
		return;
	}

	PipePassiveConn *client = new PipePassiveConn(*this);
	client->init(get_loop());

	if (accept((Stream *)client) == 0) {
		client->start();
	}
	else {
		client->close();
	}
}

}}