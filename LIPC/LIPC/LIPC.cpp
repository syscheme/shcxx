#include "ZQ_common_conf.h"
#include "LIPC.h"

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
	namespace LIPC {

// ------------------------------------------------
// class Message
// ------------------------------------------------
const char* Message::errDesc(LIPCError code)
{
	switch(code)
	{
	case LIPC_PARSING_ERROR:return "Parse error.";
	case LIPC_INVALID_REQUEST:return "Invalid JSON-RPC request.";
	case LIPC_METHOD_NOT_FOUND:return "Method not found.";
	case LIPC_INVALID_PARAMS:return "Invalid params.";
	case LIPC_INTERNAL_ERROR:return "Internal error.";
	default:return "server error.";
	}
}

void Message::setErrorCode(LIPCError code){
	Json::Value err;
	err[JSON_RPC_ERROR_CODE] = code;
	err[JSON_RPC_ERROR_MESSAGE] = errDesc(code);
	_msg[JSON_RPC_ERROR] = err;
}

Message::LIPCError Message::getErrorCode()
{
	return _msg.isMember(JSON_RPC_ERROR_CODE)?(LIPCError)_msg[JSON_RPC_ERROR_CODE].asInt():LIPC_OK;
}

void Message::setCSeq(int cseq)	{
	_msg[JSON_RPC_ID] = cseq;
}

int Message::getCSeq() const{
	return _msg.isMember(JSON_RPC_ID)?_msg[JSON_RPC_ID].asInt():-1;
}

void Message::setFd(fd_t fd,FdType type)
{
	_msg[JSON_RPC_FD] = fd;
	_msg[JSON_RPC_FD_TYPE] = type;
}

int Message::getFd() const{
	return _msg.isMember(JSON_RPC_FD)?_msg[JSON_RPC_FD].asInt():-1;
}

Message::FdType Message::getFdType() const{
	return _msg.isMember(JSON_RPC_FD_TYPE)?(Message::FdType)(_msg[JSON_RPC_FD_TYPE].asUInt()):LIPC_NONE;
}

bool Message::hasFd(){
	return _msg.isMember(JSON_RPC_FD)?true:false;
}

bool Message::empty()
{
	return (Json::Value::null == _msg)?true:false;
}

std::string Message::toString()
{
	if (Json::Value::null == _msg)
		return "";
	_msg[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	Json::FastWriter writer;
	return writer.write(_msg);
}

bool Message::fromString(const std::string& str)
{
	_msg = Json::Value::null;
	return Json::Reader().parse(str, _msg);
}

// ------------------------------------------------
// class Request
// ------------------------------------------------
void Request::setMethod(const std::string& methodName,RpcCB cb,void* data)
{
	_msg[JSON_RPC_METHOD] = methodName;
	if ((cb != NULL)&&(data != NULL))
	{
		_cbInfo.cb = cb;
		_cbInfo.data = data;
	}
}
std::string Request::getMethodName()
{
	return _msg.isMember(JSON_RPC_METHOD)?_msg[JSON_RPC_METHOD].asString():"";
}

Request::RpcCBInfo& Request::getCb()
{
	return _cbInfo;
}

void Request::setParam(const Json::Value& param)
{
	_msg[JSON_RPC_PARAMS] = param;
}

Json::Value Request::getParam() const
{
	if (_msg.isMember(JSON_RPC_PARAMS))
		 return _msg[JSON_RPC_PARAMS];
	
	return Json::Value::null;
}

// ------------------------------------------------
// class Response
// ------------------------------------------------
void Response::setResult(const Json::Value& result)
{
	_msg[JSON_RPC_RESULT] = result;
}

Json::Value Response::getResult()
{
	if (_msg.isMember(JSON_RPC_RESULT))
		return _msg[JSON_RPC_RESULT];

	return Json::Value::null; 
}


// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
Handler::Handler()
{
  /* add a RPC method that list the actual RPC methods contained in 
   * the Handler 
   */
	_lastCSeq.set(1);
}

Handler::~Handler()
{
}

uint Handler::lastCSeq()
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


void Handler::addcb(int seqId,Request::RpcCB cb,void* data)
{
	Request::RpcCBInfo info;
	info.cb = cb;
	info.data = data;
	m_seqIds[seqId] = info;
}

void Handler::process(const std::string& msg,PipeConnection& conn)
{
	Json::Value root;
	/* parsing */
	bool parsing = m_reader.parse(msg, root);

	if(!parsing)
	{
		Response::Ptr resp = new Response();
		resp->setErrorCode(Message::LIPC_PARSING_ERROR);
		conn.send(resp->toString());
		return;
	}

	if(root.isArray())
	{
		/* batched call */
		for(Json::Value::ArrayIndex i = 0 ; i < root.size() ; i++)
		{
			invoke(root[i],conn);
		}
	}
	else
	{
		invoke(root,conn);
	}
}

void Handler::invoke(const Json::Value& msg,PipeConnection& conn)
{
	Request::Ptr req = new Request(msg);
	Response::Ptr resp = new Response();

	Json::Value err;
	/* check the JSON-RPC version => 2.0 */
	if(!msg.isObject() || !msg.isMember(JSON_RPC_PROTO) || msg[JSON_RPC_PROTO] != JSON_RPC_PROTO_VERSION) 
	{
		resp->setErrorCode(Message::LIPC_PARSING_ERROR);
		conn.send(resp->toString());
		return;
	}

	if(msg.isMember(JSON_RPC_METHOD))
	{
		std::string method = msg[JSON_RPC_METHOD].asString();
		if (req->hasFd())
		{
#ifdef ZQ_OS_LINUX
			req->setFd(conn.acceptfd(),req->getFdType());
#else
			req->setFd(-1,Message::LIPC_NONE);
#endif
		}
		
		execMethod(method,req,resp);
		
		if (!resp->empty())
		{
			resp->setCSeq(req->getCSeq());
			conn.send(resp->toString(),resp->getFd());
		}
		return;
	}
	else
	{
		if (msg.isMember(JSON_RPC_ID))
		{
			int seqId = msg[JSON_RPC_ID].asInt();
			seqToCBInfoMap::iterator it = m_seqIds.find(seqId);
			if (it != m_seqIds.end())
			{
				Response::Ptr cbMsg = new Response(msg);

				if (cbMsg->hasFd())
				{
#ifdef ZQ_OS_LINUX
					cbMsg->setFd(conn.acceptfd(),cbMsg->getFdType());
#else
					cbMsg->setFd(-1,Message::LIPC_NONE);
#endif
				}

				Request::RpcCB cb = it->second.cb;
				void* data = it->second.data;
				if (cb!=NULL&&data!=NULL)
					(*cb)(cbMsg,data);
				m_seqIds.erase(it);
			}
			return;
		}
	}

	/* forge an error response */
	resp->setErrorCode(Message::LIPC_METHOD_NOT_FOUND);
	conn.send(resp->toString());
}

// -----------------------------
// class PipePassiveConn
// -----------------------------
class PipePassiveConn : public PipeConnection
{
public:
	PipePassiveConn(Service& service)
		:_service(service),PipeConnection(service._lipcLog)
	{}

	void start()
	{
		read_start();
		_service.addConn(this);
//		printf("new pipe Passive Conn\n");
		_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(PipePassiveConn, "new pipe Passive Conn"));
	}


	virtual void OnMessage(std::string& msg)
	{
		_service.process(msg,*this);
	}

	virtual void onError( int error,const char* errorDescription)
	{
		_service.onError(error,errorDescription);
		close();
	}

	virtual void OnWrote(int status)
	{
		if (status != elpeSuccess)
		{
			std::string desc = "send error:";
			desc.append(errDesc(status));
			onError(status,desc.c_str());
			return;
		}
	}

	virtual void OnClose()
	{
		_service.delConn(this);
		delete this;
	}

private:
	Service&	_service;
};

// -------------------------------------------------
// class Service
// -------------------------------------------------
int Service::init(ZQ::eloop::Loop &loop, int ipc)
{
	_ipc = ipc;
	return ZQ::eloop::Pipe::init(loop,ipc);
}

void Service::addConn(PipePassiveConn* conn)
{
	_ClientList.push_back(conn);
}

void Service::delConn(PipePassiveConn* conn)
{
	PipeClientList::iterator iter = _ClientList.begin();
	while(iter != _ClientList.end())
	{
		if (*iter == conn)
			iter = _ClientList.erase(iter);		//_PipeConn.erase(iter++);
		else
			iter++;
	}
}

void Service::doAccept(ZQ::eloop::Handle::ElpeError status)
{
	if (status != Handle::elpeSuccess) {
		std::string desc = "accept error:";
		desc.append(ZQ::eloop::Handle::errDesc(status));
		onError(status,desc.c_str());
		return;
	}

	PipePassiveConn *client = new PipePassiveConn(*this);
	client->init(get_loop(),_ipc);

	int ret = accept(client);
	if (ret == 0) {
		client->start();
	}
	else {
		client->close();
		std::string desc = "accept error:";
		desc.append(ZQ::eloop::Handle::errDesc(ret));
		onError(ret,desc.c_str());
	}
}


// ------------------------------------------------
// class Servant
// ------------------------------------------------
class Servant : public PipeConnection
{
public:
	Servant(ZQ::common::Log& log,Client& client):PipeConnection(log),_client(client){}

protected:

	virtual void OnConnected(ElpeError status)
	{
		_client.OnConnected(status);
	}
	virtual void OnWrote(int status)
	{
		_client.OnWrote(status);
	}
	virtual void OnShutdown(ElpeError status)
	{
		_client.OnShutdown(status);
	}
	virtual void OnClose()
	{
		_client.OnCloseHandle();
	}

	virtual void OnMessage(std::string& msg)
	{
		_client.OnMessage(msg);
	}
	virtual void onError( int error,const char* errorDescription )
	{	
		_client.onError(error,errorDescription);
	}

private:
	Client& _client;
};

// ------------------------------------------------
// class Client
// ------------------------------------------------
int  Client::bind(const char *name)
{
	if (_svt == NULL)
		return -1;
	_localPipeName = name;
	return _svt->bind(name);
}

ZQ::eloop::Loop& Client::get_loop() const
{
	return _loop;
}
int Client::connect(const char *name)
{
	_peerPipeName = name;
	int ret = 0;
	if (_svt == NULL)
	{
		_svt = new Servant(_lipcLog,*this);
		ret = _svt->init(_loop,_ipc);
		if (ret < 0)
			return ret;
		_svt->connect(name);
		return 0;
	}
	if (_reconnect)
	{
		_svt->close();
		return 0;
	}
	_reconnect = true;
	_svt->connect(name);
	return 0;
}

void Client::OnCloseHandle()
{
	if (_svt != NULL)
	{
		delete _svt;
		_svt = NULL;
	}
	if (_reconnect)
	{
		_svt = new Servant(_lipcLog,*this);
		int ret = _svt->init(_loop,_ipc);
		if (ret < 0)
		{
			std::string desc = "reconnect init error:";
			desc.append(ZQ::eloop::Handle::errDesc(ret));
			onError(ret,desc.c_str());
			return;
		}
		if (!_localPipeName.empty())
		{
#ifdef ZQ_OS_LINUX
			unlink(_localPipeName.c_str());
#else
#endif
			_svt->bind(_localPipeName.c_str());
		}
		_svt->connect(_peerPipeName.c_str());
	}
	else
		OnClose();
}

void Client::close()
{
	if (_svt == NULL)
		return;
	_reconnect = false;
	_svt->close();
}

int Client::shutdown()
{
	if (_svt == NULL)
		return -1;
	return _svt->shutdown();
}

int Client::read_start()
{
	if (_svt == NULL)
		return -1;
	return _svt->read_start();
}
int Client::read_stop()
{
	if (_svt == NULL)
		return -1;
	return _svt->read_stop();
}

int Client::sendRequest(Request::Ptr req)
{
	if (_svt == NULL)
		return -1;
	int seqId = 0;
	Request::RpcCBInfo info = req->getCb();
	if ((info.cb != NULL)&&(info.data != NULL))
	{
		seqId = lastCSeq();
		addcb(seqId,info.cb,info.data);
		req->setCSeq(seqId);
	}
	else
		_lipcLog(ZQ::common::Log::L_DEBUG, CLOGFMT(Client, "No callback function."));

	OnRequestPrepared(req->getMethodName(),seqId,req->getParam());
		
	int ret = _svt->send(req->toString(),req->getFd());
	if (ret < 0)
	{
		std::string desc = "send error:";
		desc.append(ZQ::eloop::Handle::errDesc(ret));
		onError(ret,desc.c_str());
		return ret;
	}
	
	return seqId;
}


void Client::OnConnected(ZQ::eloop::Handle::ElpeError status)
{

}

void Client::OnMessage(std::string& msg)
{
	process(msg,*_svt);
}

}}//ZQ::LIPC
