#include "ZQ_common_conf.h"
#include "LIPC.h"

#ifdef ZQ_OS_LINUX
	#include <dirent.h>
#endif

namespace ZQ {
namespace LIPC {

// -------------------------------------------------
// class Message
// -------------------------------------------------
std::string Message::toString()
{
	if (cseq >0)
		params[JSON_RPC_ID] = cseq;

	params[VER] = cseq;
	Json::FastWriter writer;
	return writer.write(params);
}

bool Message::fromString(const std::string& str)
{
	params = Json::Value::null;
	cseq =-1;
	bool ret = Json::Reader().parse(str, params);
	if (ret && params.isMember(JSON_RPC_ID))
		cseq = params[JSON_RPC_ID].asInt();

	return ret;
}

// ------------------------------------------------
// class Client
// ------------------------------------------------
int Client::sendHandlerRequest(std::string method, Json::Value param, RpcCB cb, void* data, ZQ::eloop::Handle* send_Handler)
{
	Message req;
	req.params[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	req.params[JSON_RPC_METHOD] = method;
	if (param != Json::Value::null)
		req.params[JSON_RPC_PARAMS] = param;

	req.cseq = lastCSeq();

	if (cb !=NULL)
		Addcb(req.cseq, cb, data);

	OnRequestPrepared(method, req);

	int ret = send(req.toString(), send_Handler); 
	if (ret < 0)
		return ret;

	return req.cseq;
}

int Client::sendRequest(std::string method, Json::Value param, RpcCB cb, void* data, int fd)
{
	Message req;
	req.params[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	req.params[JSON_RPC_METHOD] = method;
	if (param != Json::Value::null)
		req.params[JSON_RPC_PARAMS] = param;

	req.cseq = lastCSeq();

	if (cb !=NULL)
		Addcb(req.cseq, cb, data);

	int ret = sendfd(req.toString(), fd);
	if (ret < 0)
		return ret;

	return req.cseq;
}

void Client::OnMessage(std::string& msg)
{
	Process(msg, *this);
}

// -----------------------------
// class PipePassiveConn
// -----------------------------
class PipePassiveConn : public PipeConnection
{
public:
	PipePassiveConn::PipePassiveConn(Service& service)
		:_service(service),_sendAck(true),PipeConnection(service._lipcLog)
	{}

	~PipePassiveConn();
	void start()
	{
		read_start();
		_service.addConn(this);
		printf("new pipe Passive Conn\n");
	}


	virtual void OnMessage(std::string& msg)
	{
		_service.Process(msg,*this);
	}

	virtual void onError( int error,const char* errorDescription)
	{
		close();
	}

	virtual void OnWrote(int status)
	{
		_sendAck = true;
	}

	virtual void OnClose()
	{
		_service.delConn(this);
		delete this;
	}

	bool	isAck(){return _sendAck;}

private:
	Service&	_service;
	bool		_sendAck;
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
	if (status != Handle::elpeSuccess)
	{
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

// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
const char* Handler::errDesc(ErrorCode code)
{
	switch(code)
	{
	case PARSING_ERROR:   return "Parse error";
	case INVALID_REQUEST: return "Invalid JSON-RPC request";
	case METHOD_NOT_FOUND:return "Method not found";
	case INVALID_PARAMS:  return "Invalid params";
	case INTERNAL_ERROR:  return "Internal error";
	default:
		break;
	}

	return "server error";
}

Handler::Handler()
{
	// add a RPC method that list the actual RPC methods contained in the Handler 
	_lastCSeq.set(1);
	Json::Value desc;
	desc["description"] = "List the RPC methods available";
	desc["parameters"] = Json::Value::null;
	desc["returns"] = "Object that contains description of all methods registered";

	AddMethod(new RpcMethod<Handler>(*this, &Handler::SystemDescribe, std::string("system.describe"), desc));
}

Handler::~Handler()
{
	// delete all objects from the list 
	for(std::list<IMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
		delete (*it);

	m_methods.clear();
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

void Handler::AddMethod(IMethod* method)
{
	m_methods.push_back(method);
}

void Handler::Addcb(int seqId,RpcCB cb,void* data)
{
	RpcCBInfo info;
	info.cb = cb;
	info.data = data;
	m_seqIds[seqId] = info;
}

void Handler::DeleteMethod(const std::string& name)
{
    // do not delete system defined method
	if(name == "system.describe")
		return;

	for(std::list<IMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		if((*it)->GetName() == name)
		{
			delete (*it);
			m_methods.erase(it);
			return;
		}
	}
}

void Handler::SystemDescribe(const Json::Value& params, Json::Value& result)
{
	Json::Value methods;
	result["jsonrpc"] = "2.0";

	for(std::list<IMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
		methods[(*it)->GetName()] = (*it)->GetDescription();

	result["result"] = methods;
}

void Handler::Process(const std::string& msgstr, PipeConnection& conn)
{
	Message msg;
	Message resp;

	bool parsing = msg.fromString(msgstr);
	resp.cseq = msg.cseq;
	if (!parsing)
	{
		Json::Value error;

		//TODO: error[xxx]=xxxx
		resp.params["error"] = error; 
		conn.send(resp.toString());
		return;
	}

	if(!msg.params.isArray())
	{
		if (invoke(msg.params[i], resp.params))
			conn.send(resp.toString());

		return;
	}

	switch (msg.param[XXX_FD])
	{
	case UDP:
		udpPump2* pump = new udpPump2(_engine,sess.sessId,ip.c_str(),port,sess.offset);
		pump->init(get_loop());
	case TCP
	}


	//  batched call
	for(size_t i = 0 ; i < msg.params.size() ; i++)
	{
		resp.cseq = msg.params[i][JSON_RPC_ID].asInt();
		if (resp.cseq <=0)
			resp.cseq = -1;

		resp.params = Json::Value::null;
		if (invoke(msg.params[i], resp.params))
			conn.send(resp.toString());
	}
}

//@return true if the result is needed to feed back to the peer thru the connection
bool Handler::invoke(const Json::Value& params, Json::Value& result)
{
	Json::Value err;

	// check the JSON-RPC version => 2.0
	if(!params.isObject() || !params.isMember("jsonrpc") || params["jsonrpc"] != "2.0") 
	{
		result["jsonrpc"] = "2.0";
		err["code"] = Handler::INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		result["error"] = err;
		return true;
	}

	std::string method = params[JSON_RPC_METHOD].asString();
	if (method != "")
	{
		// this is an incomming request
		IMethod* rpc = Lookup(method);
		if (!rpc)
			return false;

		rpc->Call(reqParams, result);
		return true;
	}

	// this is an incomming response
	int seqId = params[JSON_RPC_ID].asInt();
	if (seqId >= 0)
	{
		seqToCBInfoMap::iterator it = m_seqIds.find(seqId);
		if (m_seqIds.end() == it)
			return false;

		RpcCB cb = it->second.cb;
		void* data = it->second.data;
		if (cb!=NULL && data!=NULL)
			(*cb)(params, data);

#pragma message(__MSGLOC__"TODO: this is a bug to erase CB here")
		m_seqIds.erase(it);
		return false;
	}

	// forge an error response
	result["jsonrpc"] = "2.0";

	err["code"] = Handler::METHOD_NOT_FOUND;
	err["message"] = "Method not found";
	result["error"] = err;

	return true;
}

bool Handler::Check(const Json::Value& params, Json::Value& error)
{
	Json::Value err;

	// check the JSON-RPC version => 2.0
	if (!params.isObject() || !params.isMember("jsonrpc") || params["jsonrpc"] != "2.0") 
	{
		error["id"] = Json::Value::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	if (params.isMember("id") && (params["id"].isArray() || params["id"].isObject()))
	{
		error["id"] = Json::Value::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	// extract "method" attribute
	if (!params.isMember("method") || !params["method"].isString())
	{
		error["id"] = Json::Value::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	return true;
}

IMethod* Handler::Lookup(const std::string& name) const
{
	for(std::list<IMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		if((*it)->GetName() == name)
			return (*it);
	}

	return NULL;
}

}}//ZQ::LIPC
