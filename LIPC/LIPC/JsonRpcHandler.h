#ifndef __ZQ_JSONRPC_HANDLER_H__
#define __ZQ_JSONRPC_HANDLER_H__

#include "json/json.h"
#include "Pointer.h"
#include <list>



namespace ZQ {
	namespace LIPC {

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

typedef Json::Value Arbitrary;
// -----------------------------
// interface CallbackMethod
// -----------------------------
class CallbackMethod
{
public:
	virtual ~CallbackMethod(){}

    virtual bool Call(const Arbitrary& msg, Arbitrary& response) = 0;

    virtual std::string GetName() const = 0;

    virtual Arbitrary GetDescription() const = 0;
};

// -----------------------------
// class RpcMethod
// -----------------------------
template<class T> class RpcMethod : public CallbackMethod
{
public:
    typedef bool (T::*Method)(const Arbitrary& msg,Arbitrary& response);

    RpcMethod(T& obj, Method method, const std::string& name,const Arbitrary description = Arbitrary::null)
		:m_obj(&obj),
		m_name(name),
		m_method(method),
		m_description(description)
    {
    }

    virtual bool Call(const Arbitrary& msg, Arbitrary& response){
		return (m_obj->*m_method)(msg, response);
    }

    virtual std::string GetName() const{
		return m_name;
	}

    virtual Arbitrary GetDescription() const{
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
    Arbitrary m_description;
};

// -------------------------------------------------
// class Request
// -------------------------------------------------
class Request: public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<Request> Ptr;
public:
	Request(Arbitrary id,Arbitrary methodname,int argc=0, Arbitrary argv=Arbitrary::null,...);
	Request(Arbitrary id,Arbitrary methodname);
	Request();
	~Request();

	void append(Arbitrary id,Arbitrary methodname,int argc=0, Arbitrary argv=Arbitrary::null,...);
	std::string toRaw();

private:
	Json::FastWriter	m_writer;
	Arbitrary			m_request;
};


// -------------------------------------------------
// class PassiveReq
// -------------------------------------------------
class PassiveReq: public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<PassiveReq> Ptr;
public:
	PassiveReq(Arbitrary req);
	~PassiveReq();

	Arbitrary id();
	Arbitrary method();
	Arbitrary param();

private:
	Arbitrary			m_request;
};

// -------------------------------------------------
// class Respon
// -------------------------------------------------
class Respon: public ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<Respon> Ptr;
	Respon();
	~Respon();

	void setResult(Arbitrary id,Arbitrary result);
	void setError(Arbitrary id,Arbitrary code,Arbitrary desc);
	std::string toRaw();
	Arbitrary getvalue();
private:
	Json::FastWriter	m_writer;
	Arbitrary			m_Result;
	Arbitrary			m_Error;
};



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

	bool Process(const std::string& msg, Arbitrary& response);

	bool Process(const char* msg, Arbitrary& response);

	bool SystemDescribe(const Arbitrary& msg, Arbitrary& response);

	std::string GetString(Arbitrary value);

private:

	Handler(const Handler& obj);
	Handler& operator=(const Handler& obj);

	Json::Reader m_reader;

	Json::FastWriter m_writer;

	std::list<CallbackMethod*> m_methods;

	CallbackMethod* Lookup(const std::string& name) const;

	bool Check(const Arbitrary& root, Arbitrary& error);

	bool Process(const Arbitrary& root, Arbitrary& response);
};

}}
#endif