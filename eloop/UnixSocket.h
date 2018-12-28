#ifndef __ZQ_COMMON_ELOOP_UnixSocket_H__
#define __ZQ_COMMON_ELOOP_UnixSocket_H__

#include "FileLog.h"
#include "json/json.h"
#include "eloop_file.h"


#include <list>
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API UnixSocket;

#define FLG_TRACE               FLAG(0)
#define FLG_INFO                FLAG(1)

class ZQ_ELOOP_API Waker;

// -----------------------------
// class UnixSocket
// -----------------------------
class UnixSocket : public ZQ::eloop::Pipe
{
	friend class Waker;
public:
	typedef enum
	{
		lipcParseError = -4396
	} LipcError;

public:

	UnixSocket(Loop& loop, ZQ::common::LogWrapper& log, int ipc=1);
	virtual ~UnixSocket();

	// int  init(Loop &loop, int ipc=1);
	void closeUnixSocket();

	virtual void OnRead(ssize_t nread, const char *buf);

	// called after buffer has been written into the stream
	virtual void OnWrote(int status);

	virtual void onError( int error,const char* errorDescription ){}
	
	virtual void OnMessage(std::string& msg){}
	virtual int send(const std::string& msg, int fd = -1);
	
	void encode(const std::string& src,std::string& dest);

//	void processMessage2(ssize_t nread, const char *buf);

	void processMessage(ssize_t nread, const char *buf);

// 	void parseMessage(ssize_t nread, const char *buf);
// 	void AsyncProcessMessage();

	int AsyncSend(const std::string& msg, int fd = -1);

	virtual void OnWakedUp();
	// void OnCloseAsync();

//protected:
//	ZQ::common::Log& _lipcLog;
	ZQ::common::LogWrapper& _lipcLog;
	static uint32 _verboseFlags;
	void    setVerbosity(uint32 verbose = (0 | ZQ::common::Log::L_ERROR)) { _verboseFlags = verbose; }

private:
	std::string		_buf;

	typedef struct _Message{

		std::string msg;
		int fd;

		_Message(){
			fd = -1;
		}
	} Message;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<Message> _outgoings;
	Waker* _waker;

//	ZQ::common::Mutex _lkRecvMsgList;
//	std::list<std::string> _RecvMsgList;

};

}} // namespaces

#endif // __ZQ_COMMON_ELOOP_UnixSocket_H__
