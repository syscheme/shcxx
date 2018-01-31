#include "RTSPConnection.h"

#include <sstream>
#include <string>
#include <map>
#include <vector>

namespace ZQ {
namespace eloop {

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
RtspCode2StatusMapInit rtspcode2status;

std::string RTSPMessage::date( int delta ) {
	char buffer[64] = {0};

	static const char* DateStrWeekDay[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	static const char* DateStrMonth[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

	struct tm tnow;
	time_t time_now;
	time(&time_now);
	time_now += (time_t)delta;
#ifdef ZQ_OS_MSWIN   
	gmtime_s(&tnow, &time_now);
	struct tm* t = &tnow;
#elif defined ZQ_OS_LINUX
	struct tm* t = gmtime_r( &time_now, &tnow);		
#endif//ZQ_OS
	sprintf(buffer,"%3s, %02d %3s %04d %02d:%02d:%02d GMT", 
		DateStrWeekDay[t->tm_wday],	
		t->tm_mday,
		DateStrMonth[t->tm_mon],
		t->tm_year+1900,
		t->tm_hour,
		t->tm_min,
		t->tm_sec);

	return buffer;
}

const std::string& RTSPMessage::code2status(int code)
{
	std::map<int,std::string>::const_iterator it = RtspCode2StatusMap.find(code);
	if( it != RtspCode2StatusMap.end())
		return it->second;
	return "unkown";
}

const std::string& RTSPMessage::header( const std::string& key) const 
{
	ZQ::common::MutexGuard gd(_lockHeaders);
	HEADERS::const_iterator it = _headers.find(key);
	if( it == _headers.end())
		return _dummyVal;
	return it->second;
}

std::string RTSPMessage::toRaw() 
{
	std::ostringstream oss;
	static const std::string line_term = "\r\n";
	if( !_RawMessage.empty())
		return _RawMessage;

	if(_msgType != RTSP_MSG_RESPONSE ) {
		oss<< _method << " " << _url << " " << "RTSP/1.0"<< line_term;
	} else {

		if (_statusDesc.empty())
			_statusDesc = code2status(_statusCode);
		oss<<"RTSP/1.0" <<" " <<_statusCode<<" "<<_statusDesc<<line_term;
	}

	if(_bodyLen >= 0 ) {
		std::ostringstream ossBL;ossBL<<_bodyLen;
		_headers["Content-Length"] = ossBL.str();
	} else {
		_headers.erase("Content-Length");
	}

	_headers["Date"] = date();

	if (_cSeq > 0)
	{
		std::ostringstream ossBL;ossBL<<_cSeq;
		_headers["CSeq"] = ossBL.str();
	}

	HEADERS::const_iterator it = _headers.begin();
	for( ; it != _headers.end() ; it ++ ) {
		oss<<it->first<<": "<<it->second<<line_term;
	}
	oss<<line_term;
	if (!_contentBody.empty())
		oss<<_contentBody;
	_RawMessage = oss.str();
	return _RawMessage;	
}

} }//namespace ZQ::eloop