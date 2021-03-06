#ifndef __RTSP_Connection_H__
#define __RTSP_Connection_H__

#include "Pointer.h"
#include "TCPServer.h"

#include <sstream>
#include <string>
#include <map>
#include <vector>

namespace ZQ {
namespace eloop {

#define RTSP_RET_SUCC(_RET) (_RET>=200 && _RET <300)

#define LINE_TERM					"\r\n"

#define	Header_CSeq					"CSeq"
#define Header_Server				"Server"
#define	Header_Session				"Session"
#define	Header_Transport			"Transport"

#define Header_UserAgent			"User-Agent"
#define Header_ContentType			"Content-Type"
#define Header_ContentLength		"Content-Length"
#define Header_Notice				"Notice"
#define Header_Date					"Date"
#define Header_Warning				"Warning"
// #define Header_Reason				"X-reason"
#define	Header_RequestId			"X-Request-ID"


#define Method_OPTIONS				"OPTIONS"
#define Method_ANNOUNCE				"ANNOUNCE"
#define Method_DESCRIBE				"DESCRIBE"
#define Method_GetParameter			"GET_PARAMETER"
#define Method_SetParameter			"SET_PARAMETER"
#define Method_SETUP				"SETUP"
#define Method_PLAY					"PLAY"
#define Method_PAUSE				"PAUSE"
#define Method_TEARDOWN				"TEARDOWN"

class ZQ_ELOOP_HTTP_API RTSPMessage;
class ZQ_ELOOP_HTTP_API RTSPSession;
class ZQ_ELOOP_HTTP_API RTSPConnection;

//-------------------------------------
//	class RTSPMessage
//-------------------------------------
class RTSPMessage : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPMessage> Ptr;
	typedef std::vector<Ptr> MsgVec;

	static bool less(Ptr i, Ptr j) {  return (i->cSeq() < j->cSeq()); }

	typedef enum _ExtendedErrCode
	{
		rcOK                    = 200,
		rcContinue				= 100,
		rcBadRequest            = 400,
		rcUnauthorized          = 401,
		rcForbidden 		    = 403,
		rcObjectNotFound        = 404,
		rcMethodNotAllowed      = 405,
		rcNotAcceptable  		= 406,
		rcRequestTimeout        = 408,
		rcBadParameter          = 451,
		rcNotEnoughBandwidth    = 453,
		rcSessNotFound			= 454,
		rcInvalidState			= 455,
		rcInvalidRange			= 457,
		rcInternalError			= 500,
		rcNotImplement			= 501,
		rcServiceUnavail		= 503,
		rcProcessTimeout        = 504,
		rcOptionNotSupport		= 551,

		// NGOD-compatible extensions
		rcNoResponse			    = 770,
		rcAssetNotFound			    = 771,
		rcSopNotAvail			    = 772,
		rcUnknownSopGroup		    = 773,
		rcUnkownSopnames		    = 774,
		rcNotEnoughVolBandwidth	    = 775,
		rcNotEnoughNetworkBandwidth = 776,
		rcInvalidRequest			= 777,

		// exec error code
		Err_BackendError   = -100,
		Err_ConnectionLost = -101,
		Err_InvalidParams  = -102,
		Err_SendFailed     = -103,
		Err_RequestTimeout = -104,

		// async handling
		Err_AsyncHandling  = -200,
	} ExtendedErrCode;

#define RTSP_SUCC(ErrCode) (ErrCode>=200 && ErrCode < 300)
#define RTSP_SUCC_PROCESS(ErrCode) (RTSP_SUCC(ErrCode) || (ZQ::eloop::RTSPMessage::Err_AsyncHandling == ErrCode))

	typedef enum _AnnounceCode
	{
		// TianShan extensions
		racStateChanged             = 8802,
		racScaleChanged             = 8801,

		// NGOD compatible codes
		racEndOfStream              = 2101,
		racTransition               = 2103,
		racBeginOfStream            = 2104,
		racPauseTimeout             = 2105,
		racTrickNoConstrained       = 2201,
		racTrickConstrained         = 2204,
		racItemSkipped              = 2205,
		racClientSessionTerminated  = 5402,
		racSessionInProgress        = 5700,
		racErrorReadingData         = 4400,
		racDownstreamFail           = 5401,
		racInternalServerError      = 5502,
		racBandwidthExceeded        = 5602,
		racServerResourceUnavail    = 5200,
		racStreamBwUnaval           = 6001,
		racDownstreamUreachable     = 6004,
		racUnableEncrpt             = 6005,

		racFakedItemStepped         = 8810, // TianShan spec defined 8803 as "ItemStepped", Gehua copied the idea
		                                    // but renumber it to 8810, which is a mess up
	} AnnounceCode;

	typedef enum _RTSPMessgeType { 
		RTSP_MSG_REQUEST = 0,
		RTSP_MSG_RESPONSE = 1
	} RTSPMessgeType;

	typedef enum _RequestMethod {
		mtdUNKNOWN,
		mtdSETUP,
		mtdPLAY,
		mtdPAUSE,
		mtdTEARDOWN,
		mtdGET_PARAMETER,
		mtdSET_PARAMETER,
		mtdDESCRIBE,
		mtdOPTIONS,
		mtdANNOUNCE, // this is a mimic
	} RequestMethod;

	static const char* methodToStr(RequestMethod method);
	static RequestMethod strToMethod(const char* method);

	typedef std::map<std::string, std::string> Properties;
	typedef Properties Headers;

	typedef struct _StrPair
	{
		std::string		key;
		std::string		value;
	} StrPair;
	typedef std::vector<StrPair>	StrPairVec;

public:
	RTSPMessage(const std::string& connId="", RTSPMessgeType type = RTSP_MSG_REQUEST);
	virtual ~RTSPMessage() {}

	static void splitStrPair(const std::string& strPairData, StrPairVec& outVec,const std::string& delimiter="\r\n");

	static std::string date( int deltaInSecond = 0 );
	static const std::string& code2status(int code);

	const std::string& header( const std::string& key) const;
	int elapsed() const;

	template<typename T>
	void header( const std::string& key, const T& value)
	{
		std::ostringstream oss;
		oss << value;
		std::string valstr = oss.str();
		ZQ::common::MutexGuard gd(_lockHeaders);
		if (valstr.empty())
			_headers.erase(key);
		else _headers[key] = valstr;
	}

	const std::string& version() const { return _rtspVersion; }
	void version(const std::string& version) { _rtspVersion = version; }

	RequestMethod method() const { return _method; }
	void method(RequestMethod method) { _method = method; }

	const std::string& url() const { return _url; }
	void url(const std::string& url) { _url = url; }

	void code( int c) { _statusCode = c; }	

	int	code() const { return _statusCode; }	

	void status(const std::string& st) { _statusDesc = st; }

	const std::string& status() const { return _statusDesc; }

	int  	contentLength() const { return _bodyLen; }
	void	contentLength(uint length) { _bodyLen = length; } //set content-length

	void appendBody(const char* body, size_t len)
	{
		if (len > 0)
		{
			_contentBody.append(body,len);  
			_bodyLen = (int)_contentBody.size(); 
		}
	}
	void setBody(const std::string& body);
	const std::string& body();

	uint32	cSeq() const { return _cSeq; }
	void	cSeq(uint32 cSeq) { _cSeq = cSeq; }

	RTSPMessgeType getMsgType() const { return _msgType; }
	void setMsgType(RTSPMessgeType type) { _msgType = type; }

	std::string toRaw();
	std::string getConnId(){ return _connId; }
	void setConnId(const std::string& connId) { _connId = connId; }

	int64				_stampCreated;

protected:
	std::string			_connId; // TODO: what if the connection is lost piror to response sending
	ZQ::common::Mutex	_lockHeaders;
	Headers				_headers;
	std::string			_dummyVal;

	uint				_bodyLen;
	std::string			_contentBody;
	uint32				_cSeq;
	RTSPMessgeType		_msgType;
	std::string 		_RawMessage;

	std::string			_statusDesc;
	int					_statusCode;//status code

	RequestMethod   	_method;
	std::string			_url;
	std::string			_rtspVersion; // 1.0 by default
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

	virtual void destroy() {}

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

protected:
	std::string		_id;
	Properties  	_props;
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
		ParseStartLineError		= -10000,
		RtspNoConnection	    = -10001,
		RtspReqTimeout			= -10002
	} Error;

	virtual ~RTSPConnection(){}

	int sendRequest(RTSPMessage::Ptr req, int64 timeout = 500,bool expectResp = true);

protected:
	RTSPConnection(InterruptibleLoop& loop, ZQ::common::Log& log, const char* connId = NULL, TCPServer* tcpServer = NULL)
		:TCPConnection(loop, log,connId,tcpServer){}

	virtual void OnConnected(ElpeError status);

	virtual void doAllocate(eloop_buf_t* buf, size_t suggested_size);

	virtual void OnRead(ssize_t nread, const char *buf);

	virtual void OnConnectionError( int error,const char* errorDescription ){}

protected: // impl of RTSPParseSink
	virtual void OnResponse(RTSPMessage::Ptr resp){}
	virtual void OnRequest(RTSPMessage::Ptr req){}

	virtual void OnRequestPrepared(RTSPMessage::Ptr req) {}
	virtual void OnRequestDone(int cseq, int ret) {}

	virtual void OnTimer();

private:
	typedef struct rtsp_parser_msg
	{
		std::string startLine;
		bool		headerCompleted;
		int64       contentBodyRead;
		RTSPMessage::Ptr pMsg;

		rtsp_parser_msg() { reset(); }

		void reset()
		{
			startLine.clear();
			headerCompleted = false;
			contentBodyRead = 0;
			pMsg = new RTSPMessage();
		}

	} RTSP_PARSER_MSG;

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
	static bool parseStartLine(const std::string& startLine, RTSPMessage::Ptr& pMsg);

private:
	RTSP_PARSER_MSG	 _currentParseMsg;
};

} }//namespace ZQ::eloop
#endif