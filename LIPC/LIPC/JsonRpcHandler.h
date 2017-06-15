#ifndef __ZQ_JSONRPC_HANDLER_H__
#define __ZQ_JSONRPC_HANDLER_H__

#include "json.h"
#include "Pointer.h"
#include <list>

namespace ZQ {
	namespace LIPC {

// -----------------------------
// interface CallbackMethod
// -----------------------------
class CallbackMethod
{
public:
	virtual ~CallbackMethod(){}

    virtual bool Call(const Json::Value& msg, Json::Value& response) = 0;

    virtual std::string GetName() const = 0;

    virtual Json::Value GetDescription() const = 0;
};

// -----------------------------
// class RpcMethod
// -----------------------------
template<class T> class RpcMethod : public CallbackMethod
{
public:
    typedef bool (T::*Method)(const Json::Value& msg,Json::Value& response);

    RpcMethod(T& obj, Method method, const std::string& name,const Json::Value description = Json::Value::null)
		:m_obj(&obj),
		m_name(name),
		m_method(method),
		m_description(description)
    {
    }

    virtual bool Call(const Json::Value& msg, Json::Value& response){
		return (m_obj->*m_method)(msg, response);
    }

    virtual std::string GetName() const{
		return m_name;
	}

    virtual Json::Value GetDescription() const{
		return m_description;
    }

private:
	//private to avoid copy
    RpcMethod(const RpcMethod& obj);
    RpcMethod& operator=(const RpcMethod& obj);

    Json::Reader m_reader;
    T* m_obj;
    Method m_method;
    std::string m_name;
    Json::Value m_description;
};
/*
// -------------------------------------------------
// class JsonRpcMessage
// -------------------------------------------------
class JsonRpcMessage: public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<JsonRpcMessage> Ptr;
	JsonRpcMessage();
	~JsonRpcMessage();

	void setMethod(std::string method);
	void setid(int id);
	template<typename T>
	void setParam(T param)
	{
		Json::Value sum(param);
		m_param.append(&sum);
	}
	void parse(const char* data, size_t size);
	std::string toRaw();

private:
	Json::Reader		m_reader;
	Json::FastWriter	m_writer;
	Json::Value			m_value;
	Json::Value			m_param;
};
*/

// ---------------------------------------------------
// class Handler
// ---------------------------------------------------
class Handler
{
public:
	enum ErrorCode
	{
		PARSING_ERROR = -32700, /**< Invalid JSON. An error occurred on the server while parsing the JSON text. */
		INVALID_REQUEST = -32600, /**< The received JSON not a valid JSON-RPC Request. */
		METHOD_NOT_FOUND = -32601, /**< The requested remote-procedure does not exist / is not available. */
		INVALID_PARAMS = -32602, /**< Invalid method parameters. */
		INTERNAL_ERROR = -32603 /**< Internal JSON-RPC error. */
	};
	static const char* errDesc(ErrorCode code)
	{
		switch(code)
		{
			case PARSING_ERROR:return "Parse error.";
			case INVALID_REQUEST:return "Invalid JSON-RPC request.";
			case METHOD_NOT_FOUND:return "Method not found.";
			case INVALID_PARAMS:return "Invalid params.";
			case INTERNAL_ERROR:return "Internal error.";
			default:return "server error.";
		}
	}
public:
	Handler();
	virtual ~Handler();

	void AddMethod(CallbackMethod* method);

	void DeleteMethod(const std::string& name);

	bool Process(const std::string& msg, Json::Value& response);

	bool Process(const char* msg, Json::Value& response);

	bool SystemDescribe(const Json::Value& msg, Json::Value& response);

	std::string GetString(Json::Value value);

private:

	Handler(const Handler& obj);
	Handler& operator=(const Handler& obj);

	Json::Reader m_reader;

	Json::FastWriter m_writer;

	std::list<CallbackMethod*> m_methods;

	CallbackMethod* Lookup(const std::string& name) const;

	bool Check(const Json::Value& root, Json::Value& error);

	bool Process(const Json::Value& root, Json::Value& response);
};

}}
#endif