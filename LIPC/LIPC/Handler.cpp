#include "Handler.h"
#include "Locks.h"
#include "PipeConnection.h"
namespace ZQ{
	namespace LIPC{

// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
Handler::Handler()
{
  /* add a RPC method that list the actual RPC methods contained in 
   * the Handler 
   */
	_lastCSeq.set(1);
	 Arbitrary root;
	 root["description"] = "List the RPC methods available";
	 root["parameters"] = Arbitrary::null;
	 root["returns"] = "Object that contains description of all methods registered";

	 AddMethod(new RpcMethod<Handler>(*this, &Handler::SystemDescribe,std::string("system.describe"), root));
}

Handler::~Handler()
{
	// delete all objects from the list 
	for(std::list<CallbackMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		delete (*it);
	}
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

void Handler::AddMethod(CallbackMethod* method)
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
    /* do not delete system defined method */
	if(name == "system.describe")
		return;

	for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		if((*it)->GetName() == name)
		{
			delete (*it);
			m_methods.erase(it);
			break;
		}
	}
}

void Handler::SystemDescribe(const Arbitrary& msg,PipeConnection& conn)
{
	Arbitrary response;
	Arbitrary methods;
	response["jsonrpc"] = "2.0";
	response["id"] = msg["id"];

	for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		methods[(*it)->GetName()] = (*it)->GetDescription();
	}

	response["result"] = methods;

}

void Handler::Process(const std::string& msg,PipeConnection& conn)
{
	Arbitrary root;
	Arbitrary response;
	Arbitrary error;
	bool parsing = false;

	printf("msg:%s\n",msg.c_str());
	/* parsing */
	parsing = m_reader.parse(msg, root);

	if(!parsing)
	{
		/* request or batched call is not in JSON format */
		response["id"] = Arbitrary::null;
		response["jsonrpc"] = "2.0";

		error["code"] = Handler::PARSING_ERROR;
		error["message"] = "Parse error.";
		response["error"] = error; 
	}

	if(root.isArray())
	{
		/* batched call */
		Arbitrary::ArrayIndex i = 0;
		Arbitrary::ArrayIndex j = 0;

		for(i = 0 ; i < root.size() ; i++)
		{
			Arbitrary ret;
			Process(root[i],conn);
		}
	}
	else
	{
		Process(root,conn);
	}
}

void Handler::Process(const Arbitrary& root,PipeConnection& conn)
{
	Arbitrary response = Arbitrary::null;
	std::string method;

	Arbitrary err;
	/* check the JSON-RPC version => 2.0 */
	if(!root.isObject() || !root.isMember("jsonrpc") ||
		root["jsonrpc"] != "2.0") 
	{
		response["id"] = Arbitrary::null;
		response["jsonrpc"] = "2.0";

		err["code"] = Handler::INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		response["error"] = err;
		conn.send(response);
		return;
	}

	method = root[JSON_RPC_METHOD].asString();

	if(method != "")
	{
		CallbackMethod* rpc = Lookup(method);
		if(rpc)
			rpc->Call(root,conn);
		return;
	}
	else
	{
		int seqId = root[JSON_RPC_ID].asInt();
		if (seqId >= 0)
		{
			seqToCBInfoMap::iterator it = m_seqIds.find(seqId);
			if (it != m_seqIds.end())
			{
				RpcCB cb = it->second.cb;
				void* data = it->second.data;
				if (cb!=NULL&&data!=NULL)
				{
					(*cb)(root,data);
				}
				m_seqIds.erase(it);
			}
			return;
		}
	}

	/* forge an error response */
	response["id"] = root.isMember("id") ? root["id"] : Arbitrary::null;
	response["jsonrpc"] = "2.0";

	err["code"] = Handler::METHOD_NOT_FOUND;
	err["message"] = "Method not found.";
	response["error"] = err;
	conn.send(response);
}

bool Handler::Check(const Arbitrary& root, Arbitrary& error)
{
	Arbitrary err;

	/* check the JSON-RPC version => 2.0 */
	if(!root.isObject() || !root.isMember("jsonrpc") ||
		root["jsonrpc"] != "2.0") 
	{
		error["id"] = Arbitrary::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	if(root.isMember("id") && (root["id"].isArray() || root["id"].isObject()))
	{
		error["id"] = Arbitrary::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	/* extract "method" attribute */
	if(!root.isMember("method") || !root["method"].isString())
	{
		error["id"] = Arbitrary::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	return true;
}

CallbackMethod* Handler::Lookup(const std::string& name) const
{
	for(std::list<CallbackMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		if((*it)->GetName() == name)
		{
			return (*it);
		}
	}
	return NULL;
}

}}
