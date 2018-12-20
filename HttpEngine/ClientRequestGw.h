#ifndef __ClientRequestGateway_H__
#define __ClientRequestGateway_H__
#include "CRMInterface.h"
#include "HttpEngine.h"
#include <list>
#include <vector>
namespace CRG{

// forward declaration
class CRMManager;
class CRHandlerFactory;

/////////////////////////////////
class CRGateway
{
public:
    CRGateway(ZQ::common::Log& log)
        :_log(log), _engine(log)
    {
        _modMgr = NULL;
        _crHandlerFac = NULL;
    }
public:
    void setEndpoint(const std::string& host, const std::string& port)
    {
        _engine.setEndpoint(host, port);
    }

    void setCapacity(size_t nConcurrentThread, int maxPendingRequest = -1)
    {
        _engine.setCapacity(nConcurrentThread, maxPendingRequest);
    }
    void enableMessageDump(bool textMode, bool incomingMessage, bool outgoingMessage) {
        _engine.enableMessageDump(textMode, incomingMessage, outgoingMessage);
    }
    void setMaxConnections(int maxConnections) {
        _engine.setMaxConnections(maxConnections);
    }
    void setIdleTimeout(int idleTimeOut) {
        _engine.setIdleTimeout(idleTimeOut);
    }
    void setModEnv(const std::string& confFolder, const std::string& logFolder)
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

    void addModule(const char* path)
    {
        if(path)
            _mods.push_back(path);
    }
public:
    void start();
    void stop();
private:
    ZQ::common::Log& _log;
    ZQHttp::Engine _engine;
    std::vector<std::string> _mods;
    CRMManager* _modMgr;
    CRHandlerFactory* _crHandlerFac;

    std::string _modConfFolder;
    std::string _modLogFolder;
};
} // namespace CRG
#endif

