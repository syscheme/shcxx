#include <ZQ_common_conf.h>
#include "HttpEngine.h"
#include "HttpUtil.h"
#include "selector.h"
#include <urlstr.h>
#include <Locks.h>
#include <boost/regex.hpp>
#include <sstream>
#include <SystemUtils.h>
#include <TimeUtil.h>
#ifdef ZQ_OS_LINUX
extern "C"
{
#include <sys/socket.h>
#include <arpa/inet.h>
}
#endif

namespace ZQHttp {

#ifdef ZQ_OS_LINUX
#ifndef stricmp
#define	stricmp strcasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif
#endif

static EngineStatistics _gEngineStatistics;
EngineStatistics& getEngineStatistics()
{
	return _gEngineStatistics;
}

#define DataBuffer_Block_Min 2048
static Method parseMethodString(const char* m)
{
	if(0 == strcmp(m, "GET"))
		return GET;
	else if(0 == strcmp(m, "POST"))
		return POST;
	else if(0 == strcmp(m,"PUT"))
		return PUT;
	else if(0 == strcmp(m,"DELETE"))
		return M_DELETE;
	else
		return UNKNOWN;
}

class Request: public IRequest
{
public:
	explicit Request(ZQ::common::Log& log)
		:_log(log)
	{
		reset();
	}

	virtual Method method() const {  return _method; }
	virtual const char* uri() const { return _uri.c_str(); }
	virtual const char* getFullUri() const { return _fulluri.c_str(); }
	virtual const char* version() const { return _version.c_str(); }

	virtual const char* queryArgument(const char* q) const
	{
		if(NULL == q || '\0' == *q)
			return NULL;
		StringMap::const_iterator it = _queryArgs.find(q);
		if(it != _queryArgs.end())
			return it->second.c_str();
		else
			return NULL;
	}

	virtual std::map<std::string, std::string> queryArguments() const
	{
		return _queryArgs;
	}

	virtual const char* header(const char* h) const
	{
		if(NULL == h || '\0' == *h)
			return NULL;

		ICaseStringMap::const_iterator it = _headers.find(h);
		if(it != _headers.end())
			return it->second.c_str();

		return NULL;
	}

public:
	void reset()
	{
		_method = UNKNOWN;
		_uri.clear();
		_queryArgs.clear();
		_headers.clear();
	}

	bool parseRequestLine(const char* rl)
	{
		reset();

		std::vector<std::string> v;
		Util::split(v, rl);
		if(v.size() != 3)
		{
			return false;
		}
		_fulluri = v[1];

		// method
		_method = parseMethodString(v[0].c_str());

		// request url
		// extract the uri and query arguments
		std::string::size_type qMarkPos = v[1].find('?');
		if(qMarkPos != std::string::npos)
		{
			_uri = v[1].substr(0, qMarkPos);
			// get the query arguments
			ZQ::common::URLStr urlParser(v[1].c_str(), true);
			_queryArgs = urlParser.getEnumVars();
		}
		else // no query argumets
		{
			_uri = v[1];
		}

		// HTTP version
		if(0 != strncmp(v[2].c_str(), "HTTP/", 5))
		{ // not HTTP request
			reset();
			return false;
		}
		else 
		{
			_version = v[2].substr(5);
		}
		return true;
	}

	bool parseHeaderLine(const char* hl)
	{
		const char* pColon = strchr(hl, ':');
		if(NULL == pColon)
			return false;

		std::string key = std::string(hl, pColon);
		std::string val = Util::trim(pColon + 1);
#pragma message(__MSGLOC__"TODO: merge the multi-entry headers")
		_headers[key] = val;
		return true;
	}

private:
	ZQ::common::Log& _log;

	Method _method;
	std::string _version;
	std::string _uri;
	std::string _fulluri;
	typedef std::map<std::string, std::string> StringMap;
	StringMap _queryArgs;

	typedef std::map<std::string, std::string, Util::ICaseLess> ICaseStringMap;
	ICaseStringMap _headers;
};

static std::string uint2hex(unsigned long u, size_t alignLen = 0, char paddingChar = '0')
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

class Dialog;
typedef ZQ::DataPostHouse::ObjectHandle<Dialog> DialogPtr;

#define CRLF "\r\n"
class tcpHoldonSettingGuard
{
public:
	tcpHoldonSettingGuard( ZQ::DataPostHouse::TcpCommunicatorSettings& setting)
		:mSettings(setting)
	{
		mSettings.holdon(true);
	}

	virtual ~tcpHoldonSettingGuard()
	{
		mSettings.holdon(false);
	}

private:	
	ZQ::DataPostHouse::TcpCommunicatorSettings& mSettings;
};

class Response: public IResponse
{
public:
	uint32 _flags;

	Response(ZQ::common::Log &log, DialogPtr dialog,Request& req, const std::string& svr = "ZQHttpE")
		:_log(log), _dialog(dialog), _svr(svr), _blocked(true),_req(req), _flags(0)
	{
		reset();
		enableMessageDump(true, false, "");
		_timeCreation= ZQ::common::now();
		_timeHeaderPrepared = 0;
		_timeBodyFlushed = 0;
	}

	bool init(ZQ::DataPostHouse::IDataCommunicatorPtr comm)
	{
		ZQ::common::MutexGuard guard(_lock);
		_comm = comm;
		mConnSetting.attachCommunicator(comm);
		//mConnSetting.nodelay(true);
		// new API for this
		// mConnSetting.setWriteBufSize( 128 * 1024 );
		return true;
	}

	void send100continue()
	{
		ZQ::common::MutexGuard guard(_lock);
		static const char* the100response = "HTTP/1.1 100 Continue\r\n\r\n";
		static const int len = (int)strlen(the100response);
		sendData(the100response, len);
	}

	virtual void setStatus(int statusCode, const char* reasonPhrase = NULL)
	{
		ZQ::common::MutexGuard guard(_lock);
		_statusCode = statusCode;
		_reasonPhrase = reasonPhrase ? reasonPhrase : "";
	}

	virtual void setHeader(const char* hdr, const char* val)
	{
		ZQ::common::MutexGuard guard(_lock);
		if(hdr)
		{
			if(val)
				_headers[hdr] = val;
			else
				_headers.erase(hdr);
		}
	}

	virtual bool headerPrepared();

	virtual bool addContent(const char* data, size_t len)
	{
		ZQ::common::MutexGuard guard(_lock);
		if(!_buf.empty())
		{ // send out the header first
			bool bSent = sendData(_buf.data(), _buf.size());
			if (!bSent)
				return false;
			//mConnSetting.nodelay(true);//flush the data to client
			resetBuffer(); // clear the buffer
		}

		if(NULL == data)
		{
			_lastError = "Bad data";
			return false;
		}
		else if (len == 0)
			return true;

		// new API for this
		//tcpHoldonSettingGuard holdonGuard(mConnSetting);

		if(_contentLen < 0) // chunked
		{ // chunk head
			if (!sendData(uint2hex(len, 4, '0') + CRLF))
				return false;
		}

		// send out data
		if(!sendData(data, len))
			return false;

		if(_contentLen < 0) // chunked
		{
			// chunk tail
			if(!sendData(CRLF, 2))
				return false;
		}

		return true;
	}

	virtual bool complete();
	virtual void sendDefaultErrorPage(int statusCode)
	{
		if(statusCode < 300 || 999 < statusCode)
		{ // not error status
			// 500
			statusCode = 500;
		}

		reset();
		setStatus(statusCode);
		std::ostringstream buf;
		buf << "<html><head><title>" << statusCode << "</title></head>"
			<< "<body><h1>" << statusCode << "</h1></body></html>";
		std::string content;
		content = buf.str();
		setHeader("Content-length", Util::int2str(content.size()).c_str());
		headerPrepared();
		addContent(content.data(), content.size());
		complete();
	}

	virtual const char* getLastError() const 
	{
		ZQ::common::MutexGuard guard(_lock);
		return _lastError.c_str();
	}

	bool completed() const
	{
		return _completed;
	}

	void setBlockingMode(bool b)
	{
		_blocked = b;
	}

	void reset()
	{
		ZQ::common::MutexGuard guard(_lock);
		setStatus(200);
		_headers.clear();
		resetBuffer();
		_contentLen = 0;

		_completed = false;
		_data.clear();
	}

	bool flush()
	{
		ZQ::common::MutexGuard guard(_lock);
		return flush_unsafe();
	}

	void enableMessageDump(bool textMode, bool outgoingMessage, const std::string& dumpHint)
	{
		_textDumpMode = textMode;
		_dumpOutgoingMessage = outgoingMessage;
		_outgoingMessageDumpHint = dumpHint;
	}

	virtual uint32 setFlags(uint32 flags, bool enable)
	{
		if (enable)
			_flags |= flags;
		else
			_flags &= ~flags;

		return _flags;
	}


private:
	// the unlock version of flush
	bool flush_unsafe()
	{
		if(_blocked)
		{
			_lastError = "Flush in blocked mode";
			return false;
		}

		while(!_data.empty())
		{
			if(doSend(_data.front().data(), _data.front().size()))
				_data.pop_front();
			else return false;
		}

		return true;
	}

private:
	static std::string currentDateString()
	{ // format: <Fri, 01 Oct 1999 21:11:54 GMT>
		static char* monthText[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		static char* dayText[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

		char buf[32];
		time_t tmt;
		struct tm* ptm;
		tmt = time(0);
		ptm = gmtime(&tmt);
		if(!ptm)
			return "";
		sprintf(buf, "%3s, %02d %3s %4d %02d:%02d:%02d GMT",
			dayText[ptm->tm_wday],
			ptm->tm_mday,
			monthText[ptm->tm_mon],
			ptm->tm_year+1900,
			ptm->tm_hour,
			ptm->tm_min,
			ptm->tm_sec);

		return buf;
	}

	static const char* defaultReasonPhrase(int statusCode)
	{
		static struct
		{
			int statusCode;
			const char* reasonPhrase;
		} rpTbl[] = 
		{
			{100, "Continue"},
			{200, "OK"},
			{400, "Bad Request"},
			{403, "Forbidden"},
			{404, "Not Found"},
			{405, "Method not Allowed"},
			{500, "Internal Server Error"},
			{501, "Not Implemented"}
		};

		for(size_t i = 0; i < sizeof(rpTbl) / sizeof(rpTbl[0]); ++i)
		{
			if(statusCode == rpTbl[i].statusCode)
				return rpTbl[i].reasonPhrase;
		}

		return NULL;
	}

	static bool constructStatusLine(std::string& statusLine, int statusCode, const char* reasonPhrase = NULL)
	{
		if(statusCode < 100 || 999 < statusCode)
			return false;

		reasonPhrase = (reasonPhrase && '\0' != *reasonPhrase) ? reasonPhrase : defaultReasonPhrase(statusCode);
		if(NULL == reasonPhrase || '\0' == *reasonPhrase)
		{
			switch(statusCode / 100)
			{
			case 1:
				reasonPhrase = "Informational";
				break;
			case 2:
				reasonPhrase = "Success";
				break;
			case 3:
				reasonPhrase = "Redirection";
				break;
			case 4:
				reasonPhrase = "Client Error";
				break;
			case 5:
				reasonPhrase = "Server Error";
				break;
			default:
				reasonPhrase = "Custom Status";
			}
		}

		statusLine.clear();
		// format:HTTP/1.1 xxx <reason phrase>
		statusLine.reserve(13 + strlen(reasonPhrase));
		statusLine += "HTTP/1.1 ";
		statusLine += Util::int2str(statusCode);
		statusLine += " ";
		statusLine += reasonPhrase;

		return true;
	}
	
	void resetBuffer()
	{
		_buf.clear();
		_buf.reserve(2 * 1024);
	}
	
	bool sendData(const char* data, size_t len)
	{
		// cache the data if need
		if(_blocked)
		{
			if(_data.empty() || _data.back().size() + len > _data.back().capacity())
			{
				_data.push_back(std::string());
				_data.back().reserve(MAX(DataBuffer_Block_Min, len));
			}
			_data.back().append(data, data + len);
			return true;
		}

		if(flush_unsafe())
			return doSend(data, len);

		return false;
	}

	bool sendData(const std::string& data)
	{
		return sendData(data.data(), data.size());
	}

	bool doSend(const char* data, size_t len)
	{
		size_t nSent = _comm->write(data, len);
		if(nSent == len)
		{
			_lastError.clear();
			if(_dumpOutgoingMessage)
				_log.hexDump(ZQ::common::Log::L_DEBUG, data, len, _outgoingMessageDumpHint.c_str(), _textDumpMode);

			return true;
		} 

		_lastError = "Write failed";
		_log.hexDump(ZQ::common::Log::L_ERROR, data, len, ("Write failed. " + _outgoingMessageDumpHint).c_str(), _textDumpMode);
		return false;
	}

private:
	ZQ::common::Log& _log;
	ZQ::DataPostHouse::IDataCommunicatorPtr _comm;
	DialogPtr _dialog;
	const std::string _svr;

	int _statusCode;
	std::string _reasonPhrase;
	typedef std::map<std::string, std::string, Util::ICaseLess> ICaseStringMap;
	ICaseStringMap _headers;

	std::string _buf;
	int _contentLen; // -1 for chunked data

	std::string _lastError;

	bool _completed;
	bool _blocked;
	std::list<std::string> _data;
	ZQ::common::Mutex _lock;
	ZQ::DataPostHouse::TcpCommunicatorSettings mConnSetting;

	bool _dumpOutgoingMessage;
	bool _textDumpMode;
	std::string _outgoingMessageDumpHint;

	int64 _timeCreation;
	int64 _timeHeaderPrepared;
	int64 _timeBodyFlushed;
	Request& _req;
};

bool Response::headerPrepared()
{
	ZQ::common::MutexGuard guard(_lock);
	// build headers buffer
	resetBuffer();
	// status line
	std::string statusLine;
	if(constructStatusLine(statusLine, _statusCode, _reasonPhrase.c_str()))
	{
		_buf.append(statusLine);
		_buf.append(CRLF);
	}
	else
	{
		_log(ZQ::common::Log::L_WARNING, CLOGFMT(Response, "Can't construct status line with status code %d"), _statusCode);
		_lastError = "Bad status code";
		return false;
	}

	// Date
	if(_headers.find("Date") == _headers.end())
	{
		_headers["Date"] = currentDateString();
	}

	// Server
	if(_headers.find("Server") == _headers.end())
	{
		_headers["Server"] = _svr;
	}

	// Cache-Control
	//_headers["Cache-Control"] = "no-cache";

	// Accept-Ranges
	_headers["Accept-Ranges"] = "bytes";

	// Content-Length & Transfer-Encoding
	{
		ICaseStringMap::const_iterator it = _headers.find("Content-Length");
		if(it != _headers.end())
		{
			_contentLen = atoi(it->second.c_str());
		}
		else
		{
			_contentLen = -1;
			_headers["Transfer-Encoding"] = "chunked";
		}
	}

	// custom headers
	ICaseStringMap::const_iterator it = _headers.begin();
	for(;it != _headers.end(); ++it)
	{
		_buf.append(it->first);
		_buf.append(": ");
		_buf.append(it->second);
		_buf.append(CRLF);
	}

	_buf.append(CRLF); // header end
	_timeHeaderPrepared = ZQ::common::now();
	return true;
}

// post data processor
class PostDataProcessor
{
public:
	PostDataProcessor(ZQ::common::Log& log);
	bool setup(const char* contentType, IRequestHandler* handler,  IResponse* resp, const std::string& sessionId);
	bool process(const char* data, size_t len);
	bool done();
	void reset();
private:
	enum Encoding
	{
		None, // raw data
		Form_Urlencoded,
		Form_Multipart
	};
private:
	ZQ::common::Log& _log;
	Encoding _encoding;
	std::string _boundary; // for multipart/form-data only
	std::string _buf;
#pragma message(__MSGLOC__"TODO: complete the implementation of the PostDataProcessor")
	IRequestHandler* _handler;
	IResponse* _resp;
	std::string _sessionId;
};

// connection info
static std::string getSocketAddressIpString(const sockaddr_storage& sa)
{
	if(!(sa.ss_family == AF_INET || sa.ss_family == AF_INET6))
	{ // not support
		return "";
	}

	char buf[64] = {0};
	size_t len = 64;

#if defined(ZQ_OS_MSWIN)
	if(0 == ::WSAAddressToString((LPSOCKADDR)&sa, sizeof(sockaddr_storage), NULL, buf, (LPDWORD)&len))
	{
		if(sa.ss_family == AF_INET)
		{ // IPv4
			// format= ip:port
			char* p = strchr(buf, ':');
			if(p)
			{
				*p = '\0';
				return buf;
			}
			else
			{
				return "";
			}
		}
		else
		{ // IPv6
			// format= [ip]:port
			// since the first charactor is always '[',
			// we need only position the ']' as the end
			char* p = strchr(buf, ']');
			if(p)
			{
				*p = '\0';
				return (buf + 1); // skip the '['
			}
			else
			{
				return "";
			}
		}
	}
	else
	{
		return "";
	}
#elif defined(ZQ_OS_LINUX)
	void* src = NULL;
	if(sa.ss_family == AF_INET)
		src = &(((sockaddr_in*)&sa)->sin_addr);
	else if(sa.ss_family == AF_INET6)
		src = &(((sockaddr_in6*)&sa)->sin6_addr);
	else
		return "";

	if(inet_ntop(sa.ss_family, src, buf, len))
	{
		return buf;
	}
	else
	{
		return "";
	}
#endif
}

static int getSocketAddressPort(const sockaddr_storage& sa)
{
	switch(sa.ss_family)
	{
	case AF_INET:
		return (unsigned long)ntohs(((const sockaddr_in&)sa).sin_port);
	case AF_INET6:
		return (unsigned long)ntohs(((const sockaddr_in6&)sa).sin6_port);
	default:
		return -1;
	}
}

static bool getSocketAddressEndpoint(const sockaddr_storage& sa, std::string& address, int& port)
{
	address = getSocketAddressIpString(sa);
	port = getSocketAddressPort(sa);
	return (!address.empty() && port > 0);
}

class Connection: public IConnection
{
public:
	Connection()
	{
		reset();
	}
	virtual ~Connection(){}

	bool init(ZQ::DataPostHouse::IDataCommunicatorPtr comm)
	{
		if(comm)
		{
			_commSettings.attachCommunicator(comm);
			_id = comm->getCommunicatorId();
			return comm->getCommunicatorAddrInfo(_local, _remote);
		}
		else
		{
			return false;
		}
	}

	virtual bool getLocalEndpoint(std::string& address, int& port) const
	{
		if(_local.addrLen != 0)
		{
			return getSocketAddressEndpoint(_local.u.storage, address, port);
		}
		else
		{
			return false;
		}
	}

	virtual bool  getRemoteEndpoint(std::string& address, int& port) const
	{
		if(_remote.addrLen != 0)
		{
			return getSocketAddressEndpoint(_remote.u.storage, address, port);
		}
		else
		{
			return false;
		}
	}

	virtual int64 getId() const {
		return _id;
	}

	virtual bool setCommOption(int opt, int val)
	{
		switch(opt)
		{
		case ZQHttp_OPT_NoDelay:
			return _commSettings.nodelay(val != 0);
		case ZQHttp_OPT_HoldOn:
			return _commSettings.holdon(val != 0);
		case ZQHttp_OPT_WriteBufSize:
			return _commSettings.setWriteBufSize(val);
		case ZQHttp_OPT_ReadBufSize:
			return _commSettings.setReadBufSize(val);
		case ZQHttp_OPT_sendTimeo:
			return _commSettings.setSendTimeout(val);
		default:
			return false;
		}
	}
	void reset()
	{
		_id = 0;
		memset(&_local, 0, sizeof(ZQ::DataPostHouse::CommAddr));
		memset(&_remote, 0, sizeof(ZQ::DataPostHouse::CommAddr));
	}
private:
	int64 _id;
	ZQ::DataPostHouse::CommAddr _local;
	ZQ::DataPostHouse::CommAddr _remote;
	ZQ::DataPostHouse::TcpCommunicatorSettings _commSettings;
};
struct DialogSession
{
	DialogSession(ZQ::common::Log& log, ZQ::DataPostHouse::IDataCommunicatorPtr comm, DialogPtr dialog)
		:req(log), resp(log, dialog,req), pdp(log) {
			restPDSize = 0;
			handler = NULL;
			resp.init(comm);
			char buf[64];
			sprintf(buf, "%p@%p@%lld", this, dialog.get(), comm->getCommunicatorId());
			id = buf;
	}
	// structured request&response data
	Request req;
	Response resp;

	// post data processing stuff
	size_t restPDSize; // rest post data size
	PostDataProcessor pdp;

	IRequestHandler* handler;

	std::string id; // the session id which is composed with session@dialog@conn
};
///////////////////////////////////////////////////////////////////////////
// Http Dialog
#define DIALOGFMT(_X) CLOGFMT(Dialog, "[%p@%lld]\t" _X), this, _conn.getId()
class Dialog: public ZQ::DataPostHouse::IDataDialog
{
public:
	Dialog(ZQ::common::Log& log, IRequestHandlerFactory& handlerFac, ZQ::common::NativeThreadPool& pool)
		:_log(log), _handlerFac(handlerFac), _pool(pool)
	{
		enableMessageDump(true, false, false);
		_state = Idle;
		_log(ZQ::common::Log::L_DEBUG,CLOGFMT(Dialog,"dialog constructed"));
	}
	virtual ~Dialog()
	{
		clear();
		ZQ::common::MutexGuard guard(_lock);
		if(!_sessList.empty()) {
			_log(ZQ::common::Log::L_INFO, CLOGFMT(Dialog, "[%p] ~Dialog() discard %d sessions on the connection"), this, _sessList.size());
			while(!_sessList.empty()) {
				DialogSession* sess = _sessList.back();
				_sessList.pop_back();
				if(sess->handler)
					_handlerFac.destroy(sess->handler);
				delete sess;
			}
		}
		_log(ZQ::common::Log::L_DEBUG,CLOGFMT(Dialog,"dialog destructed"));
	}
public:
	enum State
	{
		Idle = 0,
		RequestLine,
		Header,
		// the post data with content length
		PostData,

		// the chunked post data
		PostData_ChunkSize,
		PostData_ChunkBody,
		PostData_Trailer
	};
	static const char* showState(int st);
	void enableMessageDump(bool textMode, bool incomingMessage, bool outgoingMessage) {
		_textDumpMode = textMode;
		_dumpIncomingMessage = incomingMessage;
		_dumpOutgoingMessage = outgoingMessage;
	}
	virtual void onCommunicatorSetup(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
	{
		ZQ::common::MutexGuard guard(_lock);
		_comm = communicator;
		_conn.init(_comm);
		char buf[128];
		{ // hit log
			std::string clientIp;
			int clientPort = -1;
			if(_conn.getRemoteEndpoint(clientIp, clientPort))
			{
				sprintf(buf, "Recv[%lld|%s:%d]", _conn.getId(), clientIp.c_str(), clientPort);
				_incomingMessageDumpHint = buf;
				sprintf(buf, "Send[%lld|%s:%d]", _conn.getId(), clientIp.c_str(), clientPort);
				_outgoingMessageDumpHint = buf;
				_log(ZQ::common::Log::L_DEBUG, DIALOGFMT("connection(%lld) established from ip(%s) port(%d)"), _conn.getId(), clientIp.c_str(), clientPort);
			}
			else
			{
				sprintf(buf, "Recv[%lld]", _conn.getId());
				_incomingMessageDumpHint = buf;
				sprintf(buf, "Send[%lld]", _conn.getId());
				_outgoingMessageDumpHint = buf;
				_log(ZQ::common::Log::L_WARNING, DIALOGFMT("connection(%lld) established but endpoint info is not available"), _conn.getId());
			}
		}
	}
	virtual void onCommunicatorDestroyed(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
	{
		int64 connId = communicator ? communicator->getCommunicatorId() : 0;
		ZQ::common::MutexGuard guard(_lock);
		for(std::list<DialogSession*>::iterator it = _sessList.begin(); it != _sessList.end(); ++it) {
			IRequestHandler* handler = (*it)->handler;
			if(handler)
				handler->onBreak();
		}
		_comm = NULL;
		issueCleanupCommand();
		_log(ZQ::common::Log::L_DEBUG, DIALOGFMT("connection(%lld) closed"), connId);
	}

	virtual bool onRead(const int8* buffer, size_t bufSize)
	{
		if(NULL != buffer || bufSize > 0) {
			if(_dumpIncomingMessage) {
				_log.hexDump(ZQ::common::Log::L_DEBUG, buffer, bufSize, _incomingMessageDumpHint.c_str(), _textDumpMode);
			}
			ZQ::common::MutexGuard guard(_dataLock);
			if(_data.empty() || _data.back().size() + bufSize > _data.back().capacity()) {
				_data.push_back(std::string());
				_data.back().reserve(MAX(DataBuffer_Block_Min, bufSize));
			}
			_data.back().append(buffer, buffer + bufSize);
			issueDataPreparedCommand();
		}
		return true;
	}
	virtual void onWritten(size_t bufSize)
	{
	}
	virtual void onError()
	{
		ZQ::common::MutexGuard guard(_lock);
		_comm = NULL;
		issueCleanupCommand();
	}
	void processData() {
		ZQ::common::MutexGuard guard(_dataLock);
		while(!_data.empty()) {
			if(feed(_data.front().data(), _data.front().size())) {
				_data.pop_front();
				flush();
			} else {
				flush();
				clear();
				return;
			}
		}
	}
	void flush() {
		ZQ::common::MutexGuard guard(_lock);
		while(!_sessList.empty()) {
			DialogSession* sess = _sessList.front();
			sess->resp.setBlockingMode(false);
			sess->resp.flush();
			if(sess->resp.completed()) {
				_sessList.pop_front();
				_handlerFac.destroy(sess->handler);
				delete sess;
			} else {
				return;
			}
		}
	}
	void clear() {
		{
			ZQ::common::MutexGuard guard(_lock);
			if(_comm) {
				_log(ZQ::common::Log::L_INFO, DIALOGFMT("clear() close the connection"));
				_comm->close();
				_comm = NULL;
			}
			// discard the last session if need
			if(Idle != _state && !_sessList.empty()) {
				DialogSession* sess = _sessList.back();
				_sessList.pop_back();
				if(sess->handler)
					_handlerFac.destroy(sess->handler);
				delete sess;
			}
			if(!_sessList.empty()) {
				_log(ZQ::common::Log::L_INFO, DIALOGFMT("clear() %d active sessions on the connection"), _sessList.size());
			}
			_state = Idle;
			_lineReader.clear();
			_conn.reset();
		}
		{
			ZQ::common::MutexGuard guard(_dataLock);
			_data.clear();
		}
	}
	void issueDataPreparedCommand();
	void issueSessionCompletedCommand();
	void issueCleanupCommand();
private:
	bool feed(const char* data, size_t len);
private:
	ZQ::DataPostHouse::IDataCommunicatorPtr _comm;
private:
	ZQ::common::Log& _log;
	IRequestHandlerFactory& _handlerFac;
	ZQ::common::NativeThreadPool& _pool;

	ZQ::common::Mutex _lock;
	// header processing stuff
	State _state;
	Util::LineCache _lineReader;
	// connection descriptor
	Connection _conn;
	std::list<DialogSession*> _sessList;

	std::list< std::string > _data;
	ZQ::common::Mutex _dataLock;

	bool _dumpIncomingMessage;
	bool _dumpOutgoingMessage;
	bool _textDumpMode;
	std::string _incomingMessageDumpHint;
	std::string _outgoingMessageDumpHint;
};

bool Response::complete()
{
	bool bSent = false;
	ZQ::common::MutexGuard guard(_lock);
	if(!_buf.empty()) { // send out the header first
		bSent = sendData(_buf.data(), _buf.size());
		//if (bSent) {
			//mConnSetting.nodelay(true);//flush the data to client
			resetBuffer(); // clear the buffer
		//} else {
			//connection may be broken,
			// set _completed to true to let later procedure delete resources it gotten
			// do not return here, take failure as success
			//return false;
		//}
	} else {
		bSent = true;
	}
	_completed = true;
	if(_contentLen < 0 &&  0 == (rspflg_SkipZeroChunk & _flags) && bSent) // chunked
	{ // append last chunk
#define LAST_CHUNK "0" CRLF CRLF
#define LAST_CHUNK_LEN 5
		sendData(LAST_CHUNK, LAST_CHUNK_LEN);
	}
	_dialog->issueSessionCompletedCommand();
	_timeBodyFlushed = ZQ::common::now();

	getEngineStatistics().addCounter(_req.method(), _statusCode, 
		_timeHeaderPrepared-_timeCreation,
		_timeBodyFlushed-_timeCreation);
	return bSent;
}

// data is ready
class DataPreparedCommand: public ZQ::common::ThreadRequest {
public:
	DataPreparedCommand(ZQ::common::NativeThreadPool& pool, DialogPtr dialog)
		:ZQ::common::ThreadRequest(pool), dialog_(dialog) {
	}
	virtual bool init(void)	{ return true; };
	virtual int run(void) {
		dialog_->processData();
		return 0;
	}
	virtual void final(int retcode, bool bCancelled) {
		dialog_ = NULL;
		delete this;
	}
private:
	DialogPtr dialog_;
};
// the session is completed
class SessionCompletedCommand: public ZQ::common::ThreadRequest {
public:
	SessionCompletedCommand(ZQ::common::NativeThreadPool& pool, DialogPtr dialog)
		:ZQ::common::ThreadRequest(pool), dialog_(dialog) {
	}
	virtual bool init(void)	{ return true; };
	virtual int run(void) {
		dialog_->flush();
		return 0;
	}
	virtual void final(int retcode, bool bCancelled) {
		dialog_ = NULL;
		delete this;
	}
private:
	DialogPtr dialog_;
};

// cleanup
class CleanupCommand: public ZQ::common::ThreadRequest {
public:
	CleanupCommand(ZQ::common::NativeThreadPool& pool, DialogPtr dialog)
		:ZQ::common::ThreadRequest(pool), dialog_(dialog) {
	}
	virtual bool init(void)	{ return true; };
	virtual int run(void) {
		dialog_->clear();
		return 0;
	}
	virtual void final(int retcode, bool bCancelled) {
		dialog_ = NULL;
		delete this;
	}
private:
	DialogPtr dialog_;
};
const char* Dialog::showState(int st) {
	static const char* stateDescTbl[] = {
		"Idle",
		"RequestLine",
		"Header",
		"PostData",
		"PostData_ChunkSize",
		"PostData_ChunkBody",
		"PostData_Trailer",
		"UNKNOWN"
	};
	if((int)Idle <= st && st <= (int)PostData_Trailer) {
		return stateDescTbl[st];
	} else {
		return stateDescTbl[PostData_Trailer + 1];
	}
}
void Dialog::issueDataPreparedCommand() {
	(new DataPreparedCommand(_pool, this))->start();
}
void Dialog::issueSessionCompletedCommand() {
	(new SessionCompletedCommand(_pool, this))->start();
}
void Dialog::issueCleanupCommand() {
	(new CleanupCommand(_pool, this))->start();
}
bool Dialog::feed(const char* data, size_t len)
{
	if(NULL == data || len <= 0)
		return true; // no data feed? ignore.

	ZQ::common::MutexGuard guard(_lock);
	if(!_comm) {
		_log(ZQ::common::Log::L_INFO, DIALOGFMT("Data arrived but no communicator be provided"));
		return false;
	}
	if(_state != Idle && _sessList.empty()) {
		_log(ZQ::common::Log::L_WARNING, DIALOGFMT("No session instance in the %s(%d) state"), showState(_state), _state);
		return false;
	}
	DialogSession* CurrentSession = _sessList.empty() ? NULL : _sessList.back();
	// the PARSINGLOGFMT require the CurrentSession not NULL
#define PARSINGLOGFMT(_X) CLOGFMT(Dialog, "[%s]\t" _X), CurrentSession->id.c_str()
	while(len > 0)
	{
		switch(_state)
		{
		case Idle:
			{ // search for the request line
				_lineReader.clear();
				_state = RequestLine;
				DialogSession* newSession = new DialogSession(_log, _comm, this);
				newSession->resp.enableMessageDump(_textDumpMode, _dumpOutgoingMessage, _outgoingMessageDumpHint);
				_sessList.push_back(newSession);
				CurrentSession = _sessList.back(); // reset the current session
				if(_sessList.size() == 1) {
					CurrentSession->resp.setBlockingMode(false);
				}
			}
			break;
		case RequestLine:
			{
				const char* line = _lineReader.getLine(data, len);
				if(line)
				{
					// we SHOULD ignore the empty line(s)
					if('\0' == *line) {
						_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("ignore empty line where request line is expected"));
						_lineReader.clear(); // discard the empty line
						break;
					}
					// setup the request
					CurrentSession->resp.reset();
					CurrentSession->req.reset();
					if(CurrentSession->req.parseRequestLine(line))
					{ // select a request handler
						_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("new request arrived:%s"), line);
						if( strcmp(CurrentSession->req.version(), "1.1") != 0 ) {
							_log(ZQ::common::Log::L_WARNING, PARSINGLOGFMT("unsupported http version[%s]"),CurrentSession->req.version());
							CurrentSession->resp.sendDefaultErrorPage(505);
						}

						CurrentSession->handler = _handlerFac.create(CurrentSession->req.uri());
						if(CurrentSession->handler)
						{
							if(!CurrentSession->handler->onConnected(_conn))
							{
								_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("request refused after connected. uri=%s"), CurrentSession->req.uri());
								return false;
							}
							_state = Header;
						}
						else
						{ // 404
							_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("there's no request handler to process uri %s"), CurrentSession->req.uri());
							CurrentSession->resp.sendDefaultErrorPage(404);
							return false;
						}
					}
					else
					{ // not a valid request
						// log a warning and continue
						_log(ZQ::common::Log::L_WARNING, PARSINGLOGFMT("Invalid request line [%s]."), line);
						return false; // just close the connection
					}
					_lineReader.clear();
				}
				else
				{ // continue try get next line
				}
			}
			break;
		case Header:
			{
				const char* line = _lineReader.getLine(data, len);
				if(line)
				{ // get a header
					if('\0' == *line)
					{ // header end
						// 100-continue
						const char* expect = CurrentSession->req.header("Expect");
						if(expect && strstr(expect, "100-continue"))
						{ // send back 100 continue
							_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("send back 100-continue"));
							CurrentSession->resp.send100continue();
						}

						_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("request header is received."));
						if(!CurrentSession->handler->onRequest(CurrentSession->req, CurrentSession->resp))
						{
							_log(ZQ::common::Log::L_INFO, PARSINGLOGFMT("break the request processing after headers processed. uri=%s"), CurrentSession->req.uri());
							return false;
						}

						// check for post data
						if(POST == CurrentSession->req.method() || GET ==  CurrentSession->req.method() ||  PUT == CurrentSession->req.method() || M_DELETE == CurrentSession->req.method())
						{
							const char* contentType = CurrentSession->req.header("Content-Type");
							if(NULL == contentType || '\0' == *contentType)
							{ // we should guess the type from the content
								contentType = "application/octet-stream";
							}

							if(!CurrentSession->pdp.setup(contentType, CurrentSession->handler, &CurrentSession->resp, CurrentSession->id))
							{ // may the bad content type
								CurrentSession->resp.sendDefaultErrorPage(400);
								return false;
							}

							const char* pdSize =CurrentSession->req.header("Content-length");
							CurrentSession->restPDSize = pdSize ? atoi(pdSize) : 0;
							if(CurrentSession->restPDSize > 0)
							{ // with post data
								_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request with Content-length:%s"), pdSize);
								_state = PostData;
							}
							else
							{
								const char* transferEncoding = CurrentSession->req.header("Transfer-Encoding");
								if(transferEncoding && strstr(transferEncoding, "chunked"))
								{
									_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request with Transfer-Encoding:%s"), transferEncoding);
									_state = PostData_ChunkSize;
									CurrentSession->restPDSize = 0;
								}
								else
								{
									_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request is received completely."));
									// no post data? strange client behavior
									_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request with no post data."));
									CurrentSession->handler->onRequestEnd();
									_state = Idle;
								}
							}
						}
						else
						{ // process the request and continue
							_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("GET request is received completely."));
							CurrentSession->handler->onRequestEnd();
							_state = Idle;
						}
					}
					else
					{ // get a header
						// parse the header
						CurrentSession->req.parseHeaderLine(line);
					}
					_lineReader.clear();
				}
				else
				{ // continue try get a line
				}
			}
			break;
		case PostData:
			{ // receive the post data with content length
				if(CurrentSession->restPDSize <= len)
				{
					if(CurrentSession->pdp.process(data, CurrentSession->restPDSize))
					{
						data += CurrentSession->restPDSize;
						len -= CurrentSession->restPDSize;
						CurrentSession->restPDSize = 0;
						if(CurrentSession->pdp.done())
						{
							_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request is received completely."));
							CurrentSession->handler->onRequestEnd();
							_state = Idle;
						}
						else
						{ // can't process the post data correctly
							_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("can't process the post data correctly."));
							// disconnect from the client
							return false;
						}
					}
					else // something happened, need't continue
					{ // we need break the connection here
						_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("can't process the post data correctly."));
						return false;
					}
				}
				else
				{
					if(CurrentSession->pdp.process(data, len))
					{
					}
					else
					{
						_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("can't process the post data correctly."));
						return false;
					}
					CurrentSession->restPDSize -= len;
					return true; // all data processed
				}
			}
			break;
		case PostData_ChunkSize:
			{ // try get the chunk size
				const char* chunkSize = _lineReader.getLine(data, len);
				if(chunkSize)
				{ // parse the HEX string
					CurrentSession->restPDSize = strtoul(chunkSize, NULL, 16);
					if(CurrentSession->restPDSize == 0)
					{ // the last chunk
						_state = PostData_Trailer;
					}
					else if(CurrentSession->restPDSize == ULONG_MAX)
					{ // overflow, rare case
						_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("Chunk size overflow. chunksize=%s."), chunkSize);
						return false;
					}
					else
					{
						_state = PostData_ChunkBody;
					}
					_lineReader.clear();
				}
				else
				{ // noop
				}
			}
			break;
		case PostData_ChunkBody:
			{ // get the chunk body
				if(CurrentSession->restPDSize > 0)
				{ // get data
					if(CurrentSession->restPDSize <= len)
					{
						if(CurrentSession->pdp.process(data, CurrentSession->restPDSize))
						{
							data += CurrentSession->restPDSize;
							len -= CurrentSession->restPDSize;
							CurrentSession->restPDSize = 0;
							// need next cycle to skip the CRLF
							continue;
						}
						else // something happened, need't continue
						{ // we need break the connection here
							_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("can't process the post data correctly."));
							return false;
						}
					}
					else // _restPDSize > len
					{
						if(CurrentSession->pdp.process(data, len))
						{
						}
						else
						{
							_log(ZQ::common::Log::L_ERROR, PARSINGLOGFMT("can't process the post data correctly."));
							return false;
						}
						CurrentSession->restPDSize -= len;
						return true; // all data processed
					}
				}
				else // _restData == 0
				{// need skip the CRLF
					const char* blankLine = _lineReader.getLine(data, len);
					if(blankLine)
					{ // should we check the content of the line?
						if('\0' == *blankLine)
						{
							_state = PostData_ChunkSize;
						}
						else
						{
							_log(ZQ::common::Log::L_WARNING, PARSINGLOGFMT("Bad format of chunked post data. Get [%s] while expecting blank line."), blankLine);
#pragma message(__MSGLOC__"How to deal with the exception case against the normal sequence?")
							return false;
						}
						_lineReader.clear();
					}
				} // end if
			}
			break;
		case PostData_Trailer:
			{
				const char* line = _lineReader.getLine(data, len);
				while(line)
				{
					if('\0' == *line)
					{ // end of trailer
						_log(ZQ::common::Log::L_DEBUG, PARSINGLOGFMT("POST request is received completely."));
						CurrentSession->handler->onRequestEnd();
						_state = Idle;
					}
					else
					{ // not parse the tailer
					}
					_lineReader.clear();
					line = _lineReader.getLine(data, len);
				}
			}
			break;
		} // switch end
	} // while end
	return true;
}

class DialogFactory: public ZQ::DataPostHouse::IDataDialogFactory, public ZQ::common::NativeThread
{
public:
	DialogFactory(ZQ::common::Log& log, IRequestHandlerFactory& handlerFac, ZQ::common::NativeThreadPool& pool)
		:_log(log), _handlerFac(handlerFac), _pool(pool)
	{
		_maxConnections = 10240;
		_idleTimeoutMsec = 2 * 60 * 1000; // 2 min
		enableMessageDump(true, false, false);
	}

	virtual ~DialogFactory() 
	{
		stopIdleMonitor();
	}
public:
	void startIdleMonitor() {  start(); }
	void stopIdleMonitor()
	{
		if(!isRunning())
			return;

		_log(ZQ::common::Log::L_INFO, CLOGFMT(DialogFactory, "stopIdleMonitor() Notify the idle monitor thread to quit..."));
		_idleWaitEvent.signal();

		if (0 == waitHandle(10000))
			_log(ZQ::common::Log::L_INFO, CLOGFMT(DialogFactory, "stopIdleMonitor()The idle monitor thread quit successfully"));
		else
		{
			_log(ZQ::common::Log::L_ERROR, CLOGFMT(DialogFactory, "stopIdleMonitor()The idle monitor thread didn't quit in 10 sec. Terminate it."));
			terminate(0);
		}
	}

	void enableMessageDump(bool textMode, bool incomingMessage, bool outgoingMessage)
	{
		_textDumpMode = textMode;
		_dumpIncomingMesage = incomingMessage;
		_dumpOutgoingMessage = outgoingMessage;
	}

	void setMaxConnections(int maxConnections) { _maxConnections = maxConnections; }
	void setIdleTimeout(int idleTimeout) {  _idleTimeoutMsec = idleTimeout; }

public:
	virtual void onClose(CommunicatorS& comms)
	{
		for(CommunicatorS::iterator it = comms.begin(); it != comms.end(); ++it)
		{
			// break the dialog
			(*it)->close();
		}
	}

	virtual ZQ::DataPostHouse::IDataDialogPtr onCreateDataDialog(ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
	{
		{
			ZQ::common::MutexGuard guard(mCommLocker);
			if(_maxConnections <= mComms.size()) {
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(DialogFactory, "onCreateDataDialog() Exceed the max connections count. limit=%d"), _maxConnections);
				return NULL;
			}
		}
		Dialog* dialog = new Dialog(_log, _handlerFac, _pool);
		dialog->enableMessageDump(_textDumpMode, _dumpIncomingMesage, _dumpOutgoingMessage);
		return dialog;
	}

	virtual void onReleaseDataDialog(ZQ::DataPostHouse::IDataDialogPtr dialog, ZQ::DataPostHouse::IDataCommunicatorPtr communicator)
	{
		if( communicator)
			communicator->close();
	}

	// NativeThread
	virtual int run() {
		using namespace ZQ::DataPostHouse;
		_log(ZQ::common::Log::L_INFO, CLOGFMT(DialogFactory, "run() Enter connection idle monitor thread."));

		int32 idleScanInterval = _idleTimeoutMsec;
		while(SYS::SingleObject::TIMEDOUT == _idleWaitEvent.wait(idleScanInterval))
		{
			// poll the communicators
			std::vector<IDataCommunicatorPtr> idleComms;
			int32 maxIdleTime = 0; // exclude the idled connecions
			{
				ZQ::common::MutexGuard guard(mCommLocker);
				CommunicatorS::const_iterator it = mComms.begin();
				for(; it != mComms.end(); ++it)
				{
					int32 thisIdleTime = (*it)->getIdleTime();
					if ( thisIdleTime >= _idleTimeoutMsec)
					{
						// timeout
						idleComms.push_back(*it);
						continue;
					}

					// calc the max timeout
					maxIdleTime = thisIdleTime > maxIdleTime ? thisIdleTime : maxIdleTime;
				}
			}

			for (size_t i = 0; i < idleComms.size(); ++i)
			{
				IDataCommunicatorPtr& comm = idleComms[i];
				int64 connId = comm->getCommunicatorId();
				int32 idleTime = comm->getIdleTime();
				std::string remoteIp, remotePort, localIp, localPort;
				comm->getRemoteAddress(remoteIp, remotePort);
				comm->getLocalAddress(localIp, localPort);
				comm->close();
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(DialogFactory, "run() Connection [%lld | %s:%s -> %s:%s] idle for %dms, idle timeout and close"), connId, remoteIp.c_str(), remotePort.c_str(), localIp.c_str(), localPort.c_str(), idleTime);
			}

			idleScanInterval = _idleTimeoutMsec - maxIdleTime;

			if(idleScanInterval < 1000) // at least 1sec
				idleScanInterval = 1000;
		}

		_log(ZQ::common::Log::L_INFO, CLOGFMT(DialogFactory, "run() Leave connection idle monitor thread."));
		return 0;
	}

private:
	ZQ::common::Log& _log;
	IRequestHandlerFactory& _handlerFac;
	ZQ::common::NativeThreadPool& _pool;
	bool _dumpIncomingMesage;
	bool _dumpOutgoingMessage;
	bool _textDumpMode; // true for text dump, false for hex dump

	int32 _maxConnections;
	int32 _idleTimeoutMsec;
	SYS::SingleObject _idleWaitEvent;
};

// implement the post data processor
PostDataProcessor::PostDataProcessor(ZQ::common::Log& log)
:_log(log)
{
	reset();
}
#define PDPLOGFMT(_X) CLOGFMT(PostDataProcessor, "[%s]\t" _X), _sessionId.c_str()
bool PostDataProcessor::setup(const char* contentType, IRequestHandler* handler, IResponse* resp, const std::string& sessionId)
{
	if(NULL == contentType || '\0' == *contentType || NULL == handler || NULL == resp)
		return false;

	reset();
	_handler = handler;
	_resp = resp;
	_sessionId = sessionId;

	if(strstr(contentType, "application/x-www-form-urlencoded"))
	{
		_encoding = Form_Urlencoded;
		return true;
	}
	else if(strstr(contentType, "multipart/form-data"))
	{ // search for boundary
		_encoding = Form_Multipart;
		_boundary.clear();
		const char* boundary = strstr(contentType, "boundary=");
		if(boundary)
		{
			_boundary = Util::trim(boundary + 9); // 9 == strlen("boundary=")
		}

		if(_boundary.empty())
		{
			_log(ZQ::common::Log::L_ERROR, PDPLOGFMT("setup() need boundary string for content type multipart/form-data"));
			return false;
		}
		else
		{
			return true;
		}
	}
	else
	{
		_encoding = None;
		return true;
	}
}

bool PostDataProcessor::process(const char* data, size_t len)
{
	switch(_encoding)
	{
	case Form_Urlencoded:
		_buf.append(data, len);
		return true;
	case Form_Multipart:
		if(_boundary.empty())
		{
			return false;
		}
		else
		{
#pragma message(__MSGLOC__"TODO: improve the file upload logic")
			_buf.append(data, len);
			return true;
		}
	case None: // pass through
	default:
		{
			PostDataFrag frag;
			frag.data = data;
			frag.len = len;
			if(!_handler->onPostData(frag))
			{
				_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop processing the post data."));
				return false;
			}
			else
			{
				return true;
			}
		} // default end
	} // switch end
}

bool PostDataProcessor::done()
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
					PostDataFrag frag;
					frag.name = it->first.c_str();
					frag.len = it->second.size();
					frag.data = it->second.data();
					frag.partial = false;
					// other fields will be NULL by default
					if(!_handler->onPostData(frag))
					{
						_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop processing the post data."));
						return false;
					}
				}
				if(!_handler->onPostDataEnd())
				{
					_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop after the post data processed."));
					return false;
				}
			}
			else
			{
				_log(ZQ::common::Log::L_ERROR, PDPLOGFMT("Bad post data that can't be decoded. encoding=application/x-www-form-urlencoded"));
				_resp->sendDefaultErrorPage(400);
				return false;
			}
		}
		break;
	case Form_Multipart:
		{
#pragma message(__MSGLOC__"Should we care the quote-string boundary value?")
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

				Util::LineCache lineReader;
				size_t len = pEnd - p;
				if(!lineReader.getLine(p, len))
				{ // con't find the line end
					_log(ZQ::common::Log::L_WARNING, PDPLOGFMT("Bad format of post data. Can't position the begin of the body part. encoding=multipart/form-data"));
					_resp->sendDefaultErrorPage(400);
					return false;
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
						Util::split(params, hdr + strlen("Content-disposition:"), ";");
						// parameter's format: key=value
						for(size_t i = 0; i < params.size(); ++i)
						{
							std::string k, v;
							std::string::size_type pos = params[i].find('=');
							if(pos != std::string::npos)
							{
								k = Util::trim(params[i].substr(0, pos));
								v = Util::trim(params[i].substr(pos + 1), " \"");
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
						contentType = Util::trim(hdr + strlen("Content-Type:"));
					}
					else if(0 == strnicmp(hdr, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")))
					{
						transferEncoding = Util::trim(hdr + strlen("Content-Transfer-Encoding:"));
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
					_log(ZQ::common::Log::L_WARNING, PDPLOGFMT("Bad format of post data. Can't find the body end."));
					_resp->sendDefaultErrorPage(400);
					return false;
				}

				PostDataFrag frag;
				frag.name = name.c_str();
				frag.len = end - p;
				frag.data = p;
				frag.partial = false;
				frag.fileName = filename.c_str();
				frag.contentType = contentType.c_str();
				frag.transferEncoding = transferEncoding.c_str();
				if(!_handler->onPostData(frag))
				{
					_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop processing the post data."));
					return false;
				}

				p = end + 2; // point to the boundary, continue the next iteration
			} // while end (multi-part)
			if(!_handler->onPostDataEnd())
			{
				_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop after the post data processed."));
				return false;
			}
		}
		break;
	case None: // pass through
	default:
		{
			if(!_handler->onPostDataEnd())
			{
				_log(ZQ::common::Log::L_INFO, PDPLOGFMT("Stop after the post data processed."));
				return false;
			}
		}
		break;
	}
	return true;
}

void PostDataProcessor::reset()
{
	_sessionId.clear();
	_handler = NULL;
	_resp = NULL;
	_encoding = None;
	_boundary.clear();
	_buf.clear();
	_buf.reserve(16 * 1024);
}

// helper class
class PendingRejectHandler: public IRequestHandler {
public:
	PendingRejectHandler(ZQ::common::Log& log)
		:_log(log), _connId(-1), _resp(NULL), _nDiscardContent(0) {
	}
	virtual ~PendingRejectHandler() {}
	/// on*() return true for continue, false for break.
	virtual bool onConnected(IConnection& c) {
		_connId = c.getId();
		std::string ip;
		int port;
		if(c.getRemoteEndpoint(ip, port)) {
			_clientEndpoint = ip + ":" + Util::int2str(port);
		} else {
			_clientEndpoint.clear();
		}
		return true;
	}
	virtual bool onRequest(const IRequest& req, IResponse& resp) {
		_reqUri = req.uri();
		_resp = &resp;
		return true;
	}
	virtual bool onPostData(const PostDataFrag& frag) {
		if(frag.data) {
			_nDiscardContent += frag.len;
		}
		return true;
	}
	virtual bool onPostDataEnd() { return true; }
	virtual void onRequestEnd() {
		if(_resp) {
			_resp->setStatus(503, "Service Unavailable");
			_resp->setHeader("Content-Length", "0");
#if 0 // disable this header
			_resp->setHeader("TianShan-Notice", "exceed the max pending request limit");
#endif
			_resp->headerPrepared();
			_resp->complete();
			_log(ZQ::common::Log::L_INFO, CLOGFMT(PendingRejectHandler, "Reject the request(%s) connection id(%lld) client(%s) discard content data(%d)"), _reqUri.c_str(), _connId, _clientEndpoint.c_str(), _nDiscardContent);
		}
	}

	// break the current request processing
	virtual void onBreak() {};
private:
	ZQ::common::Log& _log;
	int64 _connId;
	std::string _clientEndpoint;
	std::string _reqUri;
	IResponse* _resp;
	int _nDiscardContent;
};
// request dispatcher
class RequestDispatcher: public IRequestHandlerFactory
{
public:
	explicit RequestDispatcher(ZQ::common::Log& log, ZQ::common::NativeThreadPool& pool)
		:_log(log), _pool(pool), _maxPendingRequest(-1)
	{
	}
	void setMaxPendingRequest(int n) {
		_maxPendingRequest = n;
	}
	virtual IRequestHandler* create(const char* uri)
	{
		if(NULL == uri)
		{
			return NULL;
		}

		IRequestHandler* handler = NULL;
		ZQ::common::MutexGuard sync(_lock);
		int nPending = _pool.pendingRequestSize();
		int activeCount = _pool.activeCount();
		if(_maxPendingRequest > 0) {
			if(nPending > 0) {
				_log((nPending > (_maxPendingRequest/2)) ? ZQ::common::Log::L_WARNING : ZQ::common::Log::L_DEBUG, CLOGFMT(RequestDispatcher, "create() busy with pending requests[%d/%d], active count[%d]"), nPending, _maxPendingRequest, activeCount);
			}
			if(nPending >= _maxPendingRequest) {
				_log(ZQ::common::Log::L_WARNING, CLOGFMT(RequestDispatcher, "create() rejecting request[%s] due to pending requests[%d] exceeded limition[%d], active count[%d]"), uri, nPending, _maxPendingRequest, activeCount);
				PendingRejectHandler* h = new PendingRejectHandler(_log);
				if(h) {
					_rejectedRequest.insert(h);
				}
				return h;
			}
		}
		IRequestHandlerFactory* fac = _selector.get(uri);
		if(fac)
		{
			handler = fac->create(uri);
			if(handler)
			{
				_currentRunningHandlers.push_back(std::make_pair(handler, fac));
			}
		}
		return handler;
	}

	virtual void destroy(IRequestHandler* handler)
	{
		if(NULL == handler)
			return;

		ZQ::common::MutexGuard sync(_lock);
		for(HandlerRels::iterator it = _currentRunningHandlers.begin(); it != _currentRunningHandlers.end(); ++it)
		{
			if(handler == it->first)
			{
				it->second->destroy(handler);
				_currentRunningHandlers.erase(it);
				return;
			}
		}
		if(_rejectedRequest.find(handler) != _rejectedRequest.end()) {
			_rejectedRequest.erase(handler);
			delete handler;
		}
	}
public:
	bool registerHandler(const std::string& urlsyntax, IRequestHandlerFactory* handlerFac)
	{
		ZQ::common::MutexGuard sync(_lock);
		return _selector.set(urlsyntax, handlerFac);
	}
private:
	ZQ::common::Log& _log;
	ZQ::common::NativeThreadPool& _pool; // for the pending

	selector<IRequestHandlerFactory> _selector;

	typedef std::pair<IRequestHandler*, IRequestHandlerFactory*> HandlerRel;
	typedef std::list<HandlerRel> HandlerRels;
	HandlerRels _currentRunningHandlers;

	ZQ::common::Mutex _lock;
	std::set <IRequestHandler*> _rejectedRequest;
	int32 _maxPendingRequest;
};


//////////////////////////////////////////////////
/// Engine
Engine::Engine(ZQ::common::Log& log)
:_log(log)
{
	_dispatcher = new RequestDispatcher(_log, _pool);
	_fac = new DialogFactory(_log, *_dispatcher, _pool);
	_env.mLogger = &_log;
	_env.dataFactory = _fac;
	_dak = new ZQ::DataPostHouse::DataPostDak(_env, _fac);
	_svr = new ZQ::DataPostHouse::AServerSocketTcp(*_dak, _env);

	setEndpoint("0.0.0.0", "80");
	setCapacity(5);
	enableMessageDump(true, false, false);
}
Engine::~Engine()
{
	_svr = NULL;
	delete _dak;
	_dak = NULL;
	_fac = NULL;
	delete _dispatcher;
	_dispatcher = NULL;
}


void Engine::setEndpoint(const std::string& host, const std::string& port)
{
	_host = host;
	_port = port;
}
void Engine::setCapacity(size_t nConcurrentThread, int maxPendingRequest)
{
	_nConcurrentThread = nConcurrentThread > 1 ? nConcurrentThread : 1;
	_dispatcher->setMaxPendingRequest(maxPendingRequest);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "setCapacity() set ConcurrentThread=%d, MaxPendingRequest=%d"), _nConcurrentThread, maxPendingRequest);
}

void Engine::setMaxConnections(int maxConnections)
{
	_fac->setMaxConnections(maxConnections);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "setMaxConnections() set MaxConnections=%d"), maxConnections);
}
void Engine::setIdleTimeout(int idleTimeout) {
	_fac->setIdleTimeout(idleTimeout);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "setIdleTimeout() set IdleTimeout=%d"), idleTimeout);
}
void Engine::enableMessageDump(bool textMode, bool incomingMessage, bool outgoingMessage) {
	_fac->enableMessageDump(textMode, incomingMessage, outgoingMessage);
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "enableMessageDump() set mode=%s, incomingMessage=%s, outgoingMessage=%s"), (textMode ? "text" : "hex"), (incomingMessage? "yes" : "no"), (outgoingMessage ? "yes" : "no"));
}
void Engine::registerHandler(const std::string& urlsyntax, IRequestHandlerFactory* handlerFac)
{
	_dispatcher->registerHandler(urlsyntax, handlerFac);
}

bool Engine::start()
{
	if(!_dak->startDak(_nConcurrentThread))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(Engine, "start() failed to start dak with %d concurrent thread"), _nConcurrentThread);
		return false;
	}
	_pool.resize(_nConcurrentThread + DEFAULT_THRPOOL_SZ);
	if(!_svr->startServer(_host, _port))
	{
		_log(ZQ::common::Log::L_ERROR, CLOGFMT(Engine, "start() failed to start socket server at %s:%s"), _host.c_str(), _port.c_str());
		_dak->stopDak();
		return false;
	}
	_fac->startIdleMonitor();
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "Start Engine at %s:%s with %d concurrent thread"), _host.c_str(), _port.c_str(), _nConcurrentThread);

	return true;
}
void Engine::stop()
{
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "Stopping engine..."));
	_fac->stopIdleMonitor();
	_svr->stop();
	_dak->stopDak();
	_pool.stop();
	_log(ZQ::common::Log::L_INFO, CLOGFMT(Engine, "Engine stoped."));
}


//////////////////////////////////////////////////////////////////////////
EngineStatistics::EngineStatistics()
{
	reset();
}

EngineStatistics::RespCode EngineStatistics::errCodeToRespCode( int32 errCode )
{
	if( errCode/100 == 2)
	{
		return RESP_2XX;
	}
	switch( errCode )
	{
	case 400:	return RESP_400;
	case 403:	return RESP_403;
	case 404:	return RESP_404;
	case 405:	return RESP_405;
	case 500:	return RESP_500;
	case 501:	return RESP_501;
	case 503:	return RESP_503;
	default:	return RESP_OTHER;
	}
}
/*
void EngineStatistics::addCounter(Method mtd, int32 errCode, int64 latencyHeader, int64 latencyBody )
{
	ZQ::common::MutexGuard gd(_locker);
	RPSTATUSMAP::iterator it = _rpStatus.find(mtd);
	if( it == _rpStatus.end() )
	{
		RequestProcessingStatus status;

		status.latencyInMsBodyAvg = latencyBody;
		status.latencyInMsBodyMax = latencyBody;

		status.latencyInMsHeaderAvg = latencyHeader;
		status.latencyInMsHeaderMax = latencyHeader;

		status.mtd = mtd;
		if( mtd == GET )
		{
			status.mtdString = "GET";
		}
		else if( mtd ==  POST )
		{
			status.mtdString = "POST";
		}
		else if( mtd ==  PUT )
		{
			status.mtdString = "PUT";
		}
		else if( mtd ==  M_DELETE )
		{
			status.mtdString = "DELETE";
		}
		else
		{
			status.mtdString = "UNKNOWN";
		}

		status.totalCount = 1;
		status.reqCount[ errCodeToRespCode(errCode) ] = 1;
		_rpStatus[mtd] = status;
	}
	else
	{
		RequestProcessingStatus& status = it->second;

		status.latencyInMsBodyMax = MAX( latencyBody, status.latencyInMsBodyMax );
		status.latencyInMsHeaderMax = MAX( latencyHeader, status.latencyInMsHeaderMax );

		status.latencyInMsBodyAvg = (status.latencyInMsBodyAvg * status.totalCount + latencyBody) / (status.totalCount + 1);
		status.latencyInMsHeaderAvg = ( status.latencyInMsHeaderAvg * status.totalCount + latencyHeader ) / (status.totalCount + 1);

		status.totalCount ++;

		status.reqCount[errCodeToRespCode(errCode)] ++;
	}
}

void EngineStatistics::reset()
{
	ZQ::common::MutexGuard gd(_locker);
	_mesureSince = ZQ::common::now();
	_rpStatus.clear();
}
*/

void EngineStatistics::reset()
{
	ZQ::common::MutexGuard gd(_locker);
	memset(&_counters, 0x00, sizeof(_counters));
	_mesureSince = ZQ::common::now();
}

void EngineStatistics::addCounter(Method mtd, int32 errCode, int64 latencyHeader, int64 latencyBody )
{
	if (mtd >= METHOD_MAX)
		mtd = UNKNOWN;

	ZQ::common::MutexGuard gd(_locker);
	_counters[mtd].totalCount ++;
	_counters[mtd].respCount[errCodeToRespCode(errCode)] ++;

	if (_counters[mtd].maxLatencyInMs_Body < latencyBody)
		_counters[mtd].maxLatencyInMs_Body = latencyBody;
	if (_counters[mtd].maxLatencyInMs_Header < latencyHeader)
		_counters[mtd].maxLatencyInMs_Header = latencyHeader;

	_counters[mtd].subtotalLatencyInMs_Body += latencyBody;
	_counters[mtd].subtotalLatencyInMs_Header += latencyHeader;

	_counters[mtd].avgLatencyInMs_Body = _counters[mtd].totalCount ? (_counters[mtd].subtotalLatencyInMs_Body /_counters[mtd].totalCount) :0;
	_counters[mtd].avgLatencyInMs_Header = _counters[mtd].totalCount ? (_counters[mtd].subtotalLatencyInMs_Header /_counters[mtd].totalCount) :0;
}

const char* EngineStatistics::nameOfMethod(int mtd)
{
	switch(mtd)
	{
	case GET: return "GET";
	case POST: return "POST";
	case PUT: return "PUT";
	case M_DELETE: return "DELETE";
	}

	return "UNKOWN";
}

} // namespace ZQHttp

