#include "DirConsumer.h"
#include "winbase.h"

namespace ZQ{
namespace common{

/************************************************************************/
/*         DirItem 类函数                                               */
/************************************************************************/
DirConsumer::DirItem::DirItem(const char* fileName, FileAction action)
: _fileAction(action)
{
	strnset(_fileName, 0, MAX_PATH);
	strncpy(_fileName, fileName, MAX_PATH - 1);
}

/************************************************************************/
/*          DirConsumer 类函数                                          */
/************************************************************************/
DirConsumer::DirConsumer(const char* monitorDirectory,
						 DWORD dirOperation,
						 bool bWatchSubdirectories,
						 bool bSnapshot,
						 bool bNotifyAsFileCome,
						 DWORD  DirMonitorInterval)
{
	glog(Log::L_DEBUG,  "DirConsumer::DirConsumer() ");
	strnset(_monitorDirectory, 0, MAX_PATH);
	initMembers(dirOperation, bWatchSubdirectories, bSnapshot, bNotifyAsFileCome);	
	_DirMonitorInterval = DirMonitorInterval;
    strncpy(_monitorDirectory, monitorDirectory, MAX_PATH - 1);

	if(_monitorDirectory[strlen(_monitorDirectory) -1] == '\\')
		_monitorDirectory[strlen(_monitorDirectory) -1] ='\0';

	glog(Log::L_DEBUG,  "DirConsumer _monitorDirectory <%s>", _monitorDirectory);
}

DirConsumer::DirConsumer(const WCHAR* monitorDirectory,
						 DWORD dirOperation,
						 bool bWatchSubdirectories,
						 bool bSnapshot,
						 bool bNotifyAsFileCome,
						 DWORD  DirMonitorInterval)
{
	glog(Log::L_DEBUG,  "DirConsumer::DirConsumer() ");
	strnset(_monitorDirectory, 0, MAX_PATH);
	initMembers(dirOperation, bWatchSubdirectories, bSnapshot, bNotifyAsFileCome);
	_DirMonitorInterval = DirMonitorInterval;
	WideCharToMultiByte(CP_ACP, 0, monitorDirectory, -1, _monitorDirectory, MAX_PATH - 1, NULL, NULL);
	
	glog(Log::L_DEBUG,  "DirConsumer _monitorDirectory <%s>", _monitorDirectory);
}

DirConsumer::~DirConsumer()
{
	MutexGuard guard(_mutex);

	glog(Log::L_DEBUG,  "DirConsumer::~DirConsumer()");

	closeHandles();
	
	if (_pDirMonitor)
	{
		glog(Log::L_DEBUG,  "DirConsumer::Delete DirMonitor pointer");
		delete _pDirMonitor;
		_pDirMonitor = NULL;
	}
	
	removeAll();
}

void DirConsumer::initMembers(DWORD dirOperation, bool bWatchSubdirectories, 
							  bool bSnapshot, bool bNotifyAsFileCome)
{
	_hStop = NULL;
	_hNotify = NULL;
	_interval = 2000;
	_pDirMonitor = NULL;
	_bSnapshot = bSnapshot;
	setDirOperation(dirOperation);
	_bNotifyAsFileCome = bNotifyAsFileCome;
	_notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME;
	_bWatchSubdirectories = bWatchSubdirectories;
}

void DirConsumer::setDirOperation(DWORD dirOperation)
{
	DWORD maxValue = DIR_ADD + DIR_DEL + DIR_MODIFY + DIR_REN_OLD + DIR_REN_NEW;
	_dirOperation = maxValue > dirOperation ? dirOperation : maxValue;
	
	glog(Log::L_DEBUG,  "DirConsumer _dirOperation <%d>", _dirOperation);
}

bool DirConsumer::configure()
{
	DWORD fileAttribute;
	fileAttribute = GetFileAttributesA(_monitorDirectory);
	if ((fileAttribute == ~0)) 
	{
		glog(Log::L_DEBUG,  "DirConsumer GetFileAttributesA error <%d>", GetLastError());
		return false;
	}
	else if (fileAttribute != FILE_ATTRIBUTE_DIRECTORY)
	{
		glog(Log::L_DEBUG,  "DirConsumer GetFileAttributesA value <%d>", fileAttribute);
		return false;
	}
	glog(Log::L_DEBUG,  "DirConsumer GetFileAttributesA <%s> is Directory", _monitorDirectory);
	return true;
}

bool DirConsumer::init(void)
{	
	_pDirMonitor = new DirMonitor(this, _DirMonitorInterval);
	if (!_pDirMonitor)
	{
		return false;
	}

	if (!openHandles())
	{
		return false;
	}
	if (!configure())
	{
		return false;
	}
	return true;
}

int DirConsumer::run(void)
{
	if (!_pDirMonitor->start())
	{
		return 0;
	}

	DWORD status;
	DWORD interval = _interval;
	bool bContinued = true;
	HANDLE handles[2] = {_hStop, _hNotify};
	
	while (bContinued)
	{
		status = WaitForMultipleObjects(2, handles, false, interval);
		switch(status)
		{
		case WAIT_OBJECT_0:
			if (_pDirMonitor)
			{
				_pDirMonitor->stop();
			}
			bContinued = false;
			break;

		case WAIT_OBJECT_0 + 1:
			processNotify();
			break;

		case WAIT_TIMEOUT:
			processTimeout();
			break;

		default:
			break;
		}
	}

	return 1;
}

/************************************************************************/
/*                针对不同的事件所对应的处理函数                        */
/************************************************************************/
bool DirConsumer::processNotify()
{
	char fileName[MAX_PATH];
	strnset(fileName, 0, MAX_PATH);
	DirItem::FileAction action;
	if (removeHead(fileName, &action))
	{
		if (!_bNotifyAsFileCome)
		{
			if (!isFileAccess(fileName))
			{
				addTail(fileName, action);
				return true;
			}
		}
//    	glog(Log::L_DEBUG,  "enter DirConsumer::processNotify() <%s> <%d>", fileName, action);	
		switch(action) 
		{
		case DirItem::ADD:
			processAdd(fileName);
			break;

		case DirItem::REMOVE:
			processDelete(fileName);
			break;

		case DirItem::MODIFY:
			processModify(fileName);
			break;

		case DirItem::RENAMEOLD:
			processRenameOld(fileName);
			break;

		case DirItem::RENAMENEW:
			processRenameNew(fileName);
			break;

		default:
			break;
		}
	}
	return true; 
}

/************************************************************************/
/*           对list的操作                                               */
/************************************************************************/
/************************************************************************/
/*           尾部添加一个表中不存在的                                   */
/************************************************************************/
bool DirConsumer::addTailExclusive(const char* fileName, DirItem::FileAction action)
{
	MutexGuard guard(_mutex);
	DirItemListIterator iter = _dirItemList.begin();
	for ( ; iter != _dirItemList.end(); iter ++)
	{
		if ( stricmp((*iter)->_fileName, fileName) == 0 )
		{
			glog(Log::L_DEBUG,  "DirConsumer addTailExclusive find <%s>", fileName);
			return true;
		}
	}

	glog(Log::L_DEBUG,  "DirConsumer addTailExclusive not find <%s>", fileName);
	if ( !addTail(fileName, action) )
	{
		return false;
	}
	return true;
}

/************************************************************************/
/*           头部添加                                                   */
/************************************************************************/
bool DirConsumer::addHead(const char* fileName, DirItem::FileAction action)
{
	MutexGuard guard(_mutex);

	DirItem* pNewItem = new DirItem(fileName, action);
	_dirItemList.push_front(pNewItem);
	glog(Log::L_DEBUG,  "DirConsumer addHead    <%s> <%d>", fileName, action);

	return true;
}

/************************************************************************/
/*           尾部添加                                                   */
/************************************************************************/
bool DirConsumer::addTail(const char* fileName, DirItem::FileAction action)
{
	MutexGuard guard(_mutex);

	DirItem* pNewItem = new DirItem(fileName, action);
	_dirItemList.push_back(pNewItem);
	glog(Log::L_DEBUG,  "DirConsumer addTail    <%s> <%d>", fileName, action);

	return true;
}

/************************************************************************/
/*           头部删除                                                   */
/************************************************************************/
bool DirConsumer::removeHead(OUT char* fileName, 
							 OUT DirItem::FileAction* action)
{
	MutexGuard guard(_mutex);

	if (_dirItemList.size() > 0)
	{
		DirItem* pTempItem = _dirItemList.front();
		if (fileName)
		{
			strcpy(fileName, pTempItem->_fileName);
		}
		if (action)
		{
			*action = pTempItem->_fileAction;
		}

		glog(Log::L_DEBUG,  "DirConsumer removeHead <%s> <%d> list size <%d>",
							pTempItem->_fileName, 
							pTempItem->_fileAction, 
							_dirItemList.size());
		
		
		_dirItemList.pop_front();
		glog(Log::L_DEBUG,  "pop_front()");
		
		if (pTempItem)
		{
			glog(Log::L_DEBUG,  "delete...");
			delete pTempItem;
			pTempItem = NULL;
		}
		glog(Log::L_DEBUG,  "deleted");

		return true;
	}

//	glog(Log::L_DEBUG,  "DirConsumer removeHead failed");
	return false;
}

/************************************************************************/
/*           尾部删除                                                   */
/************************************************************************/
bool DirConsumer::removeTail(OUT char* fileName, 
							 OUT DirItem::FileAction* action)
{
	MutexGuard guard(_mutex);

	if (_dirItemList.size() > 0)
	{
		DirItem* pTempItem = _dirItemList.back();
		if (fileName)
		{
			strcpy(fileName, pTempItem->_fileName);
		}
		if (action)
		{
			*action = pTempItem->_fileAction;
		}
		
		_dirItemList.pop_back();
		
		glog(Log::L_DEBUG,  "DirConsumer removeTail <%s> <%d> list size <%d>",
							pTempItem->_fileName, 
							pTempItem->_fileAction, 
							_dirItemList.size());
		
		if (pTempItem)
		{
			delete pTempItem;
			pTempItem = NULL;
		}
		return true;
	}

	//glog(Log::L_DEBUG,  "DirConsumer removeTail failed");
	return false;
}

/************************************************************************/
/*           头部获取                                                   */
/************************************************************************/
bool DirConsumer::getHead(OUT char* fileName, OUT DirItem::FileAction* action)
{
	MutexGuard guard(_mutex);

	if (_dirItemList.size() > 0)
	{
		DirItem* pTempItem = _dirItemList.back();
		if (fileName)
		{
			strcpy(fileName, pTempItem->_fileName);
		}
		if (action)
		{
			*action = pTempItem->_fileAction;
		}
		
		glog(Log::L_DEBUG,  "DirConsumer getHead    <%s> <%d>", 
							pTempItem->_fileName, 
							pTempItem->_fileAction);
		return true;
	}

	//glog(Log::L_DEBUG,  "DirConsumer getHead failed");
	return false;
}

/************************************************************************/
/*           尾部获取                                                   */
/************************************************************************/
bool DirConsumer::getTail(OUT char* fileName, OUT DirItem::FileAction* action)
{
	MutexGuard guard(_mutex);

	if (_dirItemList.size() > 0)
	{
		DirItem* pTempItem = _dirItemList.back();
		if (fileName)
		{
			strcpy(fileName, pTempItem->_fileName);
		}
		if (action)
		{
			*action = pTempItem->_fileAction;
		}
		glog(Log::L_DEBUG,  "DirConsumer getTail    <%s> <%d>", 
							pTempItem->_fileName, 
							pTempItem->_fileAction);
		return true;
	}
	
	//glog(Log::L_DEBUG,  "DirConsumer getTail failed");
	return false;
}

bool DirConsumer::removeFile(const char* fileName)
{
	MutexGuard guard(_mutex);
	DirItemListIterator iter = _dirItemList.begin();
	for ( ; iter != _dirItemList.end(); iter++)
	{
		if (stricmp((*iter)->_fileName, fileName) == 0)
		{
			DirItem* pTmpItem = *iter;
			_dirItemList.erase(iter);
			
			delete pTmpItem;
			pTmpItem = NULL;

			glog(Log::L_DEBUG,  "DirConsumer removeFile <%s>", fileName);
			return true;
		}
	}

	glog(Log::L_DEBUG,  "DirConsumer removeFile failed");
	return false;
}

void DirConsumer::removeAll()
{
	MutexGuard guard(_mutex);

	glog(Log::L_DEBUG,  "DirConsumer removeAll");

	char junk[MAX_PATH];
	DirItem::FileAction action;

	while (!isEmpty())
	{
		strnset(junk, 0, MAX_PATH);
		removeHead(junk, &action);
	}
}

bool DirConsumer::isFileAccess(const char* fileName)
{
	MutexGuard guard(_mutex);
	
	char fullFileName[MAX_PATH];
	strnset(fullFileName, 0, MAX_PATH);
	sprintf(fullFileName, "%s\\%s", _monitorDirectory, fileName);
	
	HANDLE hFile = NULL;
	hFile = CreateFile(fullFileName,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		int err = GetLastError();
		if (err == ERROR_SHARING_VIOLATION)
		{
			glog(Log::L_DEBUG,  "Other Process is using file <%s>", fullFileName);
			return false;
		}
	}
	CloseHandle(hFile);
	return true;
}

/************************************************************************/
/*            DirMonitor  类函数                                        */
/************************************************************************/
DirMonitor::DirMonitor(DirConsumer* dirConsumer,int interval)
:NativeThread(), 
 _hStop(NULL),
 _hNotify(NULL),
 _directoryEvent(NULL),
 _hSetCallback(NULL),
 _interval(interval),
 _pDirConsumer(dirConsumer),
 _bSetCallbackSucess(false)
{
	strnset(_buffer, 0, MAX_PATH);	
	_overlapped.hEvent = this;

}

DirMonitor::~DirMonitor()
{
	MutexGuard guard(_mutex);
	closeHandles();
	while (_initialContentsQueue.size() > 0)
	{
		_initialContentsQueue.pop();
	}
}

bool DirMonitor::configure()
{
	_directoryEvent = CreateFileA(_pDirConsumer->_monitorDirectory,
								  FILE_LIST_DIRECTORY,
								  FILE_SHARE_READ  | 
								  FILE_SHARE_WRITE | 
								  FILE_SHARE_DELETE,
								  NULL,
								  OPEN_EXISTING,
								  FILE_FLAG_BACKUP_SEMANTICS |
								  FILE_FLAG_OVERLAPPED,
								  NULL);
	if (_directoryEvent == INVALID_HANDLE_VALUE)
	{
		glog(Log::L_DEBUG,  "DirMonitor CreateFileA <%s> error <%d>", 
			 _pDirConsumer->_monitorDirectory, GetLastError());
		
		_bSetCallbackSucess = false;
		SetEvent(_hSetCallback);
		return false;
	}

	glog(Log::L_DEBUG,  "DirMonitor CreateFileA <%s> Success",
		 _pDirConsumer->_monitorDirectory);
	return true;
}

/************************************************************************/
/*                NativeThread vitural函数                              */
/************************************************************************/
bool DirMonitor::start()
{
	NativeThread::start();
	
/*	DWORD status = 0;
	status = WaitForSingleObject(_hSetCallback, INFINITE);
	if (status == WAIT_OBJECT_0)
	{
		if (_bSetCallbackSucess)
		{
			return true;
		}
	}*/
	return true;
}

bool DirMonitor::init(void)
{
	if (!openHandles())
	{
		return false;
	}

	if(!configure())
	{
		return false;
	}
	return true;
}

int DirMonitor::run(void)
{
	HANDLE handles[2] = {_hStop, _hNotify} ;

	bool bContinued = true;
	DWORD waitStatus = 0;
	DWORD interval = _interval;

	DWORD bytesReturned;
	if (!setMonitorChanges(bytesReturned))
	{
		return 0;
	}
	
	if (_pDirConsumer->_bSnapshot)
	{	
		if (!getDirectorySnapshot(_pDirConsumer->_monitorDirectory,
			"", "*.*"))
		{
			return 0;
		}
		
		if (!processDirectorySnapshot())
		{
			return 0;
		}
	}

	while (bContinued)
	{
		waitStatus = WaitForMultipleObjectsEx(2, handles, false, interval, true);
		switch(waitStatus)
		{
		case WAIT_OBJECT_0:
			bContinued = false;
			break;
		
		case WAIT_OBJECT_0 + 1:
			processNotify();
			break;

		case WAIT_TIMEOUT:
			processTimeout();
			break;

		default:
			break;
		}
	}
	
	return 1;
}

/*     事件的相应     */
bool DirMonitor::processNotify()
{
	glog(Log::L_DEBUG,  "DirMonitor processNotify");

	if (!getDirectorySnapshot(_pDirConsumer->_monitorDirectory, "", "*.*"))
	{
		return false;
	}

	if (!processDirectorySnapshot())
	{
		return false;
	}
	return true;
}

bool DirMonitor::processDirectorySnapshot()
{
	MutexGuard guard(_mutex);
	
	glog(Log::L_DEBUG,  "DirMonitor processDirectorySnapshot");

	char temp[MAX_PATH];

	int count = _initialContentsQueue.size();
	for (int i = 0; i<count; i++)
	{
		strnset(temp, 0, MAX_PATH);
		strncpy(temp, _initialContentsQueue.front().c_str(), MAX_PATH - 1);
		_initialContentsQueue.pop();
		_pDirConsumer->fileAdd(temp);
	}
	return true;
}

/************************************************************************/
/*                 私有成员函数                                         */
/************************************************************************/
bool DirMonitor::setMonitorChanges(DWORD& bytesReturned)
{
	memset(_buffer, 0, MAX_PATH);
	if (!ReadDirectoryChangesW(_directoryEvent,
							   _buffer,
							   MAX_PATH - 1,
							   _pDirConsumer->_bWatchSubdirectories,
							   _pDirConsumer->_notifyFilter,
							   &bytesReturned,
							   &_overlapped,
							   DirMonitor::HandleDirChanges))
	{
		glog(Log::L_DEBUG,  "DirMonitor  ReadDirectoryChangesW error <%d>", GetLastError());
		
		_bSetCallbackSucess = false;
		SetEvent(_hSetCallback);

		return false;
	}

	if (!_bSetCallbackSucess)
	{
		_bSetCallbackSucess = true;
		SetEvent(_hSetCallback);
	}
	return true;
}

VOID WINAPI
DirMonitor::HandleDirChanges(DWORD errorCode, DWORD bytes, LPOVERLAPPED pOverlapped)
{
	DirMonitor* pDirMon = (DirMonitor*)pOverlapped->hEvent;

	if (pDirMon)
	{		
		DWORD offset;
		FILE_NOTIFY_INFORMATION* pFni = (FILE_NOTIFY_INFORMATION*)pDirMon->_buffer;
		do 
		{
			offset = pFni->NextEntryOffset;
			pDirMon->processChangedFile(pFni);
			pFni = (FILE_NOTIFY_INFORMATION*)((BYTE*)pFni + offset);
		} 
		while(offset);

		// 重置命令
		DWORD byteReturned;
		memset(pDirMon->_buffer, 0, MAX_PATH);
		pDirMon->setMonitorChanges(byteReturned);
	}
}

bool DirMonitor::processChangedFile(FILE_NOTIFY_INFORMATION* pFni)
{
	bool bRet = false;

	char changeFilename[MAX_PATH];
	strnset(changeFilename, 0, MAX_PATH);

	WideCharToMultiByte(CP_ACP, 0, pFni->FileName, -1, changeFilename, pFni->FileNameLength, NULL, NULL);

//	glog(Log::L_DEBUG,  
//		"Enter DirMonitor::processChangedFile() <%s> <%d>", 
//					changeFilename, pFni->Action);
	switch(pFni->Action)
	{
	case FILE_ACTION_ADDED:
		if (_pDirConsumer && (_pDirConsumer->_dirOperation & DIR_ADD))
		{
			_pDirConsumer->fileAdd(changeFilename);
		}
		break;

	case FILE_ACTION_REMOVED:
		if (_pDirConsumer && (_pDirConsumer->_dirOperation & DIR_DEL))
		{
			_pDirConsumer->fileRemove(changeFilename);
		}
		break;

	case FILE_ACTION_MODIFIED:
		if (_pDirConsumer && (_pDirConsumer->_dirOperation & DIR_MODIFY))
		{
			_pDirConsumer->fileModify(changeFilename);
		}
		break;
	
	case FILE_ACTION_RENAMED_OLD_NAME:
		if (_pDirConsumer && (_pDirConsumer->_dirOperation & DIR_REN_OLD))
		{
			_pDirConsumer->fileRenameOld(changeFilename);
		}
		break;
	
	case FILE_ACTION_RENAMED_NEW_NAME:
		if (_pDirConsumer && (_pDirConsumer->_dirOperation & DIR_REN_NEW))
		{
			_pDirConsumer->fileRenameNew(changeFilename);
		}
		break;
	
	default:
		break;
	}
//	glog(Log::L_DEBUG,  
//		"Leaving DirMonitor::processChangedFile() <%s> <d>", 
//					changeFilename, pFni->Action);
	return bRet;
}

bool DirMonitor::getDirectorySnapshot(const char* baseDirectory, 
									  const char* additionalPath,
									  const char* filter)
{
	MutexGuard guard(_mutex);

	char currentPath[MAX_PATH];
	strnset(currentPath, 0, MAX_PATH);
    sprintf(currentPath, "%s\\", baseDirectory);
	glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot current path <%s>", currentPath);

	if (strcmp(additionalPath, ""))
	{
		strcat(currentPath, additionalPath);
		strcat(currentPath, "\\");
		glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot current path <%s>", currentPath);
	}
	strcat(currentPath, filter);
	glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot current path <%s>", currentPath);

	char totalFile[MAX_PATH];
	strnset(totalFile, 0, MAX_PATH);

	WIN32_FIND_DATAA FindData;
	HANDLE hSearch = FindFirstFileA(currentPath, &FindData);
	if (INVALID_HANDLE_VALUE != hSearch)
	{
		char* curFile = FindData.cFileName;
		do 
		{
			if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				strnset(totalFile, 0, MAX_PATH);
				strcpy(totalFile, additionalPath);
				if(totalFile[strlen(totalFile)-1]!='\\'&&strlen(totalFile)!=0)
				strcat(totalFile, "\\");
				strcat(totalFile, curFile);
				glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot file Relative Path<%s>", totalFile);

				if (fileFilter(totalFile))
				{
					_initialContentsQueue.push(totalFile);
				}
			}
			else if (!strcmp(curFile, "..") || !strcmp(curFile, ".") || !_pDirConsumer->_bWatchSubdirectories)
			{
				glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot ignore directory %s \n", curFile);
			}
			else
			{
				char nextLevel[MAX_PATH];
				strnset(nextLevel, 0, MAX_PATH);
				sprintf(nextLevel, "%s", additionalPath);
				if (strcmp(additionalPath, ""))
				{
					strcat(nextLevel, "\\");
				}
				strcat(nextLevel, curFile);
				
				if (!getDirectorySnapshot(baseDirectory, nextLevel, filter))
				{
					FindClose(hSearch);
					return false;
				}
			}
		} while(FindNextFileA(hSearch, &FindData));
	}
	else if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		glog(Log::L_DEBUG,  "DirMonitor  getDirectorySnapshot FindFirstFileA error <%d>", GetLastError());
		FindClose(hSearch);
		return false;
	}
	FindClose(hSearch);
	return true;
}

bool DirMonitor::openHandles()
{
	_hStop = CreateEvent(NULL, false, false, NULL);
	if (_hStop == NULL)
	{
		return false;
	}

	_hNotify = CreateEvent(NULL, false, false, NULL);
	if (_hNotify == NULL)
	{
		return false;
	}

	_hSetCallback = CreateEvent(NULL, false, false, NULL);
	if (_hSetCallback == NULL)
	{
		return false;
	}

	return true;
}

void DirMonitor::closeHandles()
{
	if (_directoryEvent)
	{
		CloseHandle(_directoryEvent);
		_directoryEvent = NULL;
	}
	
	if (_hNotify)
	{
		CloseHandle(_hNotify);
		_hNotify = NULL;
	}

	if (_hStop)
	{
		CloseHandle(_hStop);
		_hStop = NULL;
	}

	if (_hSetCallback)
	{
		CloseHandle(_hSetCallback);
		_hSetCallback = NULL;
	}
}

bool DirConsumer::openHandles()
{
	_hStop = CreateEvent(NULL, false, false, NULL);
	if (_hStop == NULL)
	{
		return false;
	}

	_hNotify = CreateSemaphoreA(NULL, 0, 5000, NULL);
	if (_hNotify == NULL)
	{
		return false;
	}

	return true;
}

void DirConsumer::closeHandles()
{
	if (_hNotify)
	{
		CloseHandle(_hNotify);
		_hNotify = NULL;
	}

	if (_hStop)
	{
		CloseHandle(_hStop);
		_hStop = NULL;
	}
}

}
}