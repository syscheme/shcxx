#include "ZQ_common_conf.h"
#include "LIPC.h"
#include "TimeUtil.h"

#define SYSTEM_DESCRIBE "systemDescribe"

#define MAX_CSEQ    0x0fffffff
#define JSON_RPC_PROTO "jsonrpc"
#define JSON_RPC_PROTO_VERSION "2.0"
#define JSON_RPC_ID "id"
#define JSON_RPC_METHOD "method"
#define JSON_RPC_PARAMS "params"
#define JSON_RPC_RESULT "result"
#define JSON_RPC_ERROR "error"
#define JSON_RPC_ERROR_CODE "code"
#define JSON_RPC_ERROR_MESSAGE "message"
#define JSON_RPC_ERROR_DATA "data"
#define JSON_RPC_FD "fd"
#define JSON_RPC_FD_TYPE "fd_type"

namespace ZQ {
namespace eloop {

// ------------------------------------------------
// class LIPCMessage
// ------------------------------------------------
LIPCMessage::LIPCMessage(int cseq, Json::Value msg)
	: _msg(msg), _cSeq(cseq)
{
	_stampCreated = ZQ::common::now();
}

const char* LIPCMessage::errDesc(Error code)
{
	switch(code)
	{
	case LIPC_PARSING_ERROR    : return "Parse error.";
	case LIPC_INVALID_REQUEST  : return "Invalid JSON-RPC request.";
	case LIPC_METHOD_NOT_FOUND : return "Method not found.";
	case LIPC_INVALID_PARAMS   : return "Invalid params.";
	case LIPC_INTERNAL_ERROR   : return "Internal error.";
	default                    : return "server error.";
	}
}

void LIPCMessage::setErrorCode(Error code)
{
	Json::Value err;
	err[JSON_RPC_ERROR_CODE]    = code;
	err[JSON_RPC_ERROR_MESSAGE] = errDesc(code);
	_msg[JSON_RPC_ERROR]        = err;
}

LIPCMessage::Error LIPCMessage::getErrorCode()
{
	return _msg.isMember(JSON_RPC_ERROR_CODE)?(Error)_msg[JSON_RPC_ERROR_CODE].asInt():LIPC_OK;
}

//int LIPCMessage::getCSeq() const
//{
//	return _msg.isMember(JSON_RPC_ID) ? _msg[JSON_RPC_ID].asInt() :-1;
//}

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
	return _msg.isMember(JSON_RPC_FD_TYPE) ?(LIPCMessage::FdType)(_msg[JSON_RPC_FD_TYPE].asUInt()) :LIPC_NONE;
}

bool LIPCMessage::hasFd()
{
	return _msg.isMember(JSON_RPC_FD)?true:false;
}

bool LIPCMessage::empty()
{
	return (Json::Value::null == _msg);
}

std::string LIPCMessage::toString()
{
	if (Json::Value::null == _msg)
		return "";
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
//void LIPCRequest::setMethod(const std::string& methodName, Callback_t cb, void* data)
//{
//	_msg[JSON_RPC_METHOD] = methodName;
//	if ((NULL == cb)|| (NULL == data))
//		return;
//
//	_cbInfo.cb = cb;
//	_cbInfo.data = data;
//}
//
//std::string LIPCRequest::getMethodName()
//{
//	return _msg.isMember(JSON_RPC_METHOD)?_msg[JSON_RPC_METHOD].asString():"";
//}
//
//LIPCRequest::Callback& LIPCRequest::getCb()
//{
//	return _cbInfo;
//}
//
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

void LIPCResponse::post()
{
	if (_cSeq > 0)
		_conn.send(toString(), getFd());
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
		:_service(service), UnixSocket(service._lipcLog)
	{}

	void start()
	{
		read_start();
		_service.addConn(this);
//		printf("new pipe Passive Conn\n");
		_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(PassiveConn, "new passive conn"));
	}

	virtual void OnMessage(std::string& msg)
	{
		Json::Value root;
		// parsing
		bool parsing = Json::Reader().parse(msg, root);

		if(!parsing)
		{
			LIPCResponse::Ptr resp = new LIPCResponse(0, *this);
			resp->setErrorCode(LIPCMessage::LIPC_PARSING_ERROR);
			send(resp->toString());
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
		int cseq =-1;
		if (msg.isMember(JSON_RPC_FD))
			cseq = msg[JSON_RPC_FD].asInt();

		if (msg.isMember(JSON_RPC_METHOD))
			methodName = msg[JSON_RPC_METHOD].asString();

		if (cseq <=0 || methodName.empty())
			return;

		LIPCRequest::Ptr req = new LIPCRequest(cseq, msg);
		LIPCResponse::Ptr resp = new LIPCResponse(cseq, *this);

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
int LIPCService::init(ZQ::eloop::Loop &loop, int ipc)
{
	_ipc = ipc;
	return ZQ::eloop::Pipe::init(loop, ipc);
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
	ClientConn(ZQ::common::Log& log, LIPCClient& client):UnixSocket(log), _client(client){}

	virtual void OnConnected(ElpeError status) { _client.OnConnected(status);	}
	virtual void OnWrote(int status)  {	_client.OnWrote(status);	}
	virtual void OnShutdown(ElpeError status)	{ _client.OnShutdown(status);	}
	virtual void OnClose()	{	_client.OnCloseHandle(); }
	virtual void OnMessage(std::string& msg) {	_client.OnMessage(msg); }
	virtual void onError( int error, const char* errorDescription ) {	_client.onError(error, errorDescription); }

private:
	LIPCClient& _client;
};

// ------------------------------------------------
// class LIPCClient
// ------------------------------------------------
LIPCClient::LIPCClient(Loop &loop, ZQ::common::Log& log, int ipc)
		:_loop(loop), _ipc(ipc), _lipcLog(log), _conn(NULL), _reconnect(false)
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
	_peerPipeName = name;
	int ret = 0;
	if (_conn == NULL)
	{
		_conn = new ClientConn(_lipcLog, *this);
		ret = _conn->init(_loop, _ipc);
		if (ret < 0)
			return ret;
		_conn->connect(name);
		return 0;
	}
	if (_reconnect)
	{
		_conn->close();
		return 0;
	}
	_reconnect = true;
	_conn->connect(name);
	return 0;
}

void LIPCClient::OnCloseHandle()
{
	if (_conn != NULL)
	{
		delete _conn;
		_conn = NULL;
	}

	if (!_reconnect)
	{
		OnClose();
		return;
	}

	_conn = new ClientConn(_lipcLog, *this);
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
	if (_conn == NULL)
		return;
	_reconnect = false;
	_conn->close();
}

int LIPCClient::shutdown()
{
	if (_conn == NULL)
		return -1;
	return _conn->shutdown();
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

int LIPCClient::sendRequest(const std::string& methodName, LIPCRequest::Ptr req)
{
	if (_conn == NULL || !req || methodName.empty())
		return -1;

	req->_cSeq = lastCSeq();
	req->_msg[JSON_RPC_METHOD] = methodName;
	Json::Value param = req->getParam();
	OnRequestPrepared(methodName, req->_cSeq, req->getParam());
		
	int ret = _conn->send(req->toString(), req->getFd());
	if (ret < 0)
	{
		std::string desc = "send error:";
		desc.append(ZQ::eloop::Handle::errDesc(ret));
		onError(ret, desc.c_str());
		return ret;
	}
	
	return req->_cSeq;
}

void LIPCClient::OnIndividualMessage(Json::Value& msg)
{
	int cseq =-1;
	if (msg.isMember(JSON_RPC_FD))
		 cseq = msg[JSON_RPC_FD].asInt();

	if (cseq <=0)
		return;

	LIPCResponse::Ptr resp = new LIPCResponse(cseq, *_conn);
	resp->_msg = msg;

	OnResponse(resp);
}

void LIPCClient::OnMessage(std::string& msg)
{
	// process(msg,*_conn);
	
	Json::Value root;
	// parsing
	bool parsing = Json::Reader().parse(msg, root);

	if(!parsing)
	{
		LIPCResponse::Ptr resp = new LIPCResponse(0, *_conn);
		resp->setErrorCode(LIPCMessage::LIPC_PARSING_ERROR);
		_conn->send(resp->toString());
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

}}//ZQ::LIPC
