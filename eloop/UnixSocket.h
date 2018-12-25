#ifndef __ZQ_COMMON_ELOOP_UnixSocket_H__
#define __ZQ_COMMON_ELOOP_UnixSocket_H__

#include "FileLog.h"
#include "json/json.h"
#include "eloop_file.h"


#include <list>
namespace ZQ {
namespace eloop {

class ZQ_ELOOP_API UnixSocket;
<<<<<<< HEAD

=======
class AsyncSender;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
// -----------------------------
// class UnixSocket
// -----------------------------
class UnixSocket : public ZQ::eloop::Pipe
{
<<<<<<< HEAD
	friend class Waker;
=======
	friend class AsyncSender;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
public:
	typedef enum
	{
		lipcParseError = -4396
	} LipcError;

public:

<<<<<<< HEAD
	UnixSocket(Loop& loop, ZQ::common::LogWrapper& log, int ipc=1);
	virtual ~UnixSocket();

	// int  init(Loop &loop, int ipc=1);
=======
	UnixSocket(ZQ::common::LogWrapper& log):_lipcLog(log),_async(NULL)
	{
		#ifdef ZQ_OS_LINUX
				//Ignore SIGPIPE signal
				signal(SIGPIPE, SIG_IGN);
		#endif
	}

	virtual ~UnixSocket()
	{
		ZQ::common::MutexGuard gd(_lkSendMsgList);
		_SendMsgList.clear();
	}

	int  init(Loop &loop, int ipc=1);
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
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

<<<<<<< HEAD
	virtual void OnWakedUp();
	// void OnCloseAsync();
=======
	void OnAsyncSend();
	void OnCloseAsync();
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

//protected:
//	ZQ::common::Log& _lipcLog;
	ZQ::common::LogWrapper& _lipcLog;
	static uint32 _verboseFlags;
	void    setVerbosity(uint32 verbose = (0 | ZQ::common::Log::L_ERROR)) { _verboseFlags = verbose; }

private:
	std::string		_buf;

<<<<<<< HEAD
	typedef struct _Message{
=======
	typedef struct _AsyncMessage{
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

		std::string msg;
		int fd;

<<<<<<< HEAD
		_Message(){
			fd = -1;
		}
	} Message;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<Message> _outgoings;
	Waker* _waker;
=======
		_AsyncMessage(){
			fd = -1;
		}
	} AsyncMessage;

	ZQ::common::Mutex _lkSendMsgList;
	std::list<AsyncMessage> _SendMsgList;
	AsyncSender* _async;
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

//	ZQ::common::Mutex _lkRecvMsgList;
//	std::list<std::string> _RecvMsgList;

};

}} // namespaces

#endif // __ZQ_COMMON_ELOOP_UnixSocket_H__
