#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__


#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#  ifdef LIPC_EXPORTS
#    define ZQ_LIPC_API __EXPORT
#  else
#    define ZQ_LIPC_API __DLLRTL
#  endif
#else
#  define ZQ_LIPC_API
#endif // OS


#include "eloop_file.h"
#include <list>
namespace ZQ {
	namespace LIPC {

class ZQ_LIPC_API PipeConnection;
class ZQ_LIPC_API TransferFdClient;
class ZQ_LIPC_API TransferFdService;
// -----------------------------
// class PipeConnection
// -----------------------------
class PipeConnection:public ZQ::eloop::Pipe
{
public:
	PipeConnection(){}
	~PipeConnection(){}
	
	virtual void OnRead(ssize_t nread, const char *buf);
	
	virtual void OnRequest(std::string& req){printf("OnRequest,len = %d,buf = %s\n",req.size(),req.c_str());}
	virtual int send(const char* buf,size_t len,ZQ::eloop::Handle* send_handle=NULL);
	
	void encode(const std::string& src,std::string& dest);
	void decode(ssize_t nread, const char *buf);
	 
private:
	std::string		_buf;
};

class Dispatcher;
// -----------------------------
// class TransferFdClient
// -----------------------------
class TransferFdClient:public PipeConnection
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
class PipePassiveConn:public PipeConnection
{
public:
	PipePassiveConn(TransferFdService& service);
	~PipePassiveConn();
	void start();
	virtual void OnRequest(std::string& req);
	virtual void OnWrote(int status);
	virtual void OnClose();
	virtual int send(const char* buf,size_t len);
	bool	isAck(){return _sendAck;}

private:
	TransferFdService&	_service;
	bool				_sendAck;
};

/*
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
*/
// -----------------------------
// class TransferFdService
// -----------------------------
class TransferFdService:public ZQ::eloop::Pipe//,public ServantManager
{
public:
	typedef std::list<PipePassiveConn*> PipeClientList;
public:
	TransferFdService();
	~TransferFdService();
	void addConn(PipePassiveConn* conn);
	void delConn(PipePassiveConn* conn);
	PipeClientList& getPipeClientList(){return _ClientList;}
	
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void onRequest(std::string& req,PipePassiveConn* conn){}

private:
	PipeClientList _ClientList;
};

}}

#endif
