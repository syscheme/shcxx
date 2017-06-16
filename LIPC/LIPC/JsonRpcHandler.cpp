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
// -------------------------------------------------
// class Request
// -------------------------------------------------
Request::Request(Arbitrary id,Arbitrary methodname,int argc, Arbitrary argv,...)
:m_request(Arbitrary::null)
{
	append(id,methodname,argc,argv);
}

Request::Request():m_request(Arbitrary::null){}
Request::~Request(){}

void Request::append(Arbitrary id,Arbitrary methodname,int argc, Arbitrary argv,...)
{
	Arbitrary tempValue;
	if (argc > 0)
	{
		va_list arg_ptr;  
		Arbitrary nArgValue,params;  
		va_start(arg_ptr, argv);
		for(int i=0;i<argc;i++)
		{
			nArgValue = va_arg(arg_ptr, Arbitrary);
			params.append(nArgValue);
		}
		va_end(arg_ptr);
		tempValue[JSON_RPC_PARAMS] = params;
	}

	tempValue[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	tempValue[JSON_RPC_METHOD] = methodname;
	tempValue[JSON_RPC_ID] = id;
	m_request.append(tempValue);
}

std::string Request::toRaw()
{
	return m_writer.write(m_request);
}

// -------------------------------------------------
// class PassiveReq
// -------------------------------------------------
PassiveReq::PassiveReq(Arbitrary req)
:m_request(req)
{
}
PassiveReq::~PassiveReq()
{
}
Arbitrary PassiveReq::id()
{
	return m_request[JSON_RPC_ID];
}
Arbitrary PassiveReq::method()
{
	return m_request[JSON_RPC_METHOD];
}
Arbitrary PassiveReq::param()
{
	return m_request[JSON_RPC_PARAMS];
}

// -------------------------------------------------
// class Respon
// -------------------------------------------------
Respon::Respon()
:m_Result(Arbitrary::null),
m_Error(Arbitrary::null)
{

}
Respon::~Respon(){}

void Respon::setResult(Arbitrary id,Arbitrary result)
{
	m_Result[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	m_Result[JSON_RPC_ID] = id;
	m_Result[JSON_RPC_RESULT] = result;
}
void Respon::setError(Arbitrary id,Arbitrary code,Arbitrary desc)
{
	Arbitrary error;
	error[JSON_RPC_ERROR_CODE] = code;
	error[JSON_RPC_ERROR_MESSAGE] = desc;

	m_Error[JSON_RPC_PROTO] = JSON_RPC_PROTO_VERSION;
	m_Error[JSON_RPC_ID] = id;
	m_Error[JSON_RPC_ERROR] = error;
}

std::string Respon::toRaw()
{
	if (m_Result != Arbitrary::null)
		return m_writer.write(m_Result);
	if (m_Error != Arbitrary::null)
		return m_writer.write(m_Error);
	return NULL;
}

Arbitrary Respon::getvalue()
{
	if (m_Result != Arbitrary::null)
		return m_Result;
	if (m_Error != Arbitrary::null)
		return m_Error;
	return Arbitrary::null;
}

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

bool Handler::SystemDescribe(const Arbitrary& msg, Arbitrary& response)
{
	Arbitrary methods;
	response["jsonrpc"] = "2.0";
	response["id"] = msg["id"];

	for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
	{
		methods[(*it)->GetName()] = (*it)->GetDescription();
	}

	response["result"] = methods;
	return true;
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

bool Handler::Process(const Arbitrary& root, Arbitrary& response)
{
	Arbitrary error;
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
	response["id"] = root.isMember("id") ? root["id"] : Arbitrary::null;
	response["jsonrpc"] = "2.0";

	error["code"] = METHOD_NOT_FOUND;
	error["message"] = "Method not found.";
	response["error"] = error;

	return false;
}

bool Handler::Process(const std::string& msg, Arbitrary& response)
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
		return false;
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
		return true;
	}
	else
	{
		return Process(root, response);
	}
}

bool Handler::Process(const char* msg, Arbitrary& response)
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