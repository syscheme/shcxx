#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__


#include "ZQ_common_conf.h"
#include "json/json.h"
#include "Handler.h"

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
	PipeConnection(ZQ::common::Log& log):_log(log)
	{
		#ifdef ZQ_OS_LINUX
				//Ignore SIGPIPE signal
				signal(SIGPIPE, SIG_IGN);
		#endif
	}
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void onError( int error,const char* errorDescription ){	fprintf(stderr, "errCode=%d errDesc:%s\n",error,errorDescription);}
	
	virtual void OnMessage(std::string& msg){}
	virtual int send(Arbitrary value,ZQ::eloop::Handle* send_handle=NULL);
	virtual int sendfd(Arbitrary value,int fd = -1);
	
	void encode(const std::string& src,std::string& dest);
	void processMessage(ssize_t nread, const char *buf);

	std::string GetString(Arbitrary value);
	 
private:
	std::string		_buf;
	Json::FastWriter m_writer;
	ZQ::common::Log& _log;
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

	virtual void OnMessage(std::string& msg);
	virtual void onError( int error,const char* errorDescription);
	virtual void OnWrote(int status);
	virtual void OnClose();
	bool	isAck(){return _sendAck;}

private:
	Service&	_service;
	bool				_sendAck;
};

}}

#endif
