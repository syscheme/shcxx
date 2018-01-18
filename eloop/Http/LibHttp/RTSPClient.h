#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__

#include "RTSPConnection.h"

namespace ZQ {
namespace eloop {

// ---------------------------------------
// class RTSPClient
// ---------------------------------------
class RTSPClient : public RTSPConnection 
{
public:
	typedef enum
	{
		RtspNoConnection	    = -10000,
		RtspReqTimeout			= -10001
	} Error;
public:
	RTSPClient(ZQ::common::Log& logger,const char* serverIp,uint serverPort,int64 timeout =500)
		:RTSPConnection(logger), _peerIp(serverIp), _peerPort(serverPort), _isConnected(false),_timeout(timeout) { _lastCSeq.set(1); }
	virtual void OnConnected(ElpeError status);

protected: // impl of RTSPParseSink
	virtual void OnResponses(RTSPMessage::MsgVec& responses){}
	virtual void OnRequests(RTSPMessage::MsgVec& requests){}

	virtual void OnRequestPrepared(RTSPMessage::Ptr req) {}
	virtual void OnRequestDone(int cseq, int ret) {}


private:

	int sendRequest(RTSPMessage::Ptr req, int64 timeout = 500);

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

	const std::string&		_peerIp;
	int						_peerPort;

	bool		_isConnected;
};

} }//namespace ZQ::eloop
#endif