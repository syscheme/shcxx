#ifndef __RTSP_Message_H__
#define __RTSP_Message_H__

#include <Locks.h>
#include <Pointer.h>
#include "TimeUtil.h"

#include <sstream>
#include <string>
#include <map>
#include <vector>


namespace ZQ {
namespace eloop {

#define CRLF "\r\n"
#define ChunkTail "0\r\n\r\n"
#define RTSPVersion "RTSP/1.0"

// typedef enum _RtspMethod
// {
// 	RTSP_ANNOUNCE_MTHD,
// 	RTSP_DESCRIBE_MTHD,
// 	RTSP_PLAY_MTHD,
// 	RTSP_RECORD_MTHD,
// 	RTSP_SETUP_MTHD,
// 	RTSP_TEARDOWN_MTHD,
// 	RTSP_PAUSE_MTHD,
// 
// 	RTSP_GET_PARAMETER_MTHD,
// 	RTSP_OPTIONS_MTHD,
// 	RTSP_REDIRECT_MTHD,
// 	RTSP_SET_PARAMETER_MTHD,
// 	RTSP_SET_PING_MTHD,
// 	RTSP_RESPONSE_MTHD,
// 	RTSP_UNKNOWN_MTHD        // 13
// } RtspMethod;
// 
// CharData(""),
// CharData("ANNOUNCE"),
// CharData("DESCRIBE"),
// CharData("PLAY"),
// CharData("RECORD"),
// CharData("SETUP"),
// CharData("TEARDOWN"),
// CharData("PAUSE"),
// 
// CharData("GET_PARAMETER"),
// CharData("OPTIONS"),
// CharData("REDIRECT"),
// CharData("SET_PARAMETER"),
// CharData("PING"),
// CharData("RESPONSE"),
// CharData("")


// struct RtspMethod2Str {
// 	RtspMethod		num;
// 	const char*		method;
// };
// 
// static const char *method_strings[] = {
// 
// 	{ RTSP_ANNOUNCE_MTHD, "ANNOUNCE" },
// 
// 	{ 200, "OK" },
// 	{ 201, "Created" },
// 
// 	"PLAY",
// 	"SETUP",
// };//static std::map<RtspMethod, std::string> RtspMethod2StrMap;



struct RtspCode2Desc {
	int 			code;
	const char*		desc;
};

static struct RtspCode2Desc RtspCode2Str[] = {
	{ 100, "Continue" },

	{ 200, "OK" },
	{ 201, "Created" },
	{ 250, "Low on Storage Space" },

	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Moved Temporarily" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },

	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Time-out" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Large" },
	{ 415, "Unsupported Media Type" },
	{ 451, "Parameter Not Understood" },
	{ 452, "Conference Not Found" },
	{ 453, "Not Enough Bandwidth" },
	{ 454, "Session Not Found" },
	{ 455, "Method Not Valid in This State" },
	{ 456, "Header Field Not Valid for Resource" },
	{ 457, "Invalid Range" },
	{ 458, "Parameter Is Read-Only" },
	{ 459, "Aggregate operation not allowed" },
	{ 460, "Only aggregate operation allowed" },
	{ 461, "Unsupported transport" },
	{ 462, "Destination unreachable" },

	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Time-out" },
	{ 505, "Rtsp Version not supported" },
	{ 551, "Option not supported" }
};

static std::map<int, std::string> RtspCode2StatusMap;

class RtspCode2StatusMapInit{
public:
	RtspCode2StatusMapInit() {
		size_t count = sizeof( RtspCode2Str ) / sizeof(RtspCode2Str[0]);
		for( size_t i = 0 ; i < count; i ++ ) {
			RtspCode2StatusMap[ RtspCode2Str[i].code ]  = RtspCode2Str[i].desc;
		}
	}
};

//-------------------------------------
//	class RTSPMessage
//-------------------------------------
class RTSPMessage : public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPMessage> Ptr;

	typedef std::vector<Ptr> MsgVec;

	typedef enum _RTSPMessgeType { 
		RTSP_MSG_REQUEST = 0,
		RTSP_MSG_RESPONSE = 1
	} RTSPMessgeType;

	typedef std::map<std::string,std::string> HEADERS;

public:
	RTSPMessage(RTSPMessgeType type = RTSP_MSG_REQUEST):_msgType(type),_cSeq(-1),_bodyLen(0),_stampCreated(ZQ::common::now())
	{

	}
	virtual ~RTSPMessage(){}

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
	HEADERS				_headers;
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

} }//namespace ZQ::eloop
#endif