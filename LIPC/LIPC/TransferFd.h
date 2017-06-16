#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__

#include "eloop_file.h"
#include <list>
namespace ZQ {
	namespace LIPC {

class Dispatcher;
// -----------------------------
// class TransferFdClient
// -----------------------------
class TransferFdClient:public ZQ::eloop::Pipe
{
public:
	TransferFdClient(Dispatcher& disp);
	~TransferFdClient();
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status);
//	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);
private:
	Dispatcher& _disp;
};


// -----------------------------
// class PipePassiveConn
// -----------------------------
class TransferFdService;
class PipePassiveConn:public ZQ::eloop::Pipe
{
public:
	PipePassiveConn(TransferFdService& service);
	~PipePassiveConn();
	void start();
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);
	virtual void OnClose();

private:
	TransferFdService&	_service;
};


// ------------------------------------------------
// class ServantManager
// ------------------------------------------------
class Servant;
class ServantManager
{
public:
	ServantManager();
	virtual ~ServantManager();
	void addServant(Servant* svt);
	void delServant(Servant* svt);
	virtual void onRequest(const char* req,Servant* st){}

private:
	std::list<Servant*>			_servantMgr;
};

// -----------------------------
// class TransferFdService
// -----------------------------
class TransferFdService:public ZQ::eloop::Pipe,public ServantManager
{
public:
	TransferFdService();
	~TransferFdService();
	void addConn(PipePassiveConn* conn);
	void delConn(PipePassiveConn* conn);
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);

private:
	std::list<PipePassiveConn*> _PipeConn;
};

}}

#endif