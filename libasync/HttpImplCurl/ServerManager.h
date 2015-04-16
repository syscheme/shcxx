#pragma once
#ifndef  SERVER_H
#define  SERVER_H

#define     TIME_INTERVAL   5000

#include <NativeThread.h>
#include <Pointer.h>
#include <vector>
#include <SystemUtils.h>
#include <Locks.h>
//#include <DownloadMgr.h>
#include <C2Downloader.h>

#include <map>

namespace HttpImpl
{
/*  define the allocate server call back*/
typedef  struct _statStruct{
		uint  successClients;
		uint  failClients;
		uint  runingClients;
} STATUS;

typedef struct _clientdata{
	  std::string    url;
	  int               totalRevSize;
	  std::string    sessId;
	  int	             avgBitrate;
	  std::string       st;
}CLIENTDATA;

typedef struct _resultdata{
	  _resultdata()
	  {
			startTime = "";
			endPoint = "";
			stat.failClients = 0;
			stat.runingClients = 0;
			stat.successClients = 0;
			clientData.clear();
			preBitrate.clear();
	  }
	  std::string   endPoint;
	  STATUS      stat;
	  std::string  startTime;

	  std::multimap<std::string, CLIENTDATA>      clientData;
	  std::vector<int >  preBitrate;
} RESULTDATA;

typedef std::vector<std::string >  FILENAMES;
class ServerManager;
typedef ZQ::common::Pointer<ServerManager>   ServerPtr;
typedef SYS::SingleObject Event; 
class AllcoCallback{
public:
	  virtual void onServerStop(ServerPtr ptr) = 0;
	  virtual  void onDataResult(RESULTDATA& data) = 0;
};

class ServerManager : public LibAsync::ServerCallBack, public ZQ::common::NativeThread, virtual public ZQ::common::SharedObject
{
public:
	  ServerManager(const std::string& IP, uint port, uint client, AllcoCallback* base, ZQ::common::Log& log, uint interval = 100, uint timeout = 3000);
	  ~ServerManager(void);
public:
	  void setUrlStr(const std::string& name);
	  void setUrlStr(const FILENAMES& names);
	  virtual bool start();
	  void  stop();
	  void getStatus(STATUS& stat){
			ZQ::common::MutexGuard ge(_statusLock);
			stat = _status;
	  }
	  std::string getEndPoint()
	  {
			char buffer[128];
			memset(buffer, '\0', 128);
			snprintf(buffer, 128, "%s:%d", _serverIP.c_str(), _port);
			return buffer;
	  }
public:
	  virtual void onDateReady(LibAsync::C2DownLoaderPtr& pt, const LibAsync::HTTPDATA&  data);
	  virtual  void onClientStop(LibAsync::C2DownLoaderPtr& pt, LibAsync::CLIENTSTATUS stat=LibAsync::CLIENT_SUCCESS);
	  
protected:
	  int run();
	  void getPreBitrates(const LibAsync::BITRATE& preBit, LibAsync::BITRATE& preBitrates);
	  bool getResultData();
private:
	  std::string							_serverIP;
	  uint									_port;
	  uint									_clientNum;
	  bool									_isRuning;
	  Event									_serverWakup;

 	  //ZQ::common::Mutex           _fileListLock;
	  FILENAMES							_fileList;
	  ZQ::common::Log&			_log;
	  AllcoCallback*						_base;
	  uint									_timeout;
	  uint                                    _interval;

	  ZQ::common::Mutex           _ptrListLock;
	  std::vector<LibAsync::C2DownLoaderPtr>           _ptrList;

 	  ZQ::common::Mutex           _ptrFreeLock;
 	  std::vector<LibAsync::C2DownLoaderPtr>           _ptrFree;

	  ZQ::common::Mutex			_statusLock;
	  STATUS								_status;
	  
	  ZQ::common::Mutex			_c2DownloadDataLock;
	  std::vector<LibAsync::HTTPDATA>    _c2DownloadData;
 
	  RESULTDATA						_data;
};
}
#endif

