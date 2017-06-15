#include "JsonRpcHandler.h"

namespace ZQ{
	namespace LIPC{
/*
// -------------------------------------------------
// class JsonRpcMessage
// -------------------------------------------------
JsonRpcMessage::JsonRpcMessage()
{

}

JsonRpcMessage::~JsonRpcMessage()
{

}

void JsonRpcMessage::setMethod(std::string method)
{
	m_value["method"] = method;
}
void JsonRpcMessage::setid(int id)
{
	m_value["id"] = id;
}

void JsonRpcMessage::parse(const char* data, size_t size)
{

}

std::string JsonRpcMessage::toRaw()
{

}
*/
Handler::Handler()
{
  /* add a RPC method that list the actual RPC methods contained in 
   * the Handler 
   */
	 Json::Value root;

	 root["description"] = "List the RPC methods available";
	 root["parameters"] = Json::Value::null;
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

bool Handler::SystemDescribe(const Json::Value& msg, Json::Value& response)
{
	Json::Value methods;
	response["jsonrpc"] = "2.0";
	response["id"] = msg["id"];

	for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		methods[(*it)->GetName()] = (*it)->GetDescription();
	}

	response["result"] = methods;
	return true;
}

std::string Handler::GetString(Json::Value value)
{
	return m_writer.write(value);
}

bool Handler::Check(const Json::Value& root, Json::Value& error)
{
	Json::Value err;

	/* check the JSON-RPC version => 2.0 */
	if(!root.isObject() || !root.isMember("jsonrpc") ||
		root["jsonrpc"] != "2.0") 
	{
		error["id"] = Json::Value::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	if(root.isMember("id") && (root["id"].isArray() || root["id"].isObject()))
	{
		error["id"] = Json::Value::null;
		error["jsonrpc"] = "2.0";

		err["code"] = INVALID_REQUEST;
		err["message"] = "Invalid JSON-RPC request.";
		error["error"] = err;
		return false;
	}

	/* extract "method" attribute */
	if(!root.isMember("method") || !root["method"].isString())
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

bool Handler::Process(const Json::Value& root, Json::Value& response)
{
	Json::Value error;
	std::string method;

	if(!Check(root, error))
	{
		response = error;
		return false;
	}

	method = root["method"].asString();

	if(method != "")
	{
		CallbackMethod* rpc = Lookup(method);
		if(rpc)
		{
			return rpc->Call(root, response);
		}
	}

	/* forge an error response */
	response["id"] = root.isMember("id") ? root["id"] : Json::Value::null;
	response["jsonrpc"] = "2.0";

	error["code"] = METHOD_NOT_FOUND;
	error["message"] = "Method not found.";
	response["error"] = error;

	return false;
}

bool Handler::Process(const std::string& msg, Json::Value& response)
{
	Json::Value root;
	Json::Value error;
	bool parsing = false;

	/* parsing */
	parsing = m_reader.parse(msg, root);

	if(!parsing)
	{
		/* request or batched call is not in JSON format */
		response["id"] = Json::Value::null;
		response["jsonrpc"] = "2.0";

		error["code"] = PARSING_ERROR;
		error["message"] = "Parse error.";
		response["error"] = error; 
		return false;
	}

	if(root.isArray())
	{
		/* batched call */
		Json::Value::ArrayIndex i = 0;
		Json::Value::ArrayIndex j = 0;

		for(i = 0 ; i < root.size() ; i++)
		{
			Json::Value ret;
			Process(root[i], ret);

			if(ret != Json::Value::null)
			{
				/* it is not a notification, add to array of responses */
				response[j] = ret;
				j++;
			}
		}
		return true;
	}
	else
	{
		return Process(root, response);
	}
}

bool Handler::Process(const char* msg, Json::Value& response)
{
	std::string str(msg);

	return Process(str, response);
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
	return 0;
}

}}