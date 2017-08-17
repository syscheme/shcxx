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


#include "eloop/eloop_file.h"
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
	typedef enum{
		lipcParseError = -4396
	} LipcError;
public:
	PipeConnection(){}
	~PipeConnection(){}
	
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void onError( int error,const char* errorDescription ){	fprintf(stderr, "errCode=%d errDesc:%s\n",error,errorDescription);}
	
	virtual void OnMessage(std::string& req) { printf("OnRequest,len = %d,buf = %s\n",req.size(),req.c_str()); }
	virtual int send(std::string buf,ZQ::eloop::Handle* send_handle=NULL);
	
	void encode(const std::string& src,std::string& dest);
	void processMessage(ssize_t nread, const char *buf);
	 
private:
	std::string		_buf;
};


// -----------------------------
// class PipePassiveConn
// -----------------------------
class Service;
class PipePassiveConn : public PipeConnection
{
public:
	PipePassiveConn(Service& service);
	~PipePassiveConn();
	void start();
	virtual void OnRequest(std::string& req);
	virtual void OnWrote(int status);
	virtual void OnClose();
	virtual int send(const char* buf,size_t len);
	bool	isAck(){return _sendAck;}

private:
	Service&	_service;
	bool				_sendAck;
};

}}

#endif
