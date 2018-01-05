#include "ZQ_common_conf.h"
#include "LIPC.h"
#include "TimeUtil.h"

#define MAX_CSEQ    0x0fffffff

#define SYSTEM_DESCRIBE        "LIPC.systemDescribe"
#define JSON_RPC_PROTO         "LIPC.jsonrpc"
#define JSON_RPC_PROTO_VERSION "LIPC.2.0"
#define JSON_RPC_ID            "LIPC.id"
#define JSON_RPC_METHOD        "LIPC.method"
#define JSON_RPC_PARAMS        "LIPC.params"
#define JSON_RPC_RESULT        "LIPC.result"
#define JSON_RPC_EXCEPTION     "LIPC.exception"
#define JSON_RPC_ERROR         "LIPC.error"
#define JSON_RPC_ERROR_CODE    "LIPC.code"
#define JSON_RPC_ERROR_MESSAGE "LIPC.message"
#define JSON_RPC_ERROR_DATA    "LIPC.data"
#define JSON_RPC_FD            "LIPC.fd"
#define JSON_RPC_FD_TYPE       "LIPC.fd_type"

namespace ZQ {
namespace eloop {

//#define TRACE_LEVEL_FLAG  FLAG(0)

#define TRACE_LEVEL_FLAG (_verboseFlags & FLG_TRACE)
#define INFO_LEVEL_FLAG (_verboseFlags & FLG_INFO)
// ------------------------------------------------
// class LIPCMessage
// ------------------------------------------------
LIPCMessage::LIPCMessage(int cseq, Json::Value msg)
	: _msg(msg), _cSeq(cseq)
{
	_stampCreated = ZQ::common::now();
}

int64 LIPCMessage::hexvalue(const char* hexstr)
{
	const char* p = strstr(hexstr, "0x");
	if (NULL == p)
		p = strstr(hexstr, "0X");
	if (NULL == p)
		p = hexstr;
	else p +=2;

	int64 ret = 0;
	sscanf(p,"%llX",&ret);
	return ret;
}

std::string LIPCMessage::hexstr(int64 value, bool prefix0X)
{
	char str[40];
	snprintf(str, sizeof(str)-2, "%s%llX", prefix0X?"0X":"", value);
	return str;
}

const char* LIPCMessage::errDesc(Error code)
{
	switch(code)
	{
	case LIPC_OK               : return "OK";

	case LIPC_CLIENT_ERROR     : return "client error";
	case LIPC_REQUEST_TIMEOUT  : return "request timeout";
	case LIPC_REQUEST_CONFLICT : return "request conflict";
	case LIPC_NOT_IMPLEMENTED  : return "not implemented";
	case LIPC_SERVICE_UNAVAIL  : return "service unavailable";
	case LIPC_UNSUPPORT_VERSION: return "unsupported version";

	case LIPC_INVALID_FD	   : return "invalid fd";
	case LIPC_INVALID_FD_TYPE  : return "invalid fd-type";


	case LIPC_PARSING_ERROR    : return "parse error";
	case LIPC_INVALID_REQUEST  : return "invalid JSON-RPC request";
	case LIPC_METHOD_NOT_FOUND : return "method not found";
	case LIPC_INVALID_PARAMS   : return "invalid params";
	case LIPC_INTERNAL_ERROR   : return "internal error";

	case LIPC_SERVER_ERROR     : 
	default                    : return "server error";
	}
}

void LIPCMessage::setErrorCode(int code,std::string errMsg)
{
	Json::Value err;
	err[JSON_RPC_ERROR_CODE]    = code;
	if (errMsg.empty())
		errMsg = errDesc((Error)code);
	err[JSON_RPC_ERROR_MESSAGE] = errMsg;
	_msg[JSON_RPC_ERROR]        = err;
}

LIPCMessage::Error LIPCMessage::getErrorCode()
{
	if (_msg.isMember(JSON_RPC_ERROR))
	{
		Json::Value err = _msg[JSON_RPC_ERROR];
		return err.isMember(JSON_RPC_ERROR_CODE) ? (Error)err[JSON_RPC_ERROR_CODE].asInt() :LIPC_OK;
	}
	return LIPC_OK;
}

int LIPCMessage::getCSeq() const 
{
	if (_cSeq > 0)
		return _cSeq; 

	return _msg.isMember(JSON_RPC_ID) ? _msg[JSON_RPC_ID].asInt() :-1;
}


void LIPCMessage::setFd(fd_t fd, FdType type)
{
	_msg[JSON_RPC_FD] = fd;
	_msg[JSON_RPC_FD_TYPE] = type;
}

int LIPCMessage::getFd() const
{
	return _msg.isMember(JSON_RPC_FD) ? _msg[JSON_RPC_FD].asInt() :-1;
}

LIPCMessage::FdType LIPCMessage::getFdType() const
{
	return _msg.isMember(JSON_RPC_FD_TYPE) ? (LIPCMessage::FdType)(_msg[JSON_RPC_FD_TYPE].asUInt()) :LIPC_NONE;
}

bool LIPCMessage::hasFd()
{
	return _msg.isMember(JSON_RPC_FD);
}

bool LIPCMessage::empty()
{
	return (Json::Value::null == _msg);
}

std::string LIPCMessage::toString()
{
	if (Json::Value::null == _msg)
		return "";
	if (_cSeq > 0)
		_msg[JSON_RPC_ID] = _cSeq;
	_msg[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	Json::FastWriter writer;
	return writer.write(_msg);
}

bool LIPCMessage::fromString(const std::string& str)
{
	_msg = Json::Value::null;
	return Json::Reader().parse(str, _msg);
}

// ------------------------------------------------
// class LIPCRequest
// ------------------------------------------------
void LIPCRequest::setParam(const Json::Value& param)
{
	_msg[JSON_RPC_PARAMS] = param;
}

Json::Value LIPCRequest::getParam()
{
	if (_msg.isMember(JSON_RPC_PARAMS))
		 return _msg[JSON_RPC_PARAMS];
	
	return Json::Value::null;
}

std::string LIPCRequest::getMethod()
{
	if (_msg.isMember(JSON_RPC_METHOD))
		return _msg[JSON_RPC_METHOD].asString();
	return "";
}

// ------------------------------------------------
// class LIPCResponse
// ------------------------------------------------
LIPCResponse::~LIPCResponse()
{
}

void LIPCResponse::setResult(const Json::Value& result)
{
	_msg[JSON_RPC_RESULT] = result;
}

void LIPCResponse::post(bool bAsync)
{
	if (_cSeq <= 0)
		return;

 	if (bAsync)
 		_conn.AsyncSend(toString(),getFd());
 	else
 		_conn.send(toString(), getFd());

//	int64 step1 = ZQ::eloop::usStampNow();
// 	std::string temp = toString();
// 	int64 step2 = ZQ::eloop::usStampNow();
// 
// 	if (bAsync)
// 		_conn.AsyncSend(temp,getFd());
// 	else
// 		_conn.send(temp, getFd());
// 	int64 step3 = ZQ::eloop::usStampNow();
// 
// 	int64 took1 = step2 - step1;
// 	int64 took2 = step3 - step2;
// 
// 	_conn._lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(LIPCResponse, "post() JsonToString took[%lld] AsyncSend took[%lld]us"),took1,took2);

}

void LIPCResponse::postException(int code, std::string errMsg,bool bAsync)
{
	setErrorCode(code,errMsg);
	post(bAsync);
}

Json::Value LIPCResponse::getResult()
{
	if (_msg.isMember(JSON_RPC_RESULT))
		return _msg[JSON_RPC_RESULT];

	return Json::Value::null; 
}

// -----------------------------
// class PassiveConn
// -----------------------------
class PassiveConn : public UnixSocket
{
public:
	PassiveConn(LIPCService& service)
		:_service(service), UnixSocket(service._log)
	{}

	void start()
	{
		read_start();
		_service.addConn(this);

		//if (TRACE_LEVEL_FLAG)
			_service._log(ZQ::common::Log::L_DEBUG, CLOGFMT(PassiveConn, "new passive conn"));
	}

	virtual void OnMessage(std::string& msg)
	{
		Json::Value root;
		// parsing
		bool parsing = Json::Reader().parse(msg, root);

		if(!parsing)
		{
			LIPCResponse::Ptr resp = new LIPCResponse(0, *this);
			resp->postException(LIPCMessage::LIPC_PARSING_ERROR);
			return;
		}

		if (!root.isArray())
		{
			OnIndividualMessage(root);
			return;
		}

		//  batched call
		for(Json::Value::ArrayIndex i = 0 ; i < root.size() ; i++)
			OnIndividualMessage(root[i]);
	}

	void OnIndividualMessage(Json::Value& msg)
	{
		std::string methodName;
		int cseq = -1;
		
		if (msg.isMember(JSON_RPC_ID))
			cseq = msg[JSON_RPC_ID].asUInt();

		LIPCResponse::Ptr resp = new LIPCResponse(cseq, *this);

		if (msg.isMember(JSON_RPC_METHOD))
			methodName = msg[JSON_RPC_METHOD].asString();

		if (methodName.empty())
		{
			resp->postException(LIPCMessage::LIPC_METHOD_NOT_FOUND);
			return;
		}

		LIPCRequest::Ptr req = new LIPCRequest(msg);
		

		if (req->hasFd())
		{
#ifdef ZQ_OS_LINUX
			req->setFd(acceptfd(), req->getFdType());
#else
			req->setFd(-1, LIPCMessage::LIPC_NONE);
#endif
		}

		_service.execOrDispatch(methodName, req, resp);
	}

	virtual void onError( int error, const char* errorDescription)
	{
		_service.onError(error, errorDescription);
		close();
	}

	virtual void OnWrote(int status)
	{
		if (status != elpeSuccess)
		{
			std::string desc = "send error:";
			desc.append(errDesc(status));
			onError(status, desc.c_str());
			return;
		}
	}

	virtual void OnClose()
	{
		_service.delConn(this);
		delete this;
	}

private:
	LIPCService&	_service;
};

// -------------------------------------------------
// class LIPCService
// -------------------------------------------------
uint32 LIPCService::_verboseFlags =0xffffffff;

int LIPCService::init(ZQ::eloop::Loop &loop, int ipc)
{
	_ipc = ipc;
	return ZQ::eloop::Pipe::init(loop, ipc);
}

void LIPCService::UnInit()
{
	close();
	for(PipeClientList::iterator it=_clients.begin(); it!= _clients.end(); it++)
		(*it)->close();
}

void LIPCService::OnClose()
{ 
	_isOnClose = true;
	if (_clients.empty())
		OnUnInit();
}

void LIPCService::OnUnInit()
{
	if(TRACE_LEVEL_FLAG)
	_log(ZQ::common::Log::L_DEBUG, CLOGFMT(LIPCService, "OnUnInit()"));
}

void LIPCService::addConn(PassiveConn* conn)
{
	_clients.push_back(conn);
}

void LIPCService::delConn(PassiveConn* conn)
{
	PipeClientList::iterator iter = _clients.begin();
	while(iter != _clients.end())
	{
		if (*iter == conn)
			iter = _clients.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
	if (_isOnClose)
		OnUnInit();
}

void LIPCService::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
		std::string desc = "accept error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		onError(status, desc.c_str());
		return;
	}

	PassiveConn *client = new PassiveConn(*this);
	client->init(get_loop(), _ipc);

	int ret = accept(client);
	if (ret == 0) {
		client->start();
	}
	else {
		client->close();
		std::string desc = "accept error:";
		desc.append(ZQ::eloop::Handle::errDesc(ret));
		onError(ret, desc.c_str());
	}
}

// ------------------------------------------------
// class ClientConn
// ------------------------------------------------
// as an established connection in Client
class ClientConn : public UnixSocket
{
public:
	ClientConn(ZQ::common::LogWrapper& log, LIPCClient& client):UnixSocket(log), _client(client){}

	virtual void OnConnected(ElpeError status) { _client.OnConnected(status);	}
	virtual void OnWrote(int status)  {	OnAsyncSend(); _client.OnWrote(status);	}
	virtual void OnShutdown(ElpeError status)	{ _client.OnShutdown(status);	}
	virtual void OnClose()	{	_client.OnCloseConn(); }
	virtual void OnMessage(std::string& msg) {	_client.OnMessage(msg); }
	virtual void onError( int error, const char* errorDescription ) {	_client.onError(error, errorDescription); }

private:
	LIPCClient& _client;
};

// ------------------------------------------------
// class ClientTimer
// ------------------------------------------------
class ClientTimer : public ZQ::eloop::Timer
{
public:
	ClientTimer(LIPCClient& client):_client(client){}

	virtual void OnTimer(){_client.OnTimer();}
	virtual void OnClose(){_client.OnCloseTimer();}

private:
	LIPCClient& _client;
};

// ------------------------------------------------
// class LIPCClient
// ------------------------------------------------
uint32 LIPCClient::_verboseFlags =0xffffffff;

LIPCClient::LIPCClient(Loop &loop, ZQ::common::Log& log, int64 timeout,int ipc)
		:_loop(loop), _ipc(ipc), _log(log), _conn(NULL), _reconnect(false),_timeout(timeout),_timer(NULL)
{
	_lastCSeq.set(1);
}

uint LIPCClient::lastCSeq()
{
	int v = _lastCSeq.add(1);
	if (v>0 && v < MAX_CSEQ)
		return (uint) v;

	static ZQ::common::Mutex lock;
	ZQ::common::MutexGuard g(lock);
	v = _lastCSeq.add(1);
	if (v >0 && v < MAX_CSEQ)
		return (uint) v;

	_lastCSeq.set(1);
	v = _lastCSeq.add(1);

	return (uint) v;
}

int  LIPCClient::bind(const char *name)
{
	if (_conn == NULL)
		return -1;
	_localPipeName = name;

	return _conn->bind(name);
}

ZQ::eloop::Loop& LIPCClient::get_loop() const
{
	return _loop;
}

int LIPCClient::connect(const char *name)
{
	if (_timer == NULL)
	{
		_timer = new ClientTimer(*this);
		_timer->init(_loop);
		_timer->start(0, 200); // set scan interval as 5Hz
	}

	_peerPipeName = name;
	if (_conn == NULL)
	{
		_conn = new ClientConn(_log, *this);
		int ret = _conn->init(_loop, _ipc);
		if (ret < 0)
			return ret;

		_conn->connect(name);
		_reconnect = true;
		_isConn = false;
		return 0;
	}

	if (_reconnect)
	{
		_conn->close();
		return 0;
	}

	_reconnect = true;
	_isConn = false;
	_conn->connect(name);
	return 0;
}

void LIPCClient::OnCloseTimer()
{
	if (_timer != NULL)
		delete _timer;
	_timer = NULL;

	if (_conn == NULL)
	{
		{
			ZQ::common::MutexGuard g(_lkAwaits);
			for (AwaitRequestMap::iterator itW = _awaits.begin(); itW != _awaits.end();)
			{
				OnRequestDone(itW->first, LIPCMessage::LIPC_CLIENT_CLOSED);
				_awaits.erase(itW++);
			}
		} // end of _lkAwaits
		OnClose();
	}
}


void LIPCClient::OnCloseConn()
{
	if (_conn != NULL)
		delete _conn;
	_conn = NULL;
	_isConn = false;

	if (!_reconnect)
	{
		if (_timer == NULL)
		{
			{
				ZQ::common::MutexGuard g(_lkAwaits);
				for (AwaitRequestMap::iterator itW = _awaits.begin(); itW != _awaits.end();)
				{
					OnRequestDone(itW->first, LIPCMessage::LIPC_CLIENT_CLOSED);
					_awaits.erase(itW++);
				}
			} // end of _lkAwaits
			OnClose();
		}
		return;
	}

	_conn = new ClientConn(_log, *this);
	int ret = _conn->init(_loop, _ipc);
	if (ret < 0)
	{
		std::string desc = "reconnect init error:";
		desc.append(ZQ::eloop::Handle::errDesc(ret));
		onError(ret, desc.c_str());
		return;
	}

	if (!_localPipeName.empty())
	{
#ifdef ZQ_OS_LINUX
		unlink(_localPipeName.c_str());
#else
#endif
		_conn->bind(_localPipeName.c_str());
	}

	_conn->connect(_peerPipeName.c_str());
}

void LIPCClient::close()
{
	if (_conn != NULL)
	{
		_reconnect = false;
		_isConn = false;
		_conn->close();
	}

	if (_timer != NULL)
		_timer->close();
}

int LIPCClient::shutdown()
{
	if (_conn == NULL)
		return -1;

	return _conn->shutdown();
}

void LIPCClient::OnTimer()
{
	std::vector<uint> expiredList;

	{
		ZQ::common::MutexGuard g(_lkAwaits);
		for (AwaitRequestMap::iterator itW = _awaits.begin(); itW != _awaits.end();)
		{
			if (itW->second.expiration < ZQ::common::now())
			{
				expiredList.push_back(itW->first);
				_awaits.erase(itW++);
				continue;
			}
			itW++;
		}
	} // end of _lkAwaits

	for (size_t i =0; i < expiredList.size(); i++)
		OnRequestDone(expiredList[i], LIPCMessage::LIPC_REQUEST_TIMEOUT);
}

int LIPCClient::read_start()
{
	if (_conn == NULL)
		return -1;

	return _conn->read_start();
}

int LIPCClient::read_stop()
{
	if (_conn == NULL)
		return -1;

	return _conn->read_stop();
}

int LIPCClient::sendRequest(const std::string& methodName, LIPCRequest::Ptr req,int64 timeout, bool bAsync, bool expectResp)
{
	if (_conn == NULL || !req || methodName.empty())
		return -1;

	req->_msg[JSON_RPC_METHOD] = methodName;

	if (expectResp)
	{
		req->_cSeq = lastCSeq();
		AwaitRequest ar;
		ar.req = req;
		ar.method = methodName;
		_timeout = (timeout > 0)?timeout:_timeout;
		ar.expiration = ZQ::common::now() + _timeout;

		ZQ::common::MutexGuard g(_lkAwaits);
		_awaits.insert(AwaitRequestMap::value_type(req->_cSeq, ar));
	}

	OnRequestPrepared(req);
	
	int ret = 0;
	if (bAsync)
		ret = _conn->AsyncSend(req->toString(), req->getFd());
	else
		ret = _conn->send(req->toString(), req->getFd());
	if (ret < 0)
	{
		OnRequestDone(req->_cSeq, LIPCMessage::LIPC_CLIENT_ERROR);
		return ret;
	}
	
	return req->_cSeq;
}

void LIPCClient::OnIndividualMessage(Json::Value& msg)
{
	int cseq =-1;
	if (msg.isMember(JSON_RPC_ID))
		 cseq = msg[JSON_RPC_ID].asInt();

	if (cseq <=0)
		return;

	LIPCResponse::Ptr resp = new LIPCResponse(cseq, *_conn);
	resp->_msg = msg;

	if (msg.isMember(JSON_RPC_FD))
	{
#ifdef ZQ_OS_LINUX
		resp->setFd(_conn->acceptfd(), resp->getFdType());
#else
		resp->setFd(-1, LIPCMessage::LIPC_NONE);
#endif
	}

	ZQ::common::MutexGuard g(_lkAwaits);
	AwaitRequestMap::iterator itW = _awaits.find(cseq);
	if (_awaits.end() == itW) // unknown request or it has been previously expired
	{
		if (TRACE_LEVEL_FLAG)
			_log(ZQ::common::Log::L_DEBUG, CLOGFMT(LIPCClient, "unknown request or it has been previously expired. cseq(%d)"),cseq);

		return;
	}

	if (TRACE_LEVEL_FLAG)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(LIPCClient, "OnResponse() %s(%d) start process the response "), itW->second.method.c_str(), cseq);

	int64 stampNow = ZQ::common::now();
	OnResponse(itW->second.method, resp);
	int elapsed = (int) (ZQ::common::now() - stampNow);
	int err = LIPCMessage::LIPC_OK;
	if (resp)
		err = resp->getErrorCode();

	if (TRACE_LEVEL_FLAG)
		_log(ZQ::common::Log::L_DEBUG, CLOGFMT(LIPCClient, "OnResponse() %s(%d) ret(%d) took %dmsec triggered, cleaning from await list"), itW->second.method.c_str(), cseq, err, elapsed);

	OnRequestDone(cseq, err);
	_awaits.erase(cseq);
}

void LIPCClient::OnMessage(std::string& msg)
{
	Json::Value root;

	// parsing
	bool parsing = Json::Reader().parse(msg, root);
	if (!parsing)
	{
		LIPCResponse::Ptr resp = new LIPCResponse(0, *_conn);
		resp->postException(LIPCMessage::LIPC_PARSING_ERROR);
		return;
	}

	if (!root.isArray())
	{
		OnIndividualMessage(root);
		return;
	}

	//  batched call
	for(Json::Value::ArrayIndex i = 0 ; i < root.size() ; i++)
		OnIndividualMessage(root[i]);
}

void LIPCClient::OnConnected(ZQ::eloop::Handle::ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess)
	{
		std::string desc = "connect error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		onError(status,desc.c_str());
		return;
	}
	read_start();
	_isConn = true;
}

void LIPCClient::OnShutdown(ZQ::eloop::Handle::ElpeError status)
{
	if (status != ZQ::eloop::Handle::elpeSuccess)
	{
		std::string desc = "shutdown error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		onError(status,desc.c_str());
		return;
	}
	close();
}

void LIPCClient::OnClose()
{
	delete this;
}

void LIPCClient::onError( int error, const char* errorDescription )
{	
	_log(ZQ::common::Log::L_ERROR, CLOGFMT(LIPCClient, "errCode = %d, errDesc:%s"), error, errorDescription);
	close();
}

}}//ZQ::LIPC
