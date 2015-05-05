#include "AllocateServer.h"
#include <http.h>
#include <SystemInfo.h>
#ifdef ZQ_OS_LINUX
	  #include <sys/resource.h>
#endif
namespace  HttpImpl
{
AllocateServer::AllocateServer(ZQ::common::Log& log)
	  :_log(log), _serverStopNum(0)
{
#ifdef ZQ_OS_LINUX
	  struct rlimit  rl;
	  rl.rlim_cur = 64 * 1024;
	  rl.rlim_max = 640 * 1024;
	  if(setrlimit( RLIMIT_NOFILE, &rl) == 0)
	  {
			_log(ZQ::common::Log::L_NOTICE, CLOGFMT(AllocateServer, "set rlimit successful."));
	  }		
	  else
	  {
			int err = errno;
			_log(ZQ::common::Log::L_WARNING, CLOGFMT(AllocateServer, "set rlimit failed with error[%d]."), err);
	  }
#endif
	  int count = SystemInfo::getCpuCounts(&log);
	  if (count <= 0)
	  {
			count = 4;
	  }
	   LibAsync::HttpClient::setup(count);
}

AllocateServer::~AllocateServer(void)
{
	  LibAsync::HttpClient::teardown();
}

bool AllocateServer::init(const SessContext& ctx)
{
	  _isRuning = true;
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "init() enter."));
	  Pools::const_iterator  pIter = ctx.itermPool.begin();
	  for (; pIter != ctx.itermPool.end(); pIter++)
	  {
			if ( !(pIter->serverIP.empty()) && (pIter->port != 0) )
			{
				  ServerPtr serverPtr = new ServerManager(pIter->serverIP, pIter->port, pIter->clientNum, this, _log, pIter->interval, pIter->timeout);
				  Iterms::const_iterator  iIter = pIter->itermS.begin();
				  for (; iIter != pIter->itermS.end(); iIter ++)
				  {
						if ( 0 == iIter->type)
						{
							  std::string url = setUrl(iIter->name, iIter->url);
							  if ( !url.empty())
									serverPtr->setUrlStr(url);
						}
						else
						{
							  FILENAMES  fileList = getUrlList(iIter->name, iIter->url);
							  serverPtr->setUrlStr(fileList);
						}
				  }// for iIter
				  {
						ZQ::common::MutexGuard    gb(_serverListLock);
						_serverList.push_back(serverPtr);
				  }
				  serverPtr->start();
				  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "init() start server with [%s:%d]."), pIter->serverIP.c_str(), pIter->port);
			}//if
	  }//for pIter
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "init()  allocate server successful."));
	  return true;
}

FILENAMES AllocateServer::getUrlList(const std::string& fileName, const std::string& url)
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "getUrlList() use file [%s] to get url."), fileName.c_str());
	  FILENAMES  list;
	  FILE* fp = fopen(fileName.c_str(), "r");
	  if (fp == NULL)
			return list;
	  char buffer[128];
	  memset(buffer, '\0', 128);
	  while ( NULL != fgets(buffer, 128, fp) )
	  {
			std::string str(buffer);
			if (str.length() > 0)
				  str.erase(str.end() - 1);
			if ( str.empty() )
				  continue;
			std::string URL = setUrl(str, url);
			if( !URL.empty() )
				  list.push_back(URL);
			memset(buffer, '\0', 128);
	  }
	  fclose(fp);
	  return list;
}

std::string   AllocateServer::setUrl(const std::string& fileName, const std::string& url)
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "setUrl() use fileName [%s] and url [%s] to get URL."), fileName.c_str(), url.c_str());
	  std::string URL = url;
	  size_t  pos = URL.find("${file}");
	  if ( pos != std::string::npos)
	  {
			URL.replace(pos, sizeof("${file}") - 1, fileName);
			return URL;
	  }
	  return "";
}

void AllocateServer::onServerStop(ServerPtr ptr)
{
	  STATUS  stat;
	  ptr->getStatus(stat);
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "onServerStop() manager server [%s] stoped with [success:%d, runing:%d, failed:%d]."), ptr->getEndPoint().c_str(), stat.successClients, stat.runingClients, stat.failClients);
	  {
			ZQ::common::MutexGuard    gb(_serverListLock);
			for ( std::vector <ServerPtr >::iterator it = _serverList.begin(); it != _serverList.end(); it ++)
			{
				  if ( ptr == *it )
				  {
						_serverStopNum ++;
						break;
				  }
			}
	  }
	  _wakeUp.signal();
}

void AllocateServer::onDataResult(RESULTDATA& data)
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "onDataResult() get result data from manager server[%s]."), data.endPoint.c_str());
	  ZQ::common::MutexGuard    gb(_resultDataLock);
	  _resultData.push_back(data);
}

void AllocateServer::writeData()
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "writeData() start to write data."));
	  FILE*   fd = fopen("Result.bin", "w");
	  if (fd == NULL)
			return ;
	  for (std::vector <RESULTDATA>::const_iterator iter = _resultData.begin(); iter != _resultData.end(); iter ++)
	  {

			fprintf(fd,"Server:   \t\t[%s]\n", iter->endPoint.c_str());
			fprintf(fd,"success:  \t\t[%d]\n", iter->stat.successClients);
			fprintf(fd,"failed:   \t\t[%d]\n", iter->stat.failClients);
			fprintf(fd,"runing:   \t\t[%d]\n", iter->stat.runingClients);
			std::multimap<std::string, CLIENTDATA>::const_iterator avgIter = iter->clientData.begin();
			for (; avgIter != iter->clientData.end(); avgIter ++)
			{
				  CLIENTDATA da = avgIter->second;
				  fprintf(fd, "client:%-10s status:%-20s cdnssSessId:%-30s URL:%s ", avgIter->first.c_str(),  da.st.c_str(), da.sessId.c_str(), da.url.c_str());
				  fprintf(fd,"avBitrate:%-10d  toatalSize:%-10d\n", da.avgBitrate, da.totalRevSize);
			
			}
	  }
	  fclose(fd);
}

int AllocateServer::run()
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "run() enter."));
	  while (_isRuning)
	  {
			_wakeUp.wait(5000);
			{
				  ZQ::common::MutexGuard    gb(_serverListLock);
				  std::vector <ServerPtr >::const_iterator iter = _serverList.begin();
				  for (; iter != _serverList.end(); iter ++)
				  {
						STATUS  stat;
						std::string endPoint = (*iter)->getEndPoint();
						(*iter)->getStatus(stat);
						printf("server:   \t\t[%s]\n", endPoint.c_str());
						printf("success:  \t\t[%d]\n", stat.successClients);
						printf("failed:   \t\t[%d]\n", stat.failClients);
						printf("runing:   \t\t[%d]\n", stat.runingClients);
						printf("=================\n");
				}
				  if (_serverList.size() == _serverStopNum)
				  {
						_isRuning = false;
						std::vector <ServerPtr >::iterator it = _serverList.begin();
						for (; it != _serverList.end(); it ++)
							  *it = NULL;
						_serverList.clear();
				  }
			}	
	  }
	  SYS::sleep(10000);
	 writeData();
	 _log(ZQ::common::Log::L_DEBUG, CLOGFMT(AllocateServer, "run() exit , all server manager has stoped ,the program will stop."));
	  return 0;
}


}

