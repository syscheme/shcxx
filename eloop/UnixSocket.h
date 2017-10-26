#ifndef __ZQ_COMMON_ELOOP_UnixSocket_H__
#define __ZQ_COMMON_ELOOP_UnixSocket_H__

#include "FileLog.h"
#include "json/json.h"
#include "eloop_file.h"


#include <list>
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API UnixSocket;
// -----------------------------
// class UnixSocket
// -----------------------------
class UnixSocket : public ZQ::eloop::Pipe
{
public:
	typedef enum
	{
		lipcParseError = -4396
	} LipcError;

public:

	UnixSocket(ZQ::common::Log& log):_lipcLog(log)
	{
		#ifdef ZQ_OS_LINUX
				//Ignore SIGPIPE signal
				signal(SIGPIPE, SIG_IGN);
		#endif
	}

	virtual void OnRead(ssize_t nread, const char *buf);

	// called after buffer has been written into the stream
	virtual void OnWrote(int status) {}

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

}} // namespaces

#endif // __ZQ_COMMON_ELOOP_UnixSocket_H__
