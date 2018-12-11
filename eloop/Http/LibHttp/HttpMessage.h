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
#include <vector>
#include <functional>

namespace ZQ {
namespace eloop {

// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
class HttpMessage : public ZQ::common::SharedObject
{
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
		PUT			= HTTP_PUT,
		OPTIONS			= HTTP_OPTIONS
	} HttpMethod;

	enum Encoding
	{
		None, // raw data
		Form_Urlencoded,
		Form_Multipart
	};

	enum StatusCode
	{
		scContinue                       =100,
		scSwitchingProtocols             =101,
		scOK                             =200,
		scCreated                        =201,
		scAccepted                       =202,
		scNonAuthoritativeInformation    =203,
		scNoContent                      =204,
		scResetContent                   =205,
		scPartialContent                 =206,
		scMultipleChoices                =300,
		scMovedPermanently               =301,
		scFound                          =302,
		scSeeOther                       =303,
		scNotModified                    =304,
		scUseProxy                       =305,
		scTemporaryRedirect              =307,
		scBadRequest                     =400,
		scUnauthorized                   =401,
		scPaymentRequired                =402,
		scForbidden                      =403,
		scNotFound                       =404,
		scMethodNotAllowed               =405,
		scNotAcceptable                  =406,
		scProxyAuthenticationRequired    =407,
		scRequestTimeout                 =408,
		scConflict                       =409,
		scGone                           =410,
		scLengthRequired                 =411,
		scPreconditionFailed             =412,
		scRequestEntityTooLarge          =413,
		scRequestURITooLong              =414,
		scUnsupportedMediaType           =415,
		scRequestedRangeNotSatisfiable   =416,
		scExpectationFailed              =417,
		scInternalServerError            =500,
		scNotImplemented                 =501,
		scBadGateway                     =502,
		scServiceUnavailable             =503,
		scGatewayTimeout                 =504,
		scHTTPVersionNotSupported        =505,
	};

	typedef enum _errorCode{

		BoundaryisNULL = 1000,
		BodyDecodedError  = 1001,
		BodyPartError = 1002,
		BodyEndError = 1003,
		Unkown  = 2000
	} ErrorCode;

public:
	HttpMessage(MessgeType type);
	virtual ~HttpMessage();

	static const char* code2status(int statusCode);
	static const char* errorCode2Desc(int err);
	static std::string httpdate( int deltaInSecond = 0 );
	static std::string uint2hex(unsigned long u, size_t alignLen = 0, char paddingChar = '0');

	HttpMethod  method() const;
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

	struct caseInsensativeCmp
	{
		bool operator()( const std::string& lhr, const std::string& rhs) const {
#ifdef ZQ_OS_LINUX
#	define stricmp strcasecmp
#endif
			return stricmp(lhr.c_str(),rhs.c_str()) < 0;
		}
	};

	typedef std::map<std::string, std::string, caseInsensativeCmp> Headers;
	const std::string& header( const std::string& key) const;
	const Headers headers() const { return _Headers; }

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

	inline unsigned int versionMajor() const { return _VerMajor; }
	inline unsigned int versionMinor() const { return _VerMinor; }

	void setVersion( unsigned int major, unsigned int minor)
	{
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
	Headers				_Headers;
	std::map<std::string, std::string> _argument;
	std::string			_DummyHeaderValue;
	int				    _BodyLength;  // only valid if _bChunked == false
	std::string 		_simpleBody;  // valid only for response
	Encoding			_encoding;
	std::string			_boundary;    // for multipart/form-data only
	std::string			_buf;
};

} }//namespace ZQ::eloop
#endif // __HTTP_MESSAGE_h__

