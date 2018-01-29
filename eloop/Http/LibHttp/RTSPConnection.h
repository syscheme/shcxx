#ifndef __RTSP_Connection_H__
#define __RTSP_Connection_H__

#include "RTSPMessage.h"
#include <Pointer.h>
#include "TCPServer.h"


namespace ZQ {
namespace eloop {

//-------------------------------------
//	class RTSPConnection
//-------------------------------------
class RTSPConnection : public TCPConnection
{
private:
	RTSPConnection(const RTSPConnection&);
	RTSPConnection& operator=( const RTSPConnection&);

public:
	typedef enum
	{
		RtspNoConnection	    = -10000,
		RtspReqTimeout			= -10001
	} Error;

	virtual ~RTSPConnection(){}

	int64 id() const
	{
		ZQ::common::MutexGuard g(_lkConnId);
		return _connId;
	}

	int sendRequest(RTSPMessage::Ptr req, int64 timeout = 500,bool expectResp = true);

protected:
	RTSPConnection(ZQ::common::Log& log, TCPServer* tcpServer = NULL)
		:TCPConnection(log,tcpServer),_byteSeen(0)
	{
		_lastCSeq.set(1);

		ZQ::common::MutexGuard g(_lkConnId);
		_connId++;
	}

	virtual void OnConnected(ElpeError status);

	virtual void doAllocate(eloop_buf_t* buf, size_t suggested_size);

	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(int status);

	virtual void onError( int error,const char* errorDescription ){}

	virtual void	onDataSent(size_t size){}
	virtual void	onDataReceived( size_t size ){}

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp){}
	virtual void OnRequest(RTSPMessage::Ptr req){}

	virtual void OnRequestPrepared(RTSPMessage::Ptr req) {}
	virtual void OnRequestDone(int cseq, int ret) {}

private:
	typedef struct rtsp_parser_msg
	{
		std::string startLine;
		bool		headerCompleted;
		uint64      contentBodyRead;
		RTSPMessage::Ptr pMsg;

		rtsp_parser_msg() { reset(); }

		void reset()
		{
			headerCompleted = false;
			contentBodyRead = 0;
			pMsg = new RTSPMessage();
		}

	} RTSP_PARSER_MSG;

	uint lastCSeq();
	ZQ::common::AtomicInt _lastCSeq;

	typedef struct
	{
		RTSPMessage::Ptr req;
		int64            expiration;
	} AwaitRequest;

	int64 _timeout;
	typedef std::map<uint, AwaitRequest> AwaitRequestMap;
	AwaitRequestMap _awaits;
	ZQ::common::Mutex _lkAwaits; // because sendRequest() is open for other threads out of eloop to call

	RTSPMessage::MsgVec		_reqList;


	void parse(ssize_t bytesRead);

	static std::string trim(char const* str);
	static char* nextLine(char* startOfLine, int maxByte); // this func may change the input chars of startOfLine

private:
	RTSP_PARSER_MSG	 _currentParseMsg;
	int				 _byteSeen;
	static int64	 _connId;
	static ZQ::common::Mutex _lkConnId;
};

} }//namespace ZQ::eloop
#endif