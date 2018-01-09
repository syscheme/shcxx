#ifndef __ZQ_COMMON_ELOOP_UnixSocket_H__
#define __ZQ_COMMON_ELOOP_UnixSocket_H__

#include "FileLog.h"
#include "json/json.h"
#include "eloop_file.h"


#include <list>
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API UnixSocket;
class AsyncSender;
// -----------------------------
// class UnixSocket
// -----------------------------
class UnixSocket : public ZQ::eloop::Pipe
{
	friend class AsyncSender;
public:
	typedef enum
	{
		lipcParseError = -4396
	} LipcError;

public:

	UnixSocket(ZQ::common::LogWrapper& log):_lipcLog(log),_async(NULL)
	{
		#ifdef ZQ_OS_LINUX
				//Ignore SIGPIPE signal
				signal(SIGPIPE, SIG_IGN);
		#endif
	}

	int  init(Loop &loop, int ipc=1);
	void closeUnixSocket();

	virtual void OnRead(ssize_t nread, const char *buf);

	// called after buffer has been written into the stream
	virtual void OnWrote(int status);

	virtual void onError( int error,const char* errorDescription ){}
	
	virtual void OnMessage(std::string& msg){}
	virtual int send(const std::string& msg, int fd = -1);
	
	void encode(const std::string& src,std::string& dest);

	void processMessage(ssize_t nread, const char *buf);

// 	void parseMessage(ssize_t nread, const char *buf);
// 	void AsyncProcessMessage();

	int AsyncSend(const std::string& msg, int fd = -1);

	void OnAsyncSend();
	void OnCloseAsync();

//protected:
//	ZQ::common::Log& _lipcLog;
	ZQ::common::LogWrapper& _lipcLog;

private:
	std::string		_buf;

	typedef struct _AsyncMessage{

		std::string msg;
		int fd;

		_AsyncMessage(){
			fd = -1;
		}
	} AsyncMessage;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<AsyncMessage> _SendMsgList;
	AsyncSender* _async;

//	ZQ::common::Mutex _lkRecvMsgList;
//	std::list<std::string> _RecvMsgList;

};

}} // namespaces

#endif // __ZQ_COMMON_ELOOP_UnixSocket_H__
