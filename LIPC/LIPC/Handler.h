#ifndef __ZQ_JSONRPC_HANDLER_H__
#define __ZQ_JSONRPC_HANDLER_H__


#include "ZQ_common_conf.h"

#ifdef ZQ_OS_MSWIN
#  ifdef LIPC_EXPORTS
#    define ZQ_LIPC_API __EXPORT
#  else
#    define ZQ_LIPC_API __DLLRTL
#  endif
#else
#  define ZQ_LIPC_API
#endif // OS


#include "json/json.h"
#include "Pointer.h"
#include <list>

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

namespace ZQ {
	namespace LIPC {

class ZQ_LIPC_API Handler;
class PipeConnection;
typedef Json::Value Arbitrary;
// -----------------------------
// interface CallbackMethod
// -----------------------------
class CallbackMethod
{
public:
	virtual ~CallbackMethod(){}

	virtual void Call(const Arbitrary& msg, PipeConnection& conn) = 0;

    virtual std::string GetName() const = 0;

    virtual Arbitrary GetDescription() const = 0;
};

// -----------------------------
// class RpcMethod
// -----------------------------
template<class T> class RpcMethod : public CallbackMethod
{
public:
    typedef void (T::*Method)(const Arbitrary& msg,PipeConnection& conn);

    RpcMethod(T& obj, Method method, const std::string& name,const Arbitrary description = Arbitrary::null)
		:m_obj(&obj),
		m_name(name),
		m_method(method),
		m_description(description)
    {
    }

	virtual void Call(const Arbitrary& msg, PipeConnection& conn){
		(m_obj->*m_method)(msg, conn);
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
	typedef void (*RpcCB)(const Arbitrary& resp,void* data);
	typedef struct{
		RpcCB cb;
		void* data;
	}RpcCBInfo;
	typedef std::map<int,RpcCBInfo> seqToCBInfoMap;
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

	void Addcb(int seqId,RpcCB cb,void* data);

	void DeleteMethod(const std::string& name);

	void SystemDescribe(const Arbitrary& msg, PipeConnection& conn);

	void Process(const std::string& msg,PipeConnection& conn);

	CallbackMethod* Lookup(const std::string& name) const;
	uint lastCSeq();

private:
	Handler(const Handler& obj);
	Handler& operator=(const Handler& obj);

	std::list<CallbackMethod*> m_methods;

	void Process(const Arbitrary& root,PipeConnection& conn);

	bool Check(const Arbitrary& root, Arbitrary& error);

	Json::Reader m_reader;
	seqToCBInfoMap m_seqIds;
	
	ZQ::common::AtomicInt _lastCSeq;
};

}}
#endif
