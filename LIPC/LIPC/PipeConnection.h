#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__


#include "ZQ_common_conf.h"
#include "Log.h"
#include "json/json.h"
// #include "Handler.h"

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
class PipeConnection : public ZQ::eloop::Pipe
{
public:
	typedef enum
	{
		lipcParseError = -4396
	} LipcError;

public:
	PipeConnection(ZQ::common::Log& log):_lipcLog(log)
	{
#ifdef ZQ_OS_LINUX
		//Ignore SIGPIPE signal
		signal(SIGPIPE, SIG_IGN);
#endif
	}

	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void onError( int error,const char* errorDescription) { fprintf(stderr, "errCode=%d errDesc:%s\n",error,errorDescription);}
	
	virtual int send(const std::string& msg, ZQ::eloop::Handle* send_handle=NULL);
	virtual int sendfd(const std::string& msg, int fd = -1);
	
	virtual void OnMessage(std::string& msgstr) {}

	void encode(const std::string& src ,std::string& dest);
	void processMessage(ssize_t nread, const char *buf);

protected:

	ZQ::common::Log& _lipcLog;
	 
private:
	std::string		_buf;
};

}}

#endif
