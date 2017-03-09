#include "CRGateway.h"

#ifdef ZQ_OS_LINUX
extern "C"
{
#include <dlfcn.h>
}
#endif

namespace CRG{

// ---------------------------------------
// class CRMManager
// ---------------------------------------
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
	return NULL;
}

// ---------------------------------------
// class RequestWriter
// ---------------------------------------

RequestWriter::RequestWriter()
:_method(UNKNOWN)
{

}
void RequestWriter::init(const ZQ::eloop::HttpMessage::Ptr msg)
{
	if (msg == NULL)
		return;
	char ver[20];
	sprintf(ver,"%d%s%d",msg->versionMajor(),".",msg->versionMinor());
	_version= ver;

	_msg = msg;
	switch(msg->method())
	{
		case ZQ::eloop::HttpMessage::GET:
			_method = GET;
			break;
		case ZQ::eloop::HttpMessage::POST:
			_method = POST;
			break;
		case ZQ::eloop::HttpMessage::PUT:
			_method = PUT;
			break;
		case ZQ::eloop::HttpMessage::HTTPDELETE:
			_method = M_DELETE;
			break;
		default:
			_method = UNKNOWN;
			break;
	}

	_uri = msg->url();

	//_queryArgs = req->queryArguments();
	_headers.clear();
	_content.clear();
}

void RequestWriter::setClientEndpoint(const char* address, const int& port)
{
	_clientAddress = address;
	_clientPort = port;
}

IRequest::Method RequestWriter::method() const
{
	return _method;
}
const char* RequestWriter::uri() const
{
	return _uri.c_str();
}
const char* RequestWriter::version() const
{
	return _version.c_str();
}

const char* RequestWriter::queryArgument(const char* q) const
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

std::map<std::string, std::string> RequestWriter::queryArguments() const
{
	return _queryArgs;
}

const char* RequestWriter::header(const char* h) const
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
		return (_msg ? _msg->header(h).c_str() : NULL);
	}
}

void RequestWriter::getContent(std::string& content) const
{
	content.assign(_content);
}

void RequestWriter::getClientEndpoint(std::string& address, int& port) const
{
	address = _clientAddress;
	port = _clientPort;
}

int64 RequestWriter::getConnectionId() const 
{
	return _connId;
}


void RequestWriter::setUri(const char* uri)
{
	if(uri && '\0' != *uri)
	{
		_uri = uri;
	}
}

void RequestWriter::setQueryArgument(const char* k, const char* v)
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

void RequestWriter::setHeader(const char* hdr, const char* val)
{
	if(hdr)
	{
		if(val)
			_headers[hdr] = val;
		else
			_headers.erase(hdr);
	}
}

void RequestWriter::setContent(const char* data, size_t len)
{
	if(data)
	{
		_content.assign(data, len);
	}
}



// ---------------------------------------
// class Response
// --------------------------------------
Response::Response(ZQ::eloop::HttpConnection& conn,ZQ::common::Log& logger)
	:_msg(NULL),
	_statusCode(200),
	_Logger(logger),
	_conn(conn)
{

}

bool Response::init()
{
	clear();
	_msg = new ZQ::eloop::HttpMessage(ZQ::eloop::HttpMessage::MSG_RESPONSE);;

	return true;
}
void Response::send()
{
	if(_msg)
	{
		_msg->code(_statusCode);
		_msg->status(_reasonPhrase);

		_msg->keepAlive(true);
		_msg->header("Server","HttpCRG");
		_msg->header("Date",ZQ::eloop::HttpMessage::httpdate());
		// reset the content length
		_msg->contentLength(_content.size());

		std::map<std::string, std::string, ICaseLess>::const_iterator it;
		for(it = _headers.begin(); it != _headers.end(); ++it)
		{
			_msg->header(it->first.c_str(), it->second.c_str());
		}

		std::string senddata = _msg->toRaw();

		if(!_content.empty())
		{
			senddata.append(_content);
		}
		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(Response,"content length:[%d] size [%d] data:[%s]"),_content.length(),_content.size(),_content.c_str());

		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(Response,"send data:[%s]"),senddata.c_str());
		_conn.write(senddata.c_str(),senddata.size());
	}
}
void Response::clear()
{
	_msg = NULL;
	_statusCode = 200;
	_reasonPhrase.clear();
	_headers.clear();
	_content.clear();
}

int Response::getStatusCode() const
{
	return _statusCode;
}

const char* Response::getReasonPhrase() const
{
	return _reasonPhrase.c_str();
}

const char* Response::getHeader(const char* h) const
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

void Response::getContent(std::string& content) const
{
	content.assign(_content);
}

// setter
void Response::setStatus(int statusCode, const char* reasonPhrase)
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
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(Response,"setStatus:statusCode[%d],reasonPhrase[%s]"),_statusCode,_reasonPhrase.c_str());
}

// set a NULL value to clear the header
void Response::setHeader(const char* hdr, const char* val)
{
	if(hdr)
	{
		if(val)
			_headers[hdr] = val;
		else
			_headers.erase(hdr);
	}
}
void Response::setContent(const char* data, size_t len)
{
	if(data)
	{
		_content.assign(data, len);
	}
}


// ---------------------------------------
// class ClientRequestHandler
// ---------------------------------------

ClientRequestHandler::ClientRequestHandler(ZQ::eloop::HttpConnection& conn,ZQ::common::Log& logger,const ZQ::eloop::HttpHandler::Properties& dirProps, const ZQ::eloop::HttpHandler::Properties& appProps,CRMManager* crmMgr)
	:ZQ::eloop::HttpHandler(conn,logger,dirProps, appProps),
	_crmMgr(crmMgr),
	_resp(conn,logger)
{

}

bool ClientRequestHandler::onHeadersEnd( const ZQ::eloop::HttpMessage::Ptr msg)
{
	_req.init(msg);
	_resp.init();
	char ip[17] = {0};
	int port = 0;
	_conn.getpeerIpPort(ip,port);
	_req.setClientEndpoint(ip,port);
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ClientRequestHandler,"onHeadersEnd [%s:%d]"),ip,port);
	return true;
}
bool ClientRequestHandler::onBodyData( const char* data, size_t size)
{
	if(data)
	{
		_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ClientRequestHandler,"onBodyData [%s][%d]"),data,size);
		_postedData.append(data,size);
	}
	return true;
}
void ClientRequestHandler::onMessageCompleted()
{
	_Logger(ZQ::common::Log::L_INFO, CLOGFMT(ClientRequestHandler,"onMessageCompleted"));
	_req.setContent(_postedData.data(), _postedData.size());
	_crmMgr->processRequestSync(&_req, &_resp);
	// fill the response object
	_resp.send();
}
void ClientRequestHandler::onParseError( int error,const char* errorDescription )
{

}

void ClientRequestHandler::onHttpDataSent()
{

}
void ClientRequestHandler::onHttpDataReceived( size_t size )
{

}

// ---------------------------------------
// class CRHandlerFactory
// ---------------------------------------
CRHandlerFactory::CRHandlerFactory(ZQ::common::Log& log, CRMManager* modMgr)
	:_log(log),
	_crmMgr(modMgr)
{

}

CRHandlerFactory::~CRHandlerFactory()
{

}

ZQ::eloop::HttpHandler::Ptr CRHandlerFactory::create( ZQ::eloop::HttpConnection& conn,ZQ::common::Log& logger,const ZQ::eloop::HttpHandler::Properties& dirProps)
{ 
	return new ClientRequestHandler(conn,logger,dirProps,_appProps,_crmMgr); 
}

// ---------------------------------------
// class CRGateway
// ---------------------------------------
CRGateway::CRGateway(ZQ::common::Log& log,ZQ::eloop::HttpServer::HttpServerConfig& conf)
	:_log(log),
	_modMgr(NULL),
	_crHandlerFac(NULL),
	_httpserver(NULL)
{
	if(conf.threadCount <= 0)
		_httpserver = new ZQ::eloop::SingleLoopHttpServer(conf,log);
	else
		_httpserver = new ZQ::eloop::MultipleLoopHttpServer(conf,log);
}

CRGateway::~CRGateway()
{
	stop();
	if (_httpserver != NULL)
	{
		delete _httpserver;
		_httpserver = NULL;
	}
}

void CRGateway::setModEnv(const std::string& confFolder, const std::string& logFolder)
{
	_modConfFolder = confFolder;
	_modLogFolder = logFolder;

	// fixup the folder path, make sure the tail '/' is append
	if(!_modConfFolder.empty() && _modConfFolder[_modConfFolder.size() - 1] != FNSEPC)
	{
		_modConfFolder += FNSEPC;
	}

	if(!_modLogFolder.empty() && _modLogFolder[_modLogFolder.size() - 1] != FNSEPC)
	{
		_modLogFolder += FNSEPC;
	}
}

void CRGateway::addModule(const char* path)
{
	if(path)
		_mods.push_back(path);
}

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

	_httpserver->mount(".*", _crHandlerFac);
	_httpserver->startAt();
}

void CRGateway::stop()
{
	_httpserver->stop();

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


}