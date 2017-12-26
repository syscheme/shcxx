#include <stdio.h>
#include <time.h>
#include "http_parser.h"
#include "HttpMessage.h"
#include "urlstr.h"

namespace ZQ {
namespace eloop {

#ifdef ZQ_OS_LINUX
#ifndef stricmp
#define	stricmp strcasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif
#endif

void split(std::vector<std::string>& v, const std::string& s, const std::string d = " ")
{
	v.clear();

	std::string::size_type pos_from = 0;
	while((pos_from = s.find_first_not_of(d, pos_from)) != std::string::npos)
	{
		std::string::size_type pos_to = s.find_first_of(d, pos_from);
		if(pos_to != std::string::npos)
		{
			v.push_back(s.substr(pos_from, pos_to - pos_from));
		}
		else
		{
			v.push_back(s.substr(pos_from));
			break;
		}

		pos_from = pos_to;
	}
}

std::string trim(const std::string& s, const std::string& d = " ")
{
	return boost::trim_copy_if(s, boost::is_any_of(d));
}

// implement the Knuth-Morris-Pratt algorithm
static void kmpPreprocess(const std::vector<char>& x, std::vector<int>& kmpNext)
{
	size_t m = x.size();
	kmpNext.clear();
	kmpNext.resize(m + 1);

	size_t i = 0;
	int j = kmpNext[0] = -1;

	while(i < m)
	{
		while(j > -1 && x[i] != x[j])
			j = kmpNext[j];
		++i;
		++j;

		if(i < m && x[i] == x[j])
			kmpNext[i] = kmpNext[j];
		else
			kmpNext[i] = j;
	}
}

void DataStreamHelper::setTarget(const std::string& target)
{
	_nLocked = 0;
	_target = target;
	kmpPreprocess(std::vector<char>(_target.begin(), _target.end()), _kmpNext);
}

// reset the search state
void DataStreamHelper::reset()
{
	_nLocked = 0;
}

// the search result won't include null pointer unless the input data is null.
// return true for reach the target
bool DataStreamHelper::search(const char* data, size_t len, SearchResult& result)
{
	int m = _target.size();
	int i = _nLocked;
	size_t j = 0;
	// kmp algorithm
	while(j < len)
	{
		while(i > -1 && _target[i] != data[j])
			i = _kmpNext[i];

		++i;
		++j;
		if(i >= m)
		{ // found
			break;
		}
	}

	// not found
	int vPos = _nLocked + j - i;
	if(vPos > (int)_nLocked)
	{ // all locked part released
		result.released.data = _target.data();
		result.released.size = _nLocked;
		result.prefix.data = data;
		result.prefix.size = j - i;
	}
	else
	{ // only first bytes of the locked part released
		result.released.data = _target.data();
		result.released.size = vPos;
		result.prefix.data = data;
		result.prefix.size = 0;
	}

	result.suffix.data = data + j;
	result.suffix.size = len - j;

	result.locked.data = _target.data();
	result.locked.size = i;

	if(i >= m)
	{ // found
		_nLocked = 0; // discard the locked part
		return true;
	}
	else
	{ // not found
		_nLocked = i;
		return false;
	}
}

LineCache::LineCache()
: _got(false)
{
	_dsh.setTarget("\r\n");
}
const char* LineCache::getLine(const char* &data, size_t &len)
{
	if(_got)
		return _line.c_str();

	DataStreamHelper::SearchResult sr;
	_got = _dsh.search(data, len, sr);
	Append_Search_Result(_line, sr);
	data = sr.suffix.data;
	len = sr.suffix.size;
	return _got ? _line.c_str() : NULL;
}

void LineCache::clear()
{
	_line.clear();
	_got = false;
	_dsh.reset();
}
// -----------------------------------------------------
// class HttpMessage
// -----------------------------------------------------
Code2StatusMapInitializer c2smapinitializer;
HttpMessage::HttpMessage(MessgeType type)
:_Type((http_parser_type) type), _Method(GET), _Code(0), _bChunked(false),
_bKeepAlive(false), _VerMajor(0), _VerMinor(0), _BodyLength(0)
{
}

HttpMessage::~HttpMessage(){

}

std::string HttpMessage::httpdate( int delta ) {
	char buffer[64] = {0};

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
		httpDateStrWeekDay[t->tm_wday],	
		t->tm_mday,
		httpDateStrMonth[t->tm_mon],
		t->tm_year+1900,
		t->tm_hour,
		t->tm_min,
		t->tm_sec);

	return buffer;
}

std::string HttpMessage::uint2hex(unsigned long u, size_t alignLen, char paddingChar)
{
	static char hexCharTbl[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};

	std::string ret;
	ret.reserve(8);

	while(u)
	{
		ret.insert(ret.begin(), hexCharTbl[u & 0xF]);
		u >>= 4;
	}

	if(ret.empty())
		return "0";

	while(ret.size() < alignLen)
		ret.insert(ret.begin(), paddingChar);

	return ret;
}

const std::string& HttpMessage::code2status( int code ) {
	std::map<int,std::string>::const_iterator it = code2statusmap.find(code);
	if( it != code2statusmap.end())
		return it->second;
	return unknownstatus;
}

const char* HttpMessage::errorCode2Desc(int err)
{
	switch(err)
	{
	case BoundaryisNULL:
		return "boundary is null";
	case BodyDecodedError:
		return "Bad post data that can't be decoded. encoding=application/x-www-form-urlencoded";
	case BodyPartError:
		return "Bad format of post data. Can't position the begin of the body part. encoding=multipart/form-data";
	case BodyEndError:
		return "Bad format of post data. Can't find the body end.";
	case Unkown:
	default:
		return "unkown";
	}
}

HttpMessage::HttpMethod HttpMessage::method() const
{
	return _Method;
}

void HttpMessage::method(HttpMethod mtd){
	_Method = mtd;
}

void HttpMessage::code( int c) {
	_Code = c;
}	

int	HttpMessage::code() const {
	return _Code;
}	

void HttpMessage::status( const std::string& s) {
	_Status = s;
}

const std::string& HttpMessage::status() const {
	return _Status;
}		

const std::string& HttpMessage::url() const {
	return _Uri;
}

void HttpMessage::url(const std::string& url) {
	_Uri = url;
}

const std::string& HttpMessage::queryArgument(const std::string& k)
{
	std::map<std::string, std::string>::const_iterator it = _argument.find(k);
	if (it == _argument.end())
		return _DummyHeaderValue;
	return it->second;
}

void HttpMessage::queryArgument(const std::string& k, const std::string& v)
{
	_argument[k] = v;
}

std::map<std::string, std::string> HttpMessage::queryArguments()
{
	return _argument;
}

int HttpMessage::onHeadersComplete()
{
	ZQ::common::URLStr decoder(NULL, true); // case sensitive
	if(decoder.parse(_Uri.c_str()))
	{
		_argument = decoder.getEnumVars();
	}
	// cut off the paramesters
	size_t pos = _Uri.find_first_of("?#");
	if (std::string::npos != pos)
		_Uri = _Uri.substr(0, pos);

	_encoding = None;
	HEADERS::iterator it = _Headers.begin();
	for(;it!=_Headers.end();it++)
	{
		if (strcmp(it->first.c_str(),"Content-Type") == 0)
		{
			if(strstr(it->second.c_str(), "application/x-www-form-urlencoded"))
				_encoding = Form_Urlencoded;
			else if(strstr(it->second.c_str(), "multipart/form-data"))
			{ // search for boundary
				_encoding = Form_Multipart;
				_boundary.clear();
				const char* boundary = strstr(it->second.c_str(), "boundary=");
				if(boundary)
					_boundary = ZQ::eloop::trim(boundary + 9); // 9 == strlen("boundary=")
				if(_boundary.empty())
					return BoundaryisNULL;
			}
			else
			{
				_encoding = None;
				return 0;
			}
		}
	}
	return 0;
}

int HttpMessage::onBody(const char* data, size_t len)
{
	switch(_encoding)
	{
	case Form_Urlencoded:
	case Form_Multipart:
		_buf.append(data, len);
		return 0;
	case None: // pass through
	default:
		return -1;
	}
}

int HttpMessage::onMessageComplete()
{
	switch(_encoding)
	{
	case Form_Urlencoded:
		{ // decode the post data in the buffer
			ZQ::common::URLStr decoder(NULL, true); // case sensitive
			if(decoder.parse(_buf.c_str()))
			{
				std::map<std::string, std::string> vars = decoder.getEnumVars();
				std::map<std::string, std::string>::const_iterator it;
				for(it = vars.begin(); it != vars.end(); ++it)
				{
					queryArgument(it->first,it->second);
				}
			}
			else
			{
				//_log(ZQ::common::Log::L_ERROR, PDPLOGFMT("Bad post data that can't be decoded. encoding=application/x-www-form-urlencoded"));
				//_resp->sendDefaultErrorPage(400);
				return BodyDecodedError;
			}
		}
		break;
	case Form_Multipart:
		{
			// processing the multipart content
			std::string dashBoundary = std::string("--") + _boundary;
			std::string delimeter = std::string("\r\n") + dashBoundary;
			const char* p = _buf.data(); // the current position
			const char* pEnd = _buf.data() + _buf.size(); // the end position
			p = std::search(p, pEnd, dashBoundary.begin(), dashBoundary.end());
			while(p < pEnd)
			{
				if(0 == memcmp(p + dashBoundary.size(), "--", 2)) // end
				{ // end of the multipart, normal quit
					break;
				}

				ZQ::eloop::LineCache lineReader;
				size_t len = pEnd - p;
				if(!lineReader.getLine(p, len))
				{ // con't find the line end
					//_log(ZQ::common::Log::L_WARNING, PDPLOGFMT("Bad format of post data. Can't position the begin of the body part. encoding=multipart/form-data"));
					//_resp->sendDefaultErrorPage(400);
					return BodyPartError;
				}
				lineReader.clear();

				std::string name, filename, contentType, transferEncoding;
				const char* hdr = NULL;
				while((hdr = lineReader.getLine(p, len)))
				{
					if('\0' == *hdr)
					{ // blank line, header end
						break;
					}
					else if(0 == strnicmp(hdr, "Content-disposition:", strlen("Content-disposition:")))
					{ // parse the content-disposition
						// get the name and file name
						std::vector<std::string> params;
						ZQ::eloop::split(params, hdr + strlen("Content-disposition:"), ";");
						// parameter's format: key=value
						for(size_t i = 0; i < params.size(); ++i)
						{
							std::string k, v;
							std::string::size_type pos = params[i].find('=');
							if(pos != std::string::npos)
							{
								k = ZQ::eloop::trim(params[i].substr(0, pos));
								v = ZQ::eloop::trim(params[i].substr(pos + 1), " \"");
							}
							if(0 == stricmp(k.c_str(), "name"))
							{
								name = v;
							}
							else if(0 == stricmp(k.c_str(), "filename"))
							{
								filename = v;
							}
						} // for end (parameter)
					}
					else if(0 == strnicmp(hdr, "Content-Type:", strlen("Content-Type:")))
					{
						contentType = ZQ::eloop::trim(hdr + strlen("Content-Type:"));
					}
					else if(0 == strnicmp(hdr, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")))
					{
						transferEncoding = ZQ::eloop::trim(hdr + strlen("Content-Transfer-Encoding:"));
					}
					else
					{ // ignore
					}
					lineReader.clear();
				}
				// find the end of the multi-part body
				const char* end = std::search(p, pEnd, delimeter.begin(), delimeter.end());
				if(end == pEnd)
				{ // can't find the body end
					//_log(ZQ::common::Log::L_WARNING, PDPLOGFMT("Bad format of post data. Can't find the body end."));
					//_resp->sendDefaultErrorPage(400);
					return BodyEndError;
				}
				queryArgument(name,p);
				p = end + 2; // point to the boundary, continue the next iteration
			} // while end (multi-part)
		}
		break;
	case None: // pass through
	default:
		break;
	}
	return 0;
}

const std::string& HttpMessage::header( const std::string& key) const {
	ZQ::common::MutexGuard gd(_Locker);
	HEADERS::const_iterator it = _Headers.find(key);
	if( it == _Headers.end())
		return _DummyHeaderValue;
	return it->second;
}

void HttpMessage::eraseHeader( const std::string& key ) {
	ZQ::common::MutexGuard gd(_Locker);
	_Headers.erase( key );
}

bool HttpMessage::keepAlive() const {
	return _bKeepAlive;
}	
void HttpMessage::keepAlive( bool b) {
	_bKeepAlive = b;		
}	

bool HttpMessage::chunked() const {
	return _bChunked;
}

void HttpMessage::chunked(bool b) {
	_bChunked = b;
	if(b){
		_BodyLength = -1;
	}
}	

int64 HttpMessage::contentLength() const {
	if(_bChunked)
		return 0;
	return _BodyLength;
}	
void HttpMessage::contentLength(int64 length) {
	if(length < 0)
		return;
	_bChunked = false;
	_BodyLength = length;
}

std::string HttpMessage::toRaw() 
{
	std::ostringstream oss;
	static const std::string line_term = "\r\n";
	ZQ::common::MutexGuard gd(_Locker);
	if( !_RawMessage.empty())
		return _RawMessage;
	if(_Type != HTTP_RESPONSE ) {
		oss<< http_method_str((http_method)_Method) << " " << _Uri << " " << "HTTP/1.1"<< line_term;
	} else {
		oss<<"HTTP/1.1" <<" " <<_Code<<" "<<_Status<<line_term;
	}
	if(_bChunked) {
		_Headers["Transfer-Encoding"] = "chunked";
		_Headers.erase("Content-Length");
	} else {
		_Headers.erase("Transfer-Encoding");
		if(_BodyLength >= 0 ) {
			std::ostringstream ossBL;ossBL<<_BodyLength;
			_Headers["Content-Length"] = ossBL.str();
		} else {
			_Headers.erase("Content-Length");
		}
	}

	if(_bKeepAlive) {
		_Headers["Connection"] = "keep-alive";
	} else {
		_Headers["Connection"] = "close";
	}
	HEADERS::const_iterator it = _Headers.begin();
	for( ; it != _Headers.end() ; it ++ ) {
		oss<<it->first<<": "<<it->second<<line_term;
	}
	oss<<line_term;
	_RawMessage = oss.str();
	return _RawMessage;	
}

} }//namespace ZQ::eloop
