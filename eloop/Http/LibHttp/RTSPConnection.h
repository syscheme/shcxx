#ifndef __RTSP_Connection_H__
#define __RTSP_Connection_H__

#include "Pointer.h"
#include "TCPServer.h"
#include "TimeUtil.h"

#include <sstream>
#include <string>
#include <map>
#include <vector>

namespace ZQ {
namespace eloop {

//-------------------------------------
//	class RTSPMessage
//-------------------------------------
class RTSPMessage : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPMessage> Ptr;

	typedef std::vector<Ptr> MsgVec;

	static bool less(Ptr i, Ptr j) {  return (i->cSeq() < j->cSeq()); }

	typedef enum _RTSPMessgeType { 
		RTSP_MSG_REQUEST = 0,
		RTSP_MSG_RESPONSE = 1
	} RTSPMessgeType;

	typedef std::map<std::string, std::string> Properties;
	typedef Properties Headers;

public:
	RTSPMessage(RTSPMessgeType type = RTSP_MSG_REQUEST):_msgType(type),_cSeq(-1),_bodyLen(0),_stampCreated(ZQ::common::now())
	{
	}

	virtual ~RTSPMessage() {}

	static std::string date( int deltaInSecond = 0 );
	static const std::string& code2status(int code);

	const std::string& header( const std::string& key) const;

	template<typename T>
	void header( const std::string& key, const T& value)
	{
		std::ostringstream oss;
		oss<<value;
		ZQ::common::MutexGuard gd(_lockHeaders);
		_headers[key] = oss.str();
	}

	const std::string& version() const { return _protocolVersion; }
	void version(const std::string& version) { _protocolVersion = version; }

	const std::string&	method() const { return _method; }
	void method(const std::string& method) { _method = method; }

	const std::string& url() const { return _url; }
	void url(const std::string& url) { _url = url; }

	void code( int c) { _statusCode = c; }	

	int	code() const { return _statusCode; }	

	void status(const std::string& st) { _statusDesc = st; }

	const std::string& status() const { return _statusDesc; }

	int64	contentLength() const { return _bodyLen; }
	void	contentLength(uint64 length) { _bodyLen = length; } //set content-length

	void appendBody(const char* body, size_t len) {  _contentBody.append(body,len); }

	uint32	cSeq() const { return _cSeq; }
	void	cSeq(uint32 cSeq) { _cSeq = cSeq; }

	RTSPMessgeType getMsgType() const { return _msgType; }
	void setMsgType(RTSPMessgeType type) { _msgType = type; }

	std::string toRaw();

private:

	ZQ::common::Mutex	_lockHeaders;
	Headers				_headers;
	std::string			_dummyVal;

	uint64				_bodyLen;
	std::string			_contentBody;
	uint32				_cSeq;
	RTSPMessgeType		_msgType;
	int64				_stampCreated;
	std::string 		_RawMessage;

	std::string			_statusDesc;
	int					_statusCode;//status code

	std::string			_method;
	std::string			_url;
	std::string			_protocolVersion;

};

#ifndef SYS_PROP
#  define SYS_PROP(_PROP)  ("sys." #_PROP)
#endif // SYS_PROP

// ---------------------------------------
// base RTSPSession
// ---------------------------------------
class RTSPSession : public virtual ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPSession> Ptr;
	typedef RTSPMessage::Properties Properties;

public:
	RTSPSession(const std::string& id): _id(id) {}
	virtual ~RTSPSession() {}

	const std::string& id() const { return _id; }
	std::string streamsInfo()   { return _props[SYS_PROP(streamKey)]; }
	Properties properities() const { return _props; }

/* TODO: decalare the common part of both server-side and client-side session
	// destroy would withdraw this session from background session manager, further leaving of Ptr would lead the deletion of this session
	virtual void destroy();

	/// set the destination where the stream should be pumped to
	///@destUrl the URL sepecifies the destination to pump to
	bool setStreamDestination(const char* destUrl, bool validate=true);

	/// reserved
	void getSessionRange(double& startTime, double& endTime) const
		{ startTime = _maxPlayStartTime; endTime = _maxPlayEndTime; }

	/// reserved
	void setSessionRange(double startTime, double endTime)
		{ _maxPlayStartTime =startTime; _maxPlayEndTime =endTime; }

	///@ return the scale of session
	virtual float& scale() { return _scale; }

	//const char* mediaSessionType() const { return _mediaSessionType.c_str(); }
	const char* sessionName() const { return _sessionName.c_str(); }
	const char* controlUri() const { return _controlUri.c_str(); }
	const char* guid() const { return _sessGuid.c_str(); }
	int64 getStampSetup() { return _stampSetup; };

	virtual const char* sessionDescription(bool generate=false, const char* serverType =NULL);
	virtual const char* getTransport(bool generate=false, const char* serverType =NULL, bool streamOutgoing =false);

	//void  setSessionGroup(const std::string& SessionGroup) { _sessGroup = SessionGroup; }
	//const std::string& getSessionGroup() const { return _sessGroup; }
*/

private:
	std::string		_id;
	Properties  	_props;
	// std::string		_sessGroup;
};

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