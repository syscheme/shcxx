#ifndef __ZQ_HttpEngine_Interface_H__
#define __ZQ_HttpEngine_Interface_H__
#include <ZQ_common_conf.h>
#include <string>
#include <map>
#include <Pointer.h>
#include "../libasync/eventloop.h"

namespace ZQHttp
{
enum Method
    {
        UNKNOWN,
        GET,
        POST,
		PUT,
		M_DELETE,
		METHOD_MAX
    };

class IRequest
{
public:
    virtual ~IRequest(){}
    virtual Method method() const = 0;
    virtual const char* version() const = 0;
    virtual const char* uri() const = 0;
	virtual const char* getFullUri() const = 0;
    virtual const char* queryArgument(const char* q) const = 0;
    virtual std::map<std::string, std::string> queryArguments() const = 0;
    virtual const char* header(const char* h) const = 0;

	virtual LibAsync::EventLoop* getLoop() const { return NULL; }
};

class IChannelWritable : public ZQ::common::SharedObject {
public:
	typedef ZQ::common::Pointer<IChannelWritable> Ptr;
	virtual ~IChannelWritable(){}
	virtual void onWritable() = 0;
};

class IResponse
{
public:
    virtual ~IResponse(){}
    virtual void setStatus(int statusCode, const char* reasonPhrase = NULL) = 0;
    // set a NULL value to clear the header
    virtual void setHeader(const char* hdr, const char* val) = 0;
    virtual bool headerPrepared() = 0;
    virtual bool addContent(const char* data, size_t len) = 0;

	/// new method addBody does the same thing as addContent
	/// but here's some different between them
	/// 1. addBody do not guarantee the whole data will be transfered. if not, error code retuned
	virtual int addBody( const char* data, size_t len) {}
	/// register an event for writing data
	/// return false if failed to register
	virtual bool registerWrite( IChannelWritable::Ptr cb ) { return false; }

    virtual bool complete() = 0;

    virtual void sendDefaultErrorPage(int statusCode) = 0;
    virtual const char* getLastError() const = 0;
};

struct PostDataFrag
{
    const char* data;
    size_t len; // length of data
    bool partial;

    const char* name; // post data name, may be NULL
    const char* fileName; // for file upload
    const char* contentType;
    const char* transferEncoding;
    PostDataFrag()
        :data(0), len(0), partial(false), name(NULL), 
         fileName(NULL), contentType(NULL), transferEncoding(NULL)
    { // clear all fields
    }
};

#define ZQHttp_OPT_NoDelay  1 // Bool
#define ZQHttp_OPT_HoldOn   2 // Bool
#define ZQHttp_OPT_WriteBufSize 3 // Size
#define ZQHttp_OPT_ReadBufSize  4 // Size
#define ZQHttp_OPT_sendTimeo 5 //send timeout in milliseond

class IConnection
{
public:
    virtual ~IConnection(){}
    virtual bool getLocalEndpoint(std::string& address, int& port) const = 0;
    virtual bool getRemoteEndpoint(std::string& address, int& port) const = 0;
    virtual int64 getId() const = 0;
    virtual bool setCommOption(int opt, int val) = 0;
};

class IRequestHandler
{
public:
    virtual ~IRequestHandler(){}
    /// on*() return true for continue, false for break.
    virtual bool onConnected(IConnection&){ return true; }
    virtual bool onRequest(const IRequest& req, IResponse& resp) = 0;
    virtual bool onPostData(const PostDataFrag& frag) { return true; }
    virtual bool onPostDataEnd() { return true; }
    virtual void onRequestEnd() = 0;

    // break the current request processing
    virtual void onBreak() = 0;
};

class IRequestHandlerFactory
{
public:
    virtual ~IRequestHandlerFactory(){}
    virtual IRequestHandler* create(const char* uri) = 0;
    virtual void destroy(IRequestHandler*) = 0;
};

} // namespace ZQHttp

#endif

