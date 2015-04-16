#pragma once
#ifndef   _ALLOCATESERVER_H
#define  _ALLOCATESERVER_H
#include <Locks.h>
#include <SystemUtils.h>

#include "ScriptParser.h"
#include "ServerManager.h"
namespace  HttpImpl
{
typedef SYS::SingleObject  EVENT;
class AllocateServer : public AllcoCallback
{
public:
	  AllocateServer(ZQ::common::Log& log);
	  ~AllocateServer(void);
public:
	  virtual void onServerStop(ServerPtr ptr);
	  virtual void onDataResult(RESULTDATA& data);
	  bool init(const SessContext& ctx);
	   int run();
protected:
	 
	  FILENAMES getUrlList(const std::string& fileName, const std::string& url);
	  std::string   setUrl(const std::string& fileName, const std::string& url);
	  void writeData();

private:
	  ZQ::common::Mutex        _serverListLock;
	  std::vector <ServerPtr >  _serverList;
	  int									_serverStopNum;
	  bool                                _isRuning;
	  EVENT							     _wakeUp;
	  ZQ::common::Mutex        _resultDataLock;
	  std::vector <RESULTDATA>  _resultData;

	  ZQ::common::Log&			_log;
};

}
#endif
