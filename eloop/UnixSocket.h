#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__

#include "FileLog.h"
#include "json/json.h"
#include "eloop_file.h"


#include <list>
namespace ZQ {
	namespace eloop {

class ZQ_ELOOP_API PipeConnection;
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
	PipeConnection(ZQ::common::Log& log):_lipcLog(log)
	{
		#ifdef ZQ_OS_LINUX
				//Ignore SIGPIPE signal
				signal(SIGPIPE, SIG_IGN);
		#endif
	}
	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void onError( int error,const char* errorDescription ){}
	
	virtual void OnMessage(std::string& msg){}
	virtual int send(const std::string& msg, int fd = -1);
	
	void encode(const std::string& src,std::string& dest);
	void processMessage(ssize_t nread, const char *buf);


protected:
	ZQ::common::Log& _lipcLog;
private:
	std::string		_buf;
};

}}

#endif
