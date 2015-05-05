#ifndef __CRM_Interface_HTTP_H__
#define __CRM_Interface_HTTP_H__
#include <ZQ_common_conf.h>
#include <Log.h>
#include <map>
#include <string>
namespace CRG
{
class IRequest
{
public:
	virtual ~IRequest(){}
    enum Method
        {
            UNKNOWN,
            GET,
			POST,
			PUT,
			M_DELETE
        };
    virtual Method method() const = 0;
    virtual const char* uri() const = 0;
    virtual const char* version() const = 0;
    virtual const char* queryArgument(const char* q) const = 0;
    virtual std::map<std::string, std::string> queryArguments() const = 0;
    virtual const char* header(const char* h) const = 0;

    virtual void getContent(std::string&) const = 0;
    
    virtual void getClientEndpoint(std::string& address, int& port) const = 0;

    virtual int64 getConnectionId() const = 0;
};

class IRequestWriter: public IRequest
{
public:
    virtual void setUri(const char* uri) = 0;
    virtual void setQueryArgument(const char* k, const char* v) = 0;
    virtual void setHeader(const char* hdr, const char* val) = 0;
    virtual void setContent(const char* data, size_t len) = 0;
};

class IResponse
{
public:
    // getter
	virtual ~IResponse(){}
    virtual int getStatusCode() const = 0;
    virtual const char* getReasonPhrase() const = 0;
    virtual const char* getHeader(const char* hdr) const = 0;
    virtual void getContent(std::string&) const = 0;

    // setter
    virtual void setStatus(int statusCode, const char* reasonPhrase = NULL) = 0;
    // set a NULL value to clear the header
    virtual void setHeader(const char* hdr, const char* val) = 0;
    virtual void setContent(const char* data, size_t len) = 0;
};

class IContentHandler
{
public:
	virtual ~IContentHandler(){}
    virtual void onRequest(const IRequest*, IResponse*) = 0;
};

class IFixupHandler
{
public:
	virtual ~IFixupHandler(){}
    // return true to continue, false to break
    virtual bool fixupRequest(IRequestWriter*)
    {
        return true;
    }

    // return true to continue, false to break
    virtual bool fixupResponse(const IRequest*, IResponse*)
    {
        return true;
    }
};


class ICRMManager
{
public:
	virtual ~ICRMManager(){}
    virtual void addRequestFixupHandler(IFixupHandler*) = 0;
    virtual void removeRequestFixupHandler(IFixupHandler*) = 0;
    virtual void addResponseFixupHandler(IFixupHandler*) = 0;
    virtual void removeResponseFixupHandler(IFixupHandler*) = 0;

    // Note: the registered uri here is a regular expression!
    virtual void registerContentHandler(const std::string& uri, IContentHandler*) = 0;
    virtual void unregisterContentHandler(const std::string& uri, IContentHandler*) = 0;

    /// get the config folder
    virtual const std::string& getConfigFolder() = 0;

    /// get the log folder
    virtual const std::string& getLogFolder() = 0;

    /// the logger instance provided by CRMMagager for 
    /// reporting emergency error during initialization.
    virtual ZQ::common::Log& superLogger() = 0;
};

#define token2str_(t) #t
#define token2str(t) token2str_(t)

#define CRM_Entry_Init CRMInit
typedef bool (*CRMInitEntry)(ICRMManager*);

#define CRM_Entry_Uninit CRMUninit
typedef void (*CRMUninitEntry)(ICRMManager*);
}
#endif

