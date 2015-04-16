#include "ServerManager.h"

#include <TimeUtil.h>
namespace HttpImpl
{

ServerManager::ServerManager(const std::string& IP, uint port, uint client, AllcoCallback* base, ZQ::common::Log& log, uint interval, uint timeout)
	: _serverIP(IP), _port(port), _clientNum(client), _base(base), _log(log),  _interval(interval), _timeout(timeout)
{
}

ServerManager::~ServerManager(void)
{
}

void ServerManager::setUrlStr(const std::string& name)
{
	  FILENAMES  names;
	  names.push_back(name);
	  setUrlStr(names);
}

void ServerManager::setUrlStr(const FILENAMES& names)
{
// 	  for (int i = 0; i < names.size(); i++ )
// 			_fileList.push_back(names[i]);
	  _fileList.insert(_fileList.end(), names.begin(), names.end());
}

bool ServerManager::start()
{
	  _isRuning = true;
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "start() with [%s:%d]."), _serverIP.c_str(), _port);
	  {
			ZQ::common::MutexGuard  gd(_statusLock);
			_status.runingClients = 0;
			_status.failClients = 0;
			_status.successClients = 0;
	  }
	  for (int i = 0; i< _clientNum && i < _fileList.size(); i++)
	  {

			try{
				  LibAsync::C2DownLoaderPtr c2Ptr = new LibAsync::C2DownLoader(_log, this, _timeout);
				  {
						ZQ::common::MutexGuard  gb(_ptrFreeLock);
						_ptrFree.push_back(c2Ptr);
				  }
			}catch(...)
			{

			}
	  }
	  return NativeThread::start();
}

void ServerManager::stop()
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "stop() with [%s:%d]."), _serverIP.c_str(), _port);
	  _isRuning = false;
	  {
			ZQ::common::MutexGuard gd(_ptrFreeLock);
			while( !_ptrFree.empty() )
			{
				  *(_ptrFree.begin()) = NULL;
				  _ptrFree.erase(_ptrFree.begin());
			}
	  }
	  _serverWakup.signal();
}

bool ServerManager::getResultData()
{
	  LibAsync::BITRATE								preBitrates;
	  for (int i = 0; i< _c2DownloadData.size(); i++)
	  {		
			CLIENTDATA      da;
			da.avgBitrate = _c2DownloadData[i].avgBitrate;
			da.url = _c2DownloadData[i].url;
			da.sessId = _c2DownloadData[i].sessId;
			da.totalRevSize = _c2DownloadData[i].totalRevSize;
			if ( 0 == _c2DownloadData[i].clientStat )
				  da.st = "CLIENT_SUCCESS";
			else if ( 1 == _c2DownloadData[i].clientStat)
				  da.st = "CLIENT_FAILED";
			else
				  da.st = "CLIENT_RUNING";
			_data.clientData.insert(make_pair(_c2DownloadData[i].strAddr, da));
			//_data.avgBitrate += _c2DownloadData[i].avgBitrate;
			getPreBitrates(_c2DownloadData[i].preBitrate, preBitrates);

	  }
	  char buffer[128];
	  memset(buffer, '\0', 128);
	  snprintf(buffer, 128, "%s:%d", _serverIP.c_str(), _port);
	  _data.endPoint = buffer;
	  if ( !preBitrates.empty() )
	  {
			char timeBuffer[128];
			memset(timeBuffer, '\0', 128);
			ZQ::common::TimeUtil::TimeToUTC(preBitrates.begin()->first, timeBuffer, 128, true);
			_data.startTime = timeBuffer;
	  }
	  _data.stat = _status;
	  for (LibAsync::BITRATE::const_iterator iter = preBitrates.begin(); iter != preBitrates.end(); iter ++)
	  {
			_data.preBitrate.push_back(iter->second);
	  }
	  return true;
}

void ServerManager::getPreBitrates(const LibAsync::BITRATE& preBit, LibAsync::BITRATE& preBitrates)
{
	  LibAsync::BITRATE   resBit;
	  LibAsync::BITRATE::const_iterator preIter = preBit.begin();
	  LibAsync::BITRATE::const_iterator currIter = preBitrates.begin();
	  int64 time = 0;
	  if ( preBitrates.empty() && !preBit.empty() )
	  {
			time = preIter->first;
	  }
	  else if( preBit.empty() && ! preBitrates.empty())
	  {
			time = currIter->first;
	  }
	  else if ( preBit.empty() && preBitrates.empty() )
	  {
			time = 0;
	  }
	  else
	  {
			time = preIter->first > currIter->first ? currIter->first : preIter->first;
	  }
	  while( preIter != preBit.end() || currIter != preBitrates.end() )
	  {
			time += TIME_INTERVAL;
			int bits = 0, num = 0;
			while( preIter != preBit.end() )
			{
				  if ( preIter->first < time )
				  {
						bits += preIter->second;
						num ++;
						preIter ++;
				  }
				  else
				  {
						break;
				  }
			}// while 1
			if ( 0 != num )   bits = bits / num;
			else  bits = 0;
		    int bits2 = 0, num2 = 0;
			while( currIter != preBitrates.end() )
			{
				  if ( currIter->first < time )
				  {
						bits2+= currIter->second;
						num2 ++;
						currIter ++;
				  }
				  else
				  {
						break;
				  }
			}// while 2
			if ( 0 != num2 )  bits2 = bits2 / num2;
			else   bits2 = 0;
			resBit[time-(TIME_INTERVAL/2)] = bits + bits2;
	  }//while preIter != preBit.end() && currIter != _preBitrates.end()
	  preBitrates = resBit;
}

void ServerManager::onDateReady(LibAsync::C2DownLoaderPtr& pt, const LibAsync::HTTPDATA&  data)
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "onDateReady() sess[%s] ready."), data.strAddr.c_str());
	  {		
			ZQ::common::MutexGuard  gd(_c2DownloadDataLock);
			_c2DownloadData.push_back(data);
	  }
}

void ServerManager::onClientStop(LibAsync::C2DownLoaderPtr& pt, LibAsync::CLIENTSTATUS stat)
{
	  if ( NULL == pt)
			return;
	  char buffer[128];
	  memset(buffer, '\0', 128);
	  snprintf(buffer, 128, "%p", pt._ptr);
	  std::string addr = buffer;
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "onClientStop() sess[%s] stop enter."), addr.c_str());


	  if (LibAsync::CLIENT_FAILED == stat)
	  {

			ZQ::common::MutexGuard  gd(_statusLock);
			_status.runingClients --;
			_status.failClients ++;
	  }
	  else if ( LibAsync::CLIENT_SUCCESS == stat )
	  {
			ZQ::common::MutexGuard  gd(_statusLock);
			_status.runingClients --;
			_status.successClients ++;
	  }

 	  {
			ZQ::common::MutexGuard gd(_ptrListLock);
			for (std::vector<LibAsync::C2DownLoaderPtr>::iterator iter = _ptrList.begin(); iter != _ptrList.end(); iter++)
			{
				  if (pt == *iter)
				  {
						*iter = NULL;
						_ptrList.erase(iter);
						if (stat == LibAsync::CLIENT_SUCCESS)
						{
							  ZQ::common::MutexGuard gd(_ptrFreeLock);
							  _ptrFree.push_back(pt);
						}
						else
						{
							 LibAsync::C2DownLoaderPtr pt2 = new LibAsync::C2DownLoader(_log, this, _timeout);
							  ZQ::common::MutexGuard gd(_ptrFreeLock);
							  _ptrFree.push_back(pt2);
						}
						break;
				  }

			}
	  }
	  
	   _serverWakup.signal();
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "onClientStop() sess[%s] stop out."), addr.c_str());	 
}

int ServerManager::run()
{
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "run() enter."));
	  while(_isRuning)
	  {
			_serverWakup.wait(10000);
			while( !_fileList.empty() )
			{
				  LibAsync::C2DownLoaderPtr pt = NULL;
				  {
						ZQ::common::MutexGuard gd(_ptrFreeLock);
						if ( !_ptrFree.empty() )
						{
							  //pt = new LibAsync::C2DownLoader(_log, this, _timeout);
							  pt = *(_ptrFree.begin());
							  *(_ptrFree.begin()) = NULL;
							  _ptrFree.erase(_ptrFree.begin());
						}
				  }
				 if ( NULL == pt )
						break;
				 std::string url = _fileList.back();
				 _fileList.pop_back();
				 pt->setURI(url);
				 pt->setCommParam(_serverIP, _port);
				 {
					   ZQ::common::MutexGuard  gd(_statusLock);
					   _status.runingClients ++;
				 }
				 {
					   ZQ::common::MutexGuard  gb(_ptrListLock);
					   _ptrList.push_back(pt);
				 }
				 SYS::sleep(_interval);
				 pt->sendRequest();
				 
			}//while ( !_fileList.empty() )
// 			{
// 				  ZQ::common::MutexGuard gd(_ptrFreeLock);
// 				  while( !_ptrFree.empty() )
// 				  {
// 						*(_ptrFree.begin()) = NULL;
// 						_ptrFree.erase(_ptrFree.begin());
// 				  }
// 			}
			{
				  ZQ::common::MutexGuard gd(_ptrListLock);
				  if (_ptrList.empty() && _fileList.empty() )
							  stop();
			}
	  }
	  if (getResultData())
			_base->onDataResult(_data);
	  ServerPtr pt(this);
	  _base->onServerStop(pt);
	  _log(ZQ::common::Log::L_DEBUG, CLOGFMT(ServerManager, "run() exit[%s:%d]."), _serverIP.c_str(), _port);
	  return 0;
}

}
