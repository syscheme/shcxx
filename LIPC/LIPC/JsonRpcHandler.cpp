#include "JsonRpcHandler.h"

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
	 Arbitrary root;

	 root["description"] = "List the RPC methods available";
	 root["parameters"] = Arbitrary::null;
	 root["returns"] = "Object that contains description of all methods registered";

	 AddMethod(new RpcMethod<Handler>(*this, &Handler::SystemDescribe,std::string("system.describe"), root));
}

Handler::~Handler()
{
	/* delete all objects from the list */
	for(std::list<CallbackMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		delete (*it);
	}
	m_methods.clear();
}

void Handler::AddMethod(CallbackMethod* method)
{
	m_methods.push_back(method);
}

void Handler::Addcb(std::string seqId,RpcCB cb)
{
	m_seqIds[seqId] = cb;
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

void Handler::SystemDescribe(const Arbitrary& msg, Arbitrary& response)
{
	Arbitrary methods;
	response["jsonrpc"] = "2.0";
	response["id"] = msg["id"];

	for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		methods[(*it)->GetName()] = (*it)->GetDescription();
	}

	response["result"] = methods;
}

std::string Handler::GetString(Arbitrary value)
{
	return m_writer.write(value);
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

void Handler::Process(const Arbitrary& root, Arbitrary& response)
{
	response = Arbitrary::null;
	std::string method;

	Arbitrary err;
	/* check the JSON-RPC version => 2.0 */
	if(!root.isObject() || !root.isMember("jsonrpc") ||
		root["jsonrpc"] != "2.0") 
	{
		response["id"] = Arbitrary::null;
		response["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		response["error"] = err;
		return;
	}

	method = root[JSON_RPC_METHOD].asString();

	if(method != "")
	{
		CallbackMethod* rpc = Lookup(method);
		if(rpc)
			rpc->Call(root, response);
		return;
	}
	else
	{
		std::string seqId = root[JSON_RPC_ID].asString();
		if (seqId != "")
		{
			RpcCB rpc = NULL;

			std::map<std::string,RpcCB>::iterator it = m_seqIds.find(seqId);
			if (it != m_seqIds.end())
			{
				rpc = it->second;
				m_seqIds.erase(it);
				if (rpc)
				{
					(*rpc)(root,this);
				}
			}
			return;
		}
	}

	/* forge an error response */
	response["id"] = root.isMember("id") ? root["id"] : Arbitrary::null;
	response["jsonrpc"] = "2.0";

	err["code"] = METHOD_NOT_FOUND;
	err["message"] = "Method not found.";
	response["error"] = err;
}

void Handler::Process(const std::string& msg, Arbitrary& response)
{
	Arbitrary root;
	Arbitrary error;
	bool parsing = false;

	/* parsing */
	parsing = m_reader.parse(msg, root);

	if(!parsing)
	{
		/* request or batched call is not in JSON format */
		response["id"] = Arbitrary::null;
		response["jsonrpc"] = "2.0";

		error["code"] = PARSING_ERROR;
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
			Process(root[i], ret);

			if(ret != Arbitrary::null)
			{
				/* it is not a notification, add to array of responses */
				response[j] = ret;
				j++;
			}
		}
	}
	else
	{
		 Process(root, response);
	}
}

void Handler::Process(const char* msg, Arbitrary& response)
{
	std::string str(msg);

	Process(str, response);
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

Handler::RpcCB Handler::find(const std::string& seqId) const
{
	std::map<std::string,RpcCB>::const_iterator it = m_seqIds.find(seqId);
	if (it != m_seqIds.end())
		return it->second;
	return NULL;
}

}}
