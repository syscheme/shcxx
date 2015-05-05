#include "ClientRequestGw.h"
#include "selector.h"
#include "HttpUtil.h"
#include <algorithm>
#include <sstream>
#include "SystemUtils.h"

#ifdef ZQ_OS_LINUX
extern "C"
{
#include <dlfcn.h>
}
#endif

namespace CRG
{
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

class RequestWriter:public IRequestWriter
{
public:
    RequestWriter():_httpReq(NULL), _method(UNKNOWN), _clientPort(-1), _connId(-1){}

    void setConnectionInfo(const ZQHttp::IConnection* conn)
    {
        if(conn) {
            conn->getRemoteEndpoint(_clientAddress, _clientPort);
            _connId = conn->getId();
        }
    }

    bool init(const ZQHttp::IRequest* req)
    {
        if(!req)
            return false;

        _httpReq = req;
        switch(req->method())
        {
        case ZQHttp::GET:
            _method = GET;
            break;
        case ZQHttp::POST:
            _method = POST;
            break;
		case ZQHttp::PUT:
			_method = PUT;
			break;
		case ZQHttp::M_DELETE:
			_method = M_DELETE;
			break;
        default:
            _method = UNKNOWN;
            break;
        }
        _version = req->version();
        _uri = req->uri();
        _queryArgs = req->queryArguments();
        _headers.clear();
        _content.clear();

        return true;
    }
public:
    virtual Method method() const
    {
        return _method;
    }
    virtual const char* uri() const
    {
        return _uri.c_str();
    }
    virtual const char* version() const
    {
        return _version.c_str();
    }
    virtual const char* queryArgument(const char* q) const
    {
        if(NULL == q || '\0' == *q)
            return NULL;

        std::map<std::string, std::string>::const_iterator it = _queryArgs.find(q);
        if(it != _queryArgs.end())
        {
            return it->second.c_str();
        }
        else
        {
            return NULL;
        }
     }
    virtual std::map<std::string, std::string> queryArguments() const
    {
        return _queryArgs;
    }
    virtual const char* header(const char* h) const
    {
        if(NULL == h || '\0' == *h)
            return NULL;

        std::map<std::string, std::string, ICaseLess>::const_iterator it = _headers.find(h);
        if(it != _headers.end())
        {
            return it->second.c_str();
        }
        else
        {
            return (_httpReq ? _httpReq->header(h) : NULL);
        }
    }

    virtual void getContent(std::string& content) const
    {
        content.assign(_content);
    }

    virtual void getClientEndpoint(std::string& address, int& port) const
    {
        address = _clientAddress;
        port = _clientPort;
    }

    virtual int64 getConnectionId() const {
        return _connId;
    }

    virtual void setUri(const char* uri)
    {
        if(uri && '\0' != *uri)
        {
            _uri = uri;
        }
    }
    virtual void setQueryArgument(const char* k, const char* v)
    {
        if(k)
        {
            if(v)
            {
                _queryArgs[k] = v;
            }
            else
            {
                _queryArgs.erase(k);
            }
        }
    }
    virtual void setHeader(const char* hdr, const char* val)
    {
        if(hdr)
        {
            if(val)
                _headers[hdr] = val;
            else
                _headers.erase(hdr);
        }
    }
    virtual void setContent(const char* data, size_t len)
    {
        if(data)
        {
            _content.assign(data, len);
        }
    }
private:
    const ZQHttp::IRequest* _httpReq;
    Method _method;
    std::string _version;
    std::string _uri;
    std::map<std::string, std::string> _queryArgs;
    std::map<std::string, std::string, ICaseLess> _headers;
    std::string _content;

    std::string _clientAddress;
    int _clientPort;
    int64 _connId;
};

class Response: public IResponse
{
public:
    Response():_httpResp(NULL), _statusCode(200){}

    bool init(ZQHttp::IResponse* resp)
    {
        if(!resp)
            return false;

        clear();
        _httpResp = resp;

        return true;
    }
    void send()
    {
        if(_httpResp)
        {
            _httpResp->setStatus(_statusCode, _reasonPhrase.c_str());

            // reset the content length
            _headers["Content-Length"] = ZQHttp::Util::int2str(_content.size());

            std::map<std::string, std::string, ICaseLess>::const_iterator it;
            for(it = _headers.begin(); it != _headers.end(); ++it)
            {
                _httpResp->setHeader(it->first.c_str(), it->second.c_str());
            }
            _httpResp->headerPrepared();
            if(!_content.empty())
            {
                _httpResp->addContent(_content.data(), _content.size());
            }
            _httpResp->complete();
        }
    }
    void clear()
    {
        _httpResp = NULL;
        _statusCode = 200;
        _reasonPhrase.clear();
        _headers.clear();
        _content.clear();
    }

public:
    // getter
    virtual int getStatusCode() const
    {
        return _statusCode;
    }
    virtual const char* getReasonPhrase() const
    {
        return _reasonPhrase.c_str();
    }
    virtual const char* getHeader(const char* h) const
    {
        if(NULL == h || '\0' == *h)
            return NULL;
        std::map<std::string, std::string, ICaseLess>::const_iterator it = _headers.find(h);
        if(it != _headers.end())
        {
            return it->second.c_str();
        }
        else
        {
            return NULL;
        }
    }
    virtual void getContent(std::string& content) const
    {
        content.assign(_content);
    }

    // setter
    virtual void setStatus(int statusCode, const char* reasonPhrase = NULL)
    {
        _statusCode = statusCode;
        if(reasonPhrase)
        {
            _reasonPhrase = reasonPhrase;
        }
        else
        {
            _reasonPhrase.clear();
        }
    }

    // set a NULL value to clear the header
    virtual void setHeader(const char* hdr, const char* val)
    {
        if(hdr)
        {
            if(val)
                _headers[hdr] = val;
            else
                _headers.erase(hdr);
        }
    }
    virtual void setContent(const char* data, size_t len)
    {
        if(data)
        {
            _content.assign(data, len);
        }
    }
private:
    ZQHttp::IResponse* _httpResp;
    int _statusCode;
    std::string _reasonPhrase;
    std::map<std::string, std::string, ICaseLess> _headers;
    std::string _content;
};

///////////////////////////////////////////////
// declaration of CRMManager
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

///////////////////////////////////////////////
CRMManager::CRMManager(ZQ::common::Log& log)
:_log(log)
{
}
CRMManager::~CRMManager()
{
    clearModules();
}

void CRMManager::addRequestFixupHandler(IFixupHandler* h)
{
    if(h)
    {
        FixupHandlers::iterator it = std::find(_reqFixupHandlerStack.begin(), _reqFixupHandlerStack.end(), h);
        if(it == _reqFixupHandlerStack.end())
        {
            _reqFixupHandlerStack.push_front(h);
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "addRequestFixupHandler() handler %p has been pushed into the stack."), h);
        }
        else
        {
            _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "addRequestFixupHandler() handler %p is already in the stack."), h);
        }
    }
}
void CRMManager::removeRequestFixupHandler(IFixupHandler* h)
{
    if(h)
    {
        FixupHandlers::iterator it = std::find(_reqFixupHandlerStack.begin(), _reqFixupHandlerStack.end(), h);
        if(it != _reqFixupHandlerStack.end())
        {
            _reqFixupHandlerStack.erase(it);
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "removeRequestFixupHandler() handler %p has been removed from the stack."), h);
        }
        else
        {
            _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "removeRequestFixupHandler() handler %p not found."), h);
        }
    }
}
void CRMManager::addResponseFixupHandler(IFixupHandler* h)
{
    if(h)
    {
        FixupHandlers::iterator it = std::find(_respFixupHandlerStack.begin(), _respFixupHandlerStack.end(), h);
        if(it == _respFixupHandlerStack.end())
        {
            _respFixupHandlerStack.push_front(h);
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "addResponseFixupHandler() handler %p has been pushed into the stack."), h);
        }
        else
        {
            _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "addResponseFixupHandler() handler %p is already in the stack."), h);
        }
    }
}

void CRMManager::removeResponseFixupHandler(IFixupHandler* h)
{
    if(h)
    {
        FixupHandlers::iterator it = std::find(_respFixupHandlerStack.begin(), _respFixupHandlerStack.end(), h);
        if(it != _respFixupHandlerStack.end())
        {
            _respFixupHandlerStack.erase(it);
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "removeResponseFixupHandler() handler %p has been removed from the stack."), h);
        }
        else
        {
            _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "removeResponseFixupHandler() handler %p not found."), h);
        }
    }
}

void CRMManager::registerContentHandler(const std::string& uri, IContentHandler* h)
{
    if(h)
    {
        if(_contentHandlerFinder.set(uri, h))
        {
            _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "registerContentHandler() regeister handler %p for request of :%s."), h, uri.c_str());
        }
        else
        {
            _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "registerContentHandler() failed to register handler %p for request:%s."), h, uri.c_str());
        }
    }
}

void CRMManager::unregisterContentHandler(const std::string& uri, IContentHandler* h)
{
    if(_contentHandlerFinder.remove(uri))
    {
        _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "unregisterContentHandler() handler %p (for:%s) has been removed."), h, uri.c_str());
    }
    else
    {
        _log(ZQ::common::Log::L_WARNING, CLOGFMT(CRMManager, "unregisterContentHandler() handler %p (for:%s) not found."), h, uri.c_str());
    }
}

void CRMManager::processRequestSync(IRequestWriter* req, IResponse* resp)
{
   // fixup request
    FixupHandlers::iterator it;
    for(it = _reqFixupHandlerStack.begin(); it != _reqFixupHandlerStack.end(); ++it)
    {
        if(!(*it)->fixupRequest(req))
            break;
    }

    // content handling
    IContentHandler* contentHandler = findContentHandler(req->uri());
    if(contentHandler)
    {
        contentHandler->onRequest(req, resp);
    }
    else
    { // need a default request handler
        // 404
        resp->setStatus(404);
        std::ostringstream buf;
        buf << "<html><head><title>404</title></head><body>"
            << "<h3>404 Page Not Found</h3>"
            << "<div>No handler found to process the request: "
            << req->uri() << "</div></body></html>";
        std::string content = buf.str();
        resp->setContent(content.data(), content.size());
        resp->setHeader("Content-Type", "text/html");
        return;
    }

    // fixup response
    for(it = _respFixupHandlerStack.begin(); it != _respFixupHandlerStack.end(); ++it)
    {
        if(!(*it)->fixupResponse(req, resp))
            break;
    }
}

/// get the config folder
const std::string& CRMManager::getConfigFolder()
{
    return _modConfFolder;
}

/// get the log folder
const std::string& CRMManager::getLogFolder()
{
    return _modLogFolder;
}

/// the logger instance provided by CRMMagager for
/// reporting emergency error during initialization.
ZQ::common::Log& CRMManager::superLogger()
{
    return _log;
}

void CRMManager::setConfigFolder(const std::string& path)
{
    _modConfFolder = (path.empty() || path[path.size() - 1] == FNSEPC) ? path : (path + FNSEPC);
}
void CRMManager::setLogFolder(const std::string& path)
{
    _modLogFolder = (path.empty() || path[path.size() - 1] == FNSEPC) ? path : (path + FNSEPC);
}

#ifdef ZQ_OS_MSWIN
bool CRMManager::loadModule(const char* path)
{
    if(NULL == path || '\0' == *path)
        return false;

    if(_modMap.end() != _modMap.find(path))
    {
        // log and quit
        return false;
    }

    ModuleInfo mod;
    mod.path = path;
    mod.hMod = ::LoadLibrary(path);
    if(!mod.hMod)
    {
        DWORD err = ::GetLastError();
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Failed to load module %s. error code=%u"), path, err);
        return false;
    }

    // try get the init&uninit entry
    mod.init = (CRMInitEntry)::GetProcAddress(mod.hMod, token2str(CRM_Entry_Init));
    if(!mod.init)
    {
        DWORD err = ::GetLastError();
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Cannot find the entry " token2str(CRM_Entry_Init) " in %s. error code=%u"), path, err);
        ::FreeLibrary(mod.hMod);
        return false;
    }

    mod.uninit = (CRMUninitEntry)::GetProcAddress(mod.hMod, token2str(CRM_Entry_Uninit));
        
    if(!mod.uninit)
    {
        DWORD err = ::GetLastError();
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Cannot find the entry " token2str(CRM_Entry_Uninit) " in %s. error code=%u"), path, err);
        ::FreeLibrary(mod.hMod);
		return false;
    }

    bool inited = false;
    try
    {
        inited = mod.init(this);
    }
    catch(...)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Unexpected exception during initializing module %s"), path);
        inited = false;
    }

    if(inited) {
        _log(ZQ::common::Log::L_INFO, CLOGFMT(CRMManager, "loadModule() Module %s has been initialized successfully."), path); 
        _modMap[mod.path] = mod;
        return true;
    }
    else {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() failed to load Module %s."), path);
        ::FreeLibrary(mod.hMod);
        return false;
    }
}

#else
bool CRMManager::loadModule(const char* path)
{
    if(NULL == path || '\0' == *path)
        return false;

    if(_modMap.end() != _modMap.find(path))
    {
        // log and quit
        return false;
    }

    ModuleInfo mod;
    mod.path = path;
    mod.hMod = dlopen(path,RTLD_LAZY);
    if(!mod.hMod)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Failed to load module %s. error code=%d errmsg=%s"),
			path, errno, dlerror());
        return false;
    }

    // try get the init&uninit entry
    mod.init = (CRMInitEntry)dlsym(mod.hMod, token2str(CRM_Entry_Init));
    if(!mod.init)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Cannot find the entry " token2str(CRM_Entry_Init) " in %s. error code=%d"), path, errno);
        dlclose(mod.hMod);
        return false;
    }

    mod.uninit = (CRMUninitEntry)dlsym(mod.hMod, token2str(CRM_Entry_Uninit));
        
    if(!mod.uninit)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Cannot find the entry " token2str(CRM_Entry_Uninit) " in %s. error code=%d"), path, errno);
        dlclose(mod.hMod);
		return false;
    }

    bool inited = false;
    try
    {
        inited = mod.init(this);
    }
    catch(...)
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() Unexpected exception during initializing module %s"), path);
        inited = false;
    }

    if(inited)
    {
        _log(ZQ::common::Log::L_INFO, CLOGFMT(CRMManager, "loadModule() Module %s has been initialized successfully."), path);
        _modMap[mod.path] = mod;
        return true;
    }
    else
    {
        _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "loadModule() failed to load Module %s"), path); 
        dlclose(mod.hMod);
        return false;
    }
}

#endif

void CRMManager::clearModules()
{
    if(_modMap.empty())
    {
        _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "clearModuls() no modules"));
        return;
    }

    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "Enter clearModuls() total %d modules"), _modMap.size());
    for(ModuleMap::iterator it = _modMap.begin(); it != _modMap.end(); ++it)
    {
        ModuleInfo &mod = it->second;
        try
        {
            mod.uninit(this);
            _log(ZQ::common::Log::L_INFO, CLOGFMT(CRMManager, "module %s uninit OK"), it->first.c_str());
        }
        catch(...)
        {
            // log here
            _log(ZQ::common::Log::L_ERROR, CLOGFMT(CRMManager, "Exception caught during uninit module %s"), it->first.c_str());
        }
#ifdef ZQ_OS_MSWIN
        ::FreeLibrary(mod.hMod);
#else
		dlclose(mod.hMod);
#endif
    }
    _modMap.clear();
    _log(ZQ::common::Log::L_DEBUG, CLOGFMT(CRMManager, "clear all modules"));
}

IContentHandler* CRMManager::findContentHandler(const std::string& uri)
{
    return _contentHandlerFinder.get(uri);
}
//////////////////////////////////////////////
class ClientRequestHandler: public ZQHttp::IRequestHandler
{
public:
    ClientRequestHandler(CRMManager* crmMgr)
        :_crmMgr(crmMgr)
    {
    }
public:
    // on*() return true for continue, false for break.
    virtual bool onConnected(ZQHttp::IConnection& conn)
    {
        _req.setConnectionInfo(&conn);
        return true;
    }

    virtual bool onRequest(const ZQHttp::IRequest& req, ZQHttp::IResponse& resp)
    {
        _req.init(&req);
        _resp.init(&resp);
        return true;
    }
    virtual bool onPostData(const ZQHttp::PostDataFrag& frag)
    {
        if(frag.data)
        {
            _postedData.append(frag.data, frag.len);
        }
        return true;
    }
    virtual bool onPostDataEnd()
    {
        _req.setContent(_postedData.data(), _postedData.size());
        return true;
    }
    virtual void onRequestEnd()
    {
        _crmMgr->processRequestSync(&_req, &_resp);
        // fill the response object
        _resp.send();
    }

    // break the current request processing
    virtual void onBreak()
    {
    }
private:
    CRMManager* _crmMgr;
    RequestWriter _req;
    Response _resp;

    std::string _postedData;
};

////////////////////////////////////////
class CRHandlerFactory: public ZQHttp::IRequestHandlerFactory
{
public:
    CRHandlerFactory(ZQ::common::Log& log, CRMManager* modMgr);
    virtual ~CRHandlerFactory();
public:
    virtual ZQHttp::IRequestHandler* create(const char* uri);
    virtual void destroy(ZQHttp::IRequestHandler* h);
private:
    ZQ::common::Log& _log;
    CRMManager* _crmMgr;
    ZQHttp::Util::AtomicCounter _nReq;
};

CRHandlerFactory::CRHandlerFactory(ZQ::common::Log& log, CRMManager* modMgr)
    :_log(log), _crmMgr(modMgr)
{
}

CRHandlerFactory::~CRHandlerFactory()
{
    _log(ZQ::common::Log::L_INFO, CLOGFMT(CRHandlerFactory, "~CRHandlerFactory() waiting for all request processing complete..."));

    // block here if any handler is still in use
    int n = 0;
    unsigned long t = SYS::getTickCount();
    while(_nReq.get())
    {
        ++n;
        if(0 == (n % 100))
        {
            _log(ZQ::common::Log::L_INFO, CLOGFMT(CRHandlerFactory, "~CRHandlerFactory() %d requests still running after %u msec"), _nReq.get(), (SYS::getTickCount() - t));
        }
        SYS::sleep(1);
    }
    _log(ZQ::common::Log::L_INFO, CLOGFMT(CRHandlerFactory, "~CRHandlerFactory() No running requests now. Destroy the factory object."));
}

ZQHttp::IRequestHandler* CRHandlerFactory::create(const char* uri)
{
    ClientRequestHandler* h =  new ClientRequestHandler(_crmMgr);
    if(h)
    {
        _nReq.incr();
    }
    return h;
}

void CRHandlerFactory::destroy(ZQHttp::IRequestHandler* h)
{
    if(h)
    {
        delete h;
        _nReq.decr();
    }
}

//////////////////////////////////////////
void CRGateway::start()
{
    _modMgr = new CRMManager(_log);

    _modMgr->setConfigFolder(_modConfFolder);
    _modMgr->setLogFolder(_modLogFolder);

    // load modules
    for(std::vector<std::string>::const_iterator it = _mods.begin(); it != _mods.end(); ++it)
    {
        _modMgr->loadModule(it->c_str());
    }
    _crHandlerFac = new CRHandlerFactory(_log, _modMgr);

    _engine.registerHandler(".*", _crHandlerFac);
    _engine.start();
}
void CRGateway::stop()
{
    _engine.stop();

    if(_crHandlerFac)
    {
        delete _crHandlerFac;
        _crHandlerFac = NULL;
    }
    if(_modMgr)
    {
        // free modules
        _modMgr->clearModules();
        delete _modMgr;
        _modMgr = NULL;
    }
}
} // namespace CRG
