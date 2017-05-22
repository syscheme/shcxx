#ifndef __HTTP_MESSAGE_h__
#define __HTTP_MESSAGE_h__
#include "ZQ_common_conf.h"

#include <Locks.h>
#include <sstream>
#include <string>
#include <Pointer.h>
#include <map>
#include "eloop_lock.h"
#include "http_parser.h"
#include <boost/algorithm/string/trim.hpp>
#include <vector>
#include <functional>

namespace ZQ {
namespace eloop {

#define CRLF "\r\n"
struct HttpCode2Str {
	int 			code;
	const char*		status;
};

static struct HttpCode2Str httpc2s[] = {
	{100,"Continue"},
	{101,"Switching Protocols"},
	{200,"OK"},
	{201,"Created"},
	{202,"Accepted"},
	{203,"Non-Authoritative Information"},
	{204,"No Content"},
	{205,"Reset Content"},
	{206,"Partial Content"},
	{300,"Multiple Choices"},
	{301,"Moved Permanently"},
	{302,"Found"},
	{303,"See Other"},
	{304,"Not Modified"},
	{305,"Use Proxy"},
	{307,"Temporary Redirect"},
	{400,"Bad Request"},
	{401,"Unauthorized"},
	{402,"Payment Required"},
	{403,"Forbidden"},
	{404,"Not Found"},
	{405,"Method Not Allowed"},
	{406,"Not Acceptable"},
	{407,"Proxy Authentication Required"},
	{408,"Request Timeout"},
	{409,"Conflict"},
	{410,"Gone"},
	{411,"Length Required"},
	{412,"Precondition Failed"},
	{413,"Request Entity Too Large"},
	{414,"Request-URI Too Long"},
	{415,"Unsupported Media Type"},
	{416,"Requested Range Not Satisfiable"},
	{417,"Expectation Failed"},
	{500,"Internal Server Error"},
	{501,"Not Implemented"},
	{502,"Bad Gateway"},
	{503,"Service Unavailable"},
	{504,"Gateway Timeout"},
	{505,"HTTP Version Not Supported"},
};

static std::map<int, std::string> code2statusmap;
static std::string 				unknownstatus = "unkown";

class Code2StatusMapInitializer{
public:
	Code2StatusMapInitializer() {
		size_t count = sizeof( httpc2s ) / sizeof(httpc2s[0]);
		for( size_t i = 0 ; i < count; i ++ ) {
			code2statusmap[ httpc2s[i].code ]  = httpc2s[i].status;
		}
	}
};

static const char* httpDateStrWeekDay[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* httpDateStrMonth[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};




class DataStreamHelper
{
public:
	struct Data
	{
		const char* data;
		size_t size;
		Data():data(NULL), size(0){}
		void clear()
		{
			data = NULL;
			size = 0;
		}
	};
	struct SearchResult
	{
		Data released;
		Data prefix;
		Data locked;
		Data suffix;

		void clear()
		{
			released.clear();
			prefix.clear();
			locked.clear();
			suffix.clear();
		}
	};
public:
	void setTarget(const std::string& target);
	// reset the search state
	void reset();
	// the search result won't include null pointer unless the input data is null.
	// return true for reach the target
	bool search(const char* data, size_t len, SearchResult& result);
private:
	std::string _target;
	std::vector<int> _kmpNext;
	size_t _nLocked;
};

#define Append_Search_Result(d, sr) {\
	d.append(sr.released.data, sr.released.size);\
	d.append(sr.prefix.data, sr.prefix.size);\
}

class LineCache
{
public:
	LineCache();
	const char* getLine(const char* &data, size_t &len);

	void clear();
private:
	std::string _line;
	bool _got;
	DataStreamHelper _dsh;
};


// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
class HttpMessage : public ZQ::common::SharedObject{
public:
	typedef ZQ::common::Pointer<HttpMessage> Ptr;
	typedef enum _MessgeType { 
		MSG_REQUEST = HTTP_REQUEST, 
		MSG_RESPONSE = HTTP_RESPONSE, 
		MSG_BOTH = HTTP_BOTH 
	} MessgeType;

	typedef enum _HttpMethod{
		HTTPDELETE	= HTTP_DELETE,
		GET			= HTTP_GET,
		POST		= HTTP_POST,
		PUT			= HTTP_PUT
	}HttpMethod;

	enum Encoding
	{
		None, // raw data
		Form_Urlencoded,
		Form_Multipart
	};

	typedef enum _errorCode{

		BoundaryisNULL = 1000,
		BodyDecodedError  = 1001,
		BodyPartError = 1002,
		BodyEndError = 1003,
		Unkown  = 2000
	}ErrorCode;

public:
	HttpMessage(MessgeType type);
	virtual ~HttpMessage();


	static const std::string& code2status(int code);
	static const char* errorCode2Desc(int err);
	static std::string httpdate( int deltaInSecond = 0 );
	static std::string uint2hex(unsigned long u, size_t alignLen = 0, char paddingChar = '0');

	HttpMethod method() const;
	void		method(HttpMethod mtd);

	const std::string& url() const;
	void		url(const std::string& url);

	void	code( int c);	//set status code
	int		code() const;	//get status cde

	void	status( const std::string& s);	// set status description
	const std::string& status() const;		//get status description

	const std::string& queryArgument(const std::string& k);
	void queryArgument(const std::string& k, const std::string& v);
	std::map<std::string, std::string> queryArguments();

	const std::string& header( const std::string& key) const;

	template<typename T>
	void	header( const std::string& key, const T& value){
		std::ostringstream oss;
		oss<<value;
		ZQ::common::MutexGuard gd(_Locker);

		_Headers[key] = oss.str();
	}
	void	eraseHeader( const std::string& key );

	bool	keepAlive() const;	//check if keepAlive is set
	void	keepAlive( bool b);	//enable/disable keepAlive

	bool	chunked() const;	//check if chunked is set
	void	chunked(bool b);	//enable/disable chunked transfer-encoding,
	//enable chunked also disable Content-Length

	int64	contentLength() const;	//get Content-Length, 0 if chunked is enabled
	void	contentLength(int64 length); //set content-length, this behaviour supress chunked

	bool	hasContentBody( ) const {
		return chunked() || contentLength() > 0;
	}

	std::string toRaw(); // generate raw http message, only start line and headers are generated

	struct caseInsensativeCmp{
		bool operator()( const std::string& lhr, const std::string& rhs) const {
#ifdef ZQ_OS_LINUX
#	define stricmp strcasecmp
#endif
			return stricmp(lhr.c_str(),rhs.c_str()) < 0;
		}
	};
	typedef std::map<std::string,std::string,caseInsensativeCmp> HEADERS;

	inline unsigned int versionMajor() const { return _VerMajor; }
	inline unsigned int versionMinor() const { return _VerMinor; }

	void setVersion( unsigned int major, unsigned int minor) {
		_VerMajor = major;
		_VerMinor = minor;
	}

	int onHeadersComplete();
	int onBody(const char* data, size_t len);
	int onMessageComplete();

private:
	friend class HttpConnection;

	ZQ::common::Mutex	_Locker;
	http_parser_type	_Type;//request or response
	HttpMethod			_Method;
	std::string			_Uri;
	std::string			_Status;
	int					_Code;//status code
	bool				_bChunked;
	bool				_bKeepAlive;

	unsigned int		_VerMajor;
	unsigned int		_VerMinor;
	HEADERS				_Headers;
	std::map<std::string, std::string> _argument;
	std::string			_DummyHeaderValue;
	int64				_BodyLength;//only valid if _bChunked == false
	std::string 		_RawMessage;
	Encoding			_encoding;
	std::string			_boundary; // for multipart/form-data only
	std::string			_buf;

};

} }//namespace ZQ::eloop
#endif