// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
//
// Ident : $Id: DirConsumer.h,v 1.0 2006/11/30 10:530:56 Chen Shuai Exp $
// Branch: $Name:  $
// Author: Chen Shuai
// Desc  : Define DirConsumer
//

// ===========================================================================

#ifndef __DIRCONSUMER_H__
#define __DIRCONSUMER_H__


#include "afx.h"
#include "NativeThread.h"
#include "Locks.h"
#include <list>
#include <queue>
#include "Log.h"

using namespace std;

#define OUT

namespace ZQ{
namespace common{

class ZQ_COMMON_API DirConsumer;
class ZQ_COMMON_API DirMonitor;

#define DIR_ADD     0x00000001
#define DIR_DEL     0x00000002
#define DIR_MODIFY  0x00000004
#define DIR_REN_OLD 0x00000008
#define DIR_REN_NEW 0x00000010

/************************************************************************/
/*            DirConsumer 类定义                                        */
/************************************************************************/
class DirConsumer : public NativeThread
{	
	/************************************************************************/
	/*            DirItem 类定义                                            */
	/************************************************************************/
	class DirItem
	{
	public:
		enum FileAction {UNKNOWN,
						 ADD,
						 REMOVE, 
						 MODIFY,
						 RENAMEOLD,
						 RENAMENEW};
		
		DirItem(const char* fileName, FileAction action);
		~DirItem() {};

	public:
		char _fileName[MAX_PATH];
		FileAction _fileAction;

	private:
		DirItem();

	};

	/************************************************************************/
	/*            typedef                                                   */
	/************************************************************************/
	typedef DirItem* LPDirItem;
	typedef list<LPDirItem> DirItemList;
	typedef list<LPDirItem>::iterator DirItemListIterator;

	/************************************************************************/
	/*           DirConsumer 成员函数与变量定义                             */
	/************************************************************************/
	friend class DirMonitor;
public:
	DirConsumer(const char* monitorDirectory,
				DWORD dirOperation = DIR_ADD | DIR_DEL | DIR_MODIFY | DIR_REN_OLD | DIR_REN_NEW,
				bool bWatchSubdirectories = true, 
			    bool bSnapshot = true,
				bool bNotifyAsFileCome = false,
				DWORD  DirMonitorInterval= INFINITE);

	DirConsumer(const WCHAR* monitorDirectory,
				DWORD dirOperation = DIR_ADD | DIR_DEL | DIR_MODIFY | DIR_REN_OLD | DIR_REN_NEW,
				bool bWatchSubdirectories = true, 
			    bool bSnapshot = true,
				bool bNotifyAsFileCome = false,
				 DWORD  DirMonitorInterval = INFINITE);

	virtual ~DirConsumer();

	void stop() { SetEvent(_hStop); waitHandle(1000);};
	
	/************************************************************************/
	/*                针对不同的事件所对应的处理函数                        */
	/************************************************************************/
	virtual bool processAdd(const char* fileName) {return true; };
	virtual bool processDelete(const char* fileName) {return true; };
	virtual bool processModify(const char* fileName) {return true; };
	virtual bool processRenameOld(const char* fileName) {return true; };
	virtual bool processRenameNew(const char* fileName) {return true; };

protected:
	virtual bool init(void);
	virtual int run(void);
	virtual bool configure();

	/************************************************************************/
	/*           事件的响应                                                 */
	/************************************************************************/
	virtual bool notify() { long count; return ReleaseSemaphore(_hNotify, 1, &count); };
	virtual bool processNotify();
	virtual bool processTimeout() { processNotify(); return true;};

	void fileAdd(const char* fileName)		 {addTail(fileName, DirItem::ADD); notify(); };
private:
	DirConsumer();
	bool openHandles();
	void closeHandles();
	bool isFileAccess(const char* fileName);
	void setDirOperation(DWORD dirOperation);
	bool isEmpty() { return ( 0 == _dirItemList.size() ); };
	void initMembers(DWORD dirOperation, bool bWatchSubdirectories, 
					 bool bSnapshot, bool bNotifyAsFileCome);
	
	/************************************************************************/
	/*           对list的操作                                               */
	/************************************************************************/
	void fileRemove(const char* fileName)	 {addTail(fileName, DirItem::REMOVE); notify(); };
	void fileModify(const char* fileName)	 {addTail(fileName, DirItem::MODIFY); notify(); };
	void fileRenameOld(const char* fileName) {addTail(fileName, DirItem::RENAMEOLD); notify(); };
	void fileRenameNew(const char* fileName) {addTail(fileName, DirItem::RENAMENEW); notify(); };
	
	bool addTailExclusive(const char* fileName, DirItem::FileAction action = DirItem::ADD);
	bool addHead(const char* fileName, DirItem::FileAction action = DirItem::ADD);
	bool addTail(const char* fileName, DirItem::FileAction action = DirItem::ADD);
	bool removeHead(OUT char* fileName, OUT DirItem::FileAction* action = NULL);
	bool removeTail(OUT char* fileName, OUT DirItem::FileAction* action = NULL);
	
	bool getHead(OUT char* fileName, OUT DirItem::FileAction* action = NULL);
	bool getTail(OUT char* fileName, OUT DirItem::FileAction* action = NULL);

	bool removeFile(const char* fileName);
	void removeAll();

protected:
	HANDLE _hStop;
	HANDLE _hNotify;

	DWORD _dirOperation;
	DWORD _notifyFilter;
	bool _bWatchSubdirectories;
	bool _bSnapshot;
	char _monitorDirectory[MAX_PATH];
	bool _bNotifyAsFileCome;
	DWORD _interval;
    DWORD _DirMonitorInterval;

private:

	DirItemList _dirItemList;
	DirMonitor* _pDirMonitor;
	Mutex _mutex;
};


/************************************************************************/
/*              DirMonitor  类定义                                      */
/************************************************************************/
class DirMonitor : public NativeThread
{
	typedef queue<string> StringQueue;
	
	friend class DirConsumer;
protected:
	DirMonitor(DirConsumer* dirConsumer,int interval);
	virtual ~DirMonitor();
	
	void stop() { SetEvent(_hStop); waitHandle(1000);};

	virtual bool configure();
	
	/************************************************************************/
	/*                NativeThread virtual函数                              */
	/************************************************************************/
	virtual bool start();
	virtual bool init(void);
	virtual int  run(void);

	/************************************************************************/
	/*                事件的响应                                            */
	/************************************************************************/
	virtual bool notify() { return (SetEvent(_hNotify)); };
	virtual bool processTimeout() { processNotify(); return true;};
	virtual bool processNotify();

	/************************************************************************/
	/*                对监视目录的操作                                      */
	/************************************************************************/
	virtual bool getDirectorySnapshot(const char* baseDirectory,
									  const char* additionalPath,
									  const char* filter);
	virtual bool processDirectorySnapshot();
	virtual int  fileFilter(const char* str) { return 1; };
	
private:
	DirMonitor();

	bool setMonitorChanges(DWORD&);
	bool processChangedFile(FILE_NOTIFY_INFORMATION* pFni);
	static VOID WINAPI HandleDirChanges(DWORD, DWORD, LPOVERLAPPED);

	bool openHandles();
	void closeHandles();

protected:
	HANDLE _hStop;
	HANDLE _hNotify;
	HANDLE _directoryEvent;
	HANDLE _hSetCallback;

	DWORD _interval;
private:
	char _buffer[MAX_PATH];
	OVERLAPPED _overlapped;

	StringQueue _initialContentsQueue;
	DirConsumer* _pDirConsumer;
	bool _bSetCallbackSucess;
	Mutex _mutex;
};


}
}

#endif