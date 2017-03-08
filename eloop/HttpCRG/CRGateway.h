#ifndef __CRGateway_H__
#define __CRGateway_H__
#include "HttpServer.h"
#include "selector.h"
#include "CRMInterface.h"
#include <list>
#include <vector>
namespace CRG{

// ---------------------------------------
// class CRMManager
// ---------------------------------------
class CRMManager: public ICRMManager
{
public:
	CRMManager(ZQ::common::Log& log);
	virtual ~CRMManager();
public:
	virtual void addRequestFixupHandler(IFixupHandler* h);
	virtual void removeRequestFixupHandler(IFixupHandler* h);
	virtual void addResponseFixupHandler(IFixupHandler* h);
	virtual void removeResponseFixupHandler(IFixupHandler* h);
	virtual void registerContentHandler(const std::string& uri, IContentHandler* h);
	virtual void unregisterContentHandler(const std::string& uri, IContentHandler* h);

	/// get the config folder
	virtual const std::string& getConfigFolder();

	/// get the log folder
	virtual const std::string& getLogFolder();

	/// the logger instance provided by CRMMagager for 
	/// reporting emergency error during initialization.
	virtual ZQ::common::Log& superLogger();
public:
	void setConfigFolder(const std::string& path);
	void setLogFolder(const std::string& path);
public:
	void processRequestSync(IRequestWriter* req, IResponse* resp);
	bool loadModule(const char* path);
	//    void freeModule(const char* path);
	void clearModules();
private:
	IContentHandler* findContentHandler(const std::string& uri);
private:
	ZQ::common::Log& _log;
	typedef std::list<IFixupHandler*> FixupHandlers;
	FixupHandlers _reqFixupHandlerStack;
	FixupHandlers _respFixupHandlerStack;
	selector<IContentHandler> _contentHandlerFinder;

	struct ModuleInfo
	{
		std::string path;
#ifdef ZQ_OS_MSWIN
		HMODULE hMod;
#else
		void* hMod;
#endif
		CRMInitEntry init;
		CRMUninitEntry uninit;
		ModuleInfo():hMod(NULL), init(NULL), uninit(NULL){}
	};

	typedef std::map< std::string, ModuleInfo > ModuleMap;
	ModuleMap _modMap;

	std::string _modConfFolder;
	std::string _modLogFolder;
};

// ---------------------------------------
// class ICaseLess
// ---------------------------------------
class ICaseLess: std::binary_function<std::string, std::string, bool> 
{
public:
	result_type operator()( const first_argument_type& a, const second_argument_type& b) const
	{
#ifdef ZQ_OS_MSWIN
		return (stricmp(a.c_str(), b.c_str()) < 0);
#else
		return (strcasecmp(a.c_str(), b.c_str()) < 0);
#endif
	}
};

// ---------------------------------------
// class RequestWriter
// ---------------------------------------
class RequestWriter:public IRequestWriter
{
public:
	RequestWriter();
	void init(const ZQ::eloop::HttpMessage::Ptr msg);
	void setClientEndpoint(const char* address, const int& port);

public:
	virtual Method method() const;
	virtual const char* uri() const;
	virtual const char* version() const;
	virtual const char* queryArgument(const char* q) const;
	virtual std::map<std::string, std::string> queryArguments() const;
	virtual const char* header(const char* h) const;

	virtual void getContent(std::string&) const;

	virtual void getClientEndpoint(std::string& address, int& port) const;

	virtual int64 getConnectionId() const;

	virtual void setUri(const char* uri);
	virtual void setQueryArgument(const char* k, const char* v);
	virtual void setHeader(const char* hdr, const char* val);
	virtual void setContent(const char* data, size_t len);


private:
	IRequest::Method _method;
	ZQ::eloop::HttpMessage::Ptr _msg;
	std::string _version;
	std::string _uri;
	std::map<std::string, std::string> _queryArgs;
	std::map<std::string, std::string, ICaseLess> _headers;
	std::string _content;

	std::string _clientAddress;
	int _clientPort;
	int64 _connId;
};

// ---------------------------------------
// class Response
// ---------------------------------------
class Response: public IResponse
{
public:
	Response(ZQ::eloop::HttpConnection& conn);
	bool init();
	void clear();
	void send();

public:
	// getter
	virtual int getStatusCode() const;
	virtual const char* getReasonPhrase() const;
	virtual const char* getHeader(const char* hdr) const;
	virtual void getContent(std::string&) const;

	// setter
	virtual void setStatus(int statusCode, const char* reasonPhrase = NULL);
	// set a NULL value to clear the header
	virtual void setHeader(const char* hdr, const char* val);
	virtual void setContent(const char* data, size_t len);

private:
	ZQ::eloop::HttpMessage::Ptr _msg;
	int _statusCode;
	std::string _reasonPhrase;
	std::map<std::string, std::string, ICaseLess> _headers;
	std::string _content;
	ZQ::eloop::HttpConnection& _conn;
};

// ---------------------------------------
// class ClientRequestHandler
// ---------------------------------------
class ClientRequestHandler: public ZQ::eloop::HttpHandler
{
public:
	ClientRequestHandler(ZQ::eloop::HttpConnection& conn,ZQ::common::Log& logger,const ZQ::eloop::HttpHandler::Properties& dirProps, const ZQ::eloop::HttpHandler::Properties& appProps,CRMManager* crmMgr);

	virtual bool onHeadersEnd( const ZQ::eloop::HttpMessage::Ptr msg);
	virtual bool onBodyData( const char* data, size_t size);
	virtual void onMessageCompleted();
	virtual void onParseError( int error,const char* errorDescription );

	virtual void	onHttpDataSent();
	virtual void	onHttpDataReceived( size_t size );

private:
	CRMManager* _crmMgr;
	RequestWriter _req;
	Response _resp;

	std::string _postedData;
};


// ---------------------------------------
// class CRHandlerFactory
// ---------------------------------------
class CRHandlerFactory: public ZQ::eloop::HttpBaseApplication
{
public:
	CRHandlerFactory(ZQ::common::Log& log, CRMManager* modMgr);
	virtual ~CRHandlerFactory();
public:
	//virtual ZQHttp::IRequestHandler* create(const char* uri);
	//virtual void destroy(ZQHttp::IRequestHandler* h);
	virtual ZQ::eloop::HttpHandler::Ptr create( ZQ::eloop::HttpConnection& conn,ZQ::common::Log& logger,const ZQ::eloop::HttpHandler::Properties& dirProps);
private:
	ZQ::common::Log& _log;
	CRMManager* _crmMgr;
	//ZQHttp::Util::AtomicCounter _nReq;
};


// ---------------------------------------
// class CRGateway
// ---------------------------------------
class CRGateway
{
public:
	CRGateway(ZQ::common::Log& log,ZQ::eloop::HttpServer::HttpServerConfig& conf);
	~CRGateway();

	void setModEnv(const std::string& confFolder, const std::string& logFolder);

	void addModule(const char* path);

	void start();

	void stop();
private:
	ZQ::common::Log& _log;
	ZQ::eloop::HttpServer* _httpserver;
	std::vector<std::string> _mods;
	CRMManager* _modMgr;
	CRHandlerFactory* _crHandlerFac;

	std::string _modConfFolder;
	std::string _modLogFolder;
};












} // namespace CRG
#endif

