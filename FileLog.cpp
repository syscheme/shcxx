
//////////////////////////////////////////////////////////////////////////
// FileLog.cpp: implementation of the FileLog class.
// Author: copyright (c) Han Guan
//////////////////////////////////////////////////////////////////////
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "FileLog.h"
#include "TimeUtil.h"
#include <iostream>
#include <string>
#include <fstream>
#include <new>
#include <algorithm>
#include <functional>

extern "C" {
#include <assert.h>
#ifndef ZQ_OS_MSWIN
#  include <sys/stat.h>
#  include <sys/timeb.h>
#  include <dirent.h>
#  include <glob.h>
#  include <errno.h>
#else
#include <io.h>
#endif
}


//#pragma warning(disable: 4307)
//#pragma warning(disable: 4018)

#define SYSTEMFILELOG (this->m_SysLog)
#define ZQLOG_DELAY_OPRATION_SIZE 1024*512 // 512KB


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace ZQ{
namespace common{

	
/// -----------------------------
/// class FileLogException
/// -----------------------------
FileLogException::FileLogException(const std::string &what_arg) throw()
            :IOException(what_arg)
{
}

FileLogException::~FileLogException() throw()
{
}

/// -----------------------------
/// class FileLogNest
/// -----------------------------
class FileLogNest
{
public:
	FileLogNest():m_FileStream(NULL){};
private:
	friend class FileLog;
	std::ofstream*	m_FileStream;			//log文件流
};

//////////////////////////////////////////////////////////////////////////
// LogThread implementation
//////////////////////////////////////////////////////////////////////////

LogThread::LogThread() : _bQuit(false)
{
}

LogThread::~LogThread()
{
	try {stop();} catch (...) {}
}

bool LogThread::init(void)
{
#ifdef ZQ_OS_MSWIN
	_event = NULL;
	_event = ::CreateEvent(NULL,TRUE,FALSE,NULL);
	return (NULL != _event);
#else
	sem_init(&_pthsem,0,0);
	return true;
#endif
}

void LogThread::final(void)
{
}

int LogThread::run()
{
	while (!_bQuit)
	{
		{
			MutexGuard lk(_lockLogInsts);
			LogItor itor = _logInsts.begin();
			for (; itor != _logInsts.end(); itor ++)
				(*itor)->run_interval();
		}

#ifdef ZQ_OS_MSWIN
#pragma message(__MSGLOC__"TODO: make the hard coded flush interval here to customizeable")
		WaitForSingleObject(_event, ZQLOG_DEFAULT_FLUSHINTERVAL*1000);
#else
		struct timespec ts;
		struct timeb tb;
		ftime(&tb);
		tb.time += ZQLOG_DEFAULT_FLUSHINTERVAL;
		ts.tv_sec = tb.time;
		ts.tv_nsec = tb.millitm * 1000000;
		sem_timedwait(&_pthsem,&ts);
#endif
	}
	return 0;
}

void LogThread::stop()
{
	_bQuit = true;
#ifdef ZQ_OS_MSWIN
	if (NULL != _event)
		::SetEvent(_event);
#else
	try
	{
		sem_post(&_pthsem);
	}
	catch (...){}
#endif
	
	waitHandle(5000);

#ifdef ZQ_OS_MSWIN
	// close event handle
	if (NULL != _event)
		try {CloseHandle(_event);} catch (...) {}
	_event = NULL;
#else
	try
	{
		sem_destroy(&_pthsem);
	}
	catch(...){}
#endif
}

void LogThread::addLogInst(FileLog* logInst)
{
	MutexGuard lk(_lockLogInsts);
	_logInsts.push_back(logInst);
}

void LogThread::rmvLogInst(FileLog* logInst)
{
	MutexGuard lk(_lockLogInsts);
	LogItor itor = _logInsts.begin();
	for (; itor != _logInsts.end(); itor ++)
	{
		if ((*itor)->m_instIdent != logInst->m_instIdent)
			continue;

		_logInsts.erase(itor);
		break;
	}
}

LogThread* FileLog::m_staticThread = NULL;
int FileLog::m_lastInstIdent = 0;

std::string FileLog::leftStr(const std::string& cstStr, int pos)
{
	int size = cstStr.size();
	if (size -1 < pos)
		return cstStr;

	int cur = 0;
	std::string strRet;
	for (; cur < pos; cur++)
		strRet += cstStr[cur];

	return strRet;
}

std::string FileLog::getLeftStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
{
	std::string::size_type find_pos = std::string::npos;
	if (first)
	{
		find_pos = cstStr.find_first_of(splitStr);
		if (find_pos != std::string::npos) 
			return (leftStr(cstStr, find_pos));

		return std::string("");
	}
	else 
	{
		find_pos = cstStr.find_last_of(splitStr);
		if (find_pos != std::string::npos) 
			return (leftStr(cstStr,find_pos));

		return std::string("");
	}
}

std::string FileLog::rightStr(const std::string& cstStr, int pos)
{
	if (pos <= -1)
		return cstStr;

	std::string strRet;
	int len = cstStr.size();
	for (int cur = pos + 1; cur < len; cur++)
		strRet += cstStr[cur];

	return strRet;
}

std::string FileLog::getRightStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
{
	std::string::size_type find_pos = std::string::npos;
	if (first)
	{
		find_pos = cstStr.find_first_of(splitStr);
		if (find_pos != std::string::npos)
			return rightStr(cstStr,find_pos);

		return std::string("");
	}
	else
	{
		find_pos = cstStr.find_last_of(splitStr);
		if (find_pos != std::string::npos)
			return rightStr(cstStr,find_pos);

		return std::string("");
	}
}

std::string FileLog::midStr(const std::string& cstStr, int f_pos, int l_pos)
{
	if (f_pos >= l_pos)
		return "";

	if (f_pos < -1)
		f_pos = -1;

	int size = cstStr.size();
	if (l_pos > size)
		l_pos = size;

	int cur = f_pos + 1;
	std::string strRet;
	for (; cur < l_pos; cur++)
		strRet += cstStr[cur];

	return strRet;
}

void FileLog::splitStr(const std::string& cstStr, const std::string split, std::vector<std::string>& strVect)
{
	std::string tmp;
	strVect.clear();
	std::string::size_type find_pos = std::string::npos, last_find_pos = std::string::npos;
	find_pos = cstStr.find_first_of(split);
	while (find_pos != std::string::npos)
	{
		tmp = midStr(cstStr, last_find_pos, find_pos);
		if (tmp.size() > 0)
			strVect.push_back(tmp);
		last_find_pos = find_pos;
		find_pos = cstStr.find_first_of(split, last_find_pos + 1);
	}

	tmp = midStr(cstStr, last_find_pos, cstStr.size());
	if (tmp.size() > 0)
		strVect.push_back(tmp);
}

std::string FileLog::nLeftStr(const std::string& cstStr, int num)
{
	return leftStr(cstStr, num);
}

std::string FileLog::nRightStr(const std::string& cstStr, int num)
{
	return rightStr(cstStr, cstStr.size() - num - 1);
}

bool FileLog::isInt(const std::string& cstStr)
{
	std::string::size_type find_pos = std::string::npos;
	int size = cstStr.size();
	if (size == 0)
		return false;

	find_pos = cstStr.find_first_not_of("0123456789");
	if (find_pos != std::string::npos)
		return false;

	return true;
}

std::string FileLog::getPath(const std::string& cstStr)
{
	return getLeftStr(cstStr, "\\/",false);
}

void FileLog::createDir(const std::string& dirName)
{
	std::vector<std::string> dirVct;
	std::string dirStr = dirName;
	do 
	{
		dirVct.push_back(dirStr);
		dirStr = getPath(dirStr);
	} while (!dirStr.empty());

#ifdef ZQ_OS_MSWIN
	for (std::vector<std::string>::reverse_iterator rItor = dirVct.rbegin(); rItor != dirVct.rend(); rItor ++)
		::CreateDirectoryA(rItor->c_str(), NULL);
#else	
	for (std::vector<std::string>::reverse_iterator rItor = dirVct.rbegin(); rItor != dirVct.rend(); rItor ++)
	{
		try
		{
			mkdir(rItor->c_str(),0777);
		}
		catch(...){}
		
	}

#endif
}

FileLog::FileLog() 
	: m_nMaxLogfileNum(Max_FileNum)
	, m_nCurrentFileSize(0)
	, m_nMaxFileSize(2000 * 1024 * 1024)
	, _pNest(NULL)
	, m_nFlushInterval(ZQLOG_DEFAULT_FLUSHINTERVAL)
	, m_currentMonth(0)
	, m_eventLogLevel(ZQLOG_DEFAULT_EVENTLOGLEVEL)
	, m_instIdent(0)
	, m_cYield(0)
    , m_stampLastReportedFailure(0)
{
	memset(m_FileName, 0, sizeof(m_FileName));

	m_buffMtx = reinterpret_cast<void*>( new boost::recursive_mutex());
	m_semAvail = reinterpret_cast<void*>( new boost::condition_variable_any());
	m_semFlush = reinterpret_cast<void*>( new boost::condition_variable_any());

	// 创建输出文件流
	try
	{
		_pNest = new FileLogNest();
	}
	catch (const std::bad_alloc& e)
	{
		throw FileLogException(NULL != e.what() ? e.what() : "");
	}
}

FileLog::FileLog(const char* filename, const int verbosity, int logFileNum, int fileSize, int buffersize, int flushInterval, int eventLogLevel, const char* appName)
	: m_nMaxLogfileNum(Max_FileNum)
	, m_nCurrentFileSize(0)
	, m_nMaxFileSize(2000 * 1024 * 1024)
	, _pNest(NULL)
	, m_nFlushInterval(ZQLOG_DEFAULT_FLUSHINTERVAL)
	, m_currentMonth(0)
	, m_eventLogLevel(ZQLOG_DEFAULT_EVENTLOGLEVEL)
	, m_instIdent(0)
	, m_cYield(0)
    , m_stampLastReportedFailure(0)
{
	memset(m_FileName, 0, sizeof(m_FileName));

	m_buffMtx = reinterpret_cast<void*>( new boost::recursive_mutex());
	m_semAvail = reinterpret_cast<void*>( new boost::condition_variable_any());
	m_semFlush = reinterpret_cast<void*>( new boost::condition_variable_any());

	// 创建输出文件流
	try
	{
		_pNest = new FileLogNest();
	}
	catch (const ::std::bad_alloc& e)
	{
		throw FileLogException(NULL != e.what() ? e.what() : "");
	}

	open(filename, verbosity, logFileNum, fileSize, buffersize, flushInterval, eventLogLevel, appName);
}

FileLog::~FileLog()
{
	// 清理对象
	clear();

	boost::recursive_mutex *pMutex = reinterpret_cast<boost::recursive_mutex*>(m_buffMtx);
	delete pMutex;
	boost::condition_variable_any* pCond = reinterpret_cast<boost::condition_variable_any*>(m_semAvail);
	delete pCond;

	pCond = reinterpret_cast<boost::condition_variable_any*>(m_semFlush);
	delete pCond;
}

void FileLog::clear()
{
	// 从static thread上面注销
	{
		MutexGuard lk(m_lockLastInstIdent);
		if (NULL != m_staticThread)
		{
			m_staticThread->rmvLogInst(this);
			if (m_staticThread->_logInsts.size() == 0)
			{
				try {delete m_staticThread;} catch (...) {}
				m_staticThread = NULL;
			}
		}
	}

	// 强制写文件
	flush();
	flushData();

	//销毁缓冲区
	mbRunning = false;
	(reinterpret_cast<boost::condition_variable_any*>(m_semFlush))->notify_one();
	waitHandle(-1);

	{
		boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		for( size_t i = 0; i < mAvailBuffers.size(); i ++ ) {
			LogBuffer* buf = mAvailBuffers[i];
			assert(buf != NULL);
			if (NULL != buf->m_Buff)
				delete [](buf->m_Buff);
			delete buf;
		}
		mAvailBuffers.clear();
	}

	// 关闭文件
	if (NULL != _pNest && NULL != _pNest->m_FileStream && true == _pNest->m_FileStream->is_open())
		_pNest->m_FileStream->close();

	if( NULL != _pNest->m_FileStream )
		delete _pNest->m_FileStream;
	_pNest->m_FileStream = NULL;

	if (NULL != _pNest)
		delete _pNest;
	_pNest = NULL;
}

void FileLog::open(const char* filename, const int verbosity, int logFileNum, int fileSize, int buffersize, int flushInterval, int eventLogLevel, const char* appName)
{
	// 设置log级别
	setVerbosity(verbosity);

	// 设置此log在eventlog中显示的名称
	char mdlName[MAX_PATH];
	memset(mdlName, 0, sizeof(mdlName));

#ifdef ZQ_OS_MSWIN
/*  stop guessing the application name if it is not given, just simply disable the forward to SysLog. -andy 10/21/2009
	if (NULL == appName || strlen(appName) == 0)
	{
		DWORD dRet = ::GetModuleFileName(NULL, mdlName, sizeof(mdlName));
		if (dRet == 0)
			strcpy(mdlName, "FileLog");
	}
	else 
		snprintf(mdlName, sizeof(mdlName) - 1, "%s", appName);
*/

	// 打开event log
	m_SysLog.open(appName, eventLogLevel);
	
	//设置当前日期，以便发现日期改变
	SYSTEMTIME time;
	GetLocalTime(&time);
	m_currentMonth = time.wMonth;
#else

	if (NULL == appName || strlen(appName) == 0)
	{
		int res = readlink("/proc/self/exe", mdlName, sizeof(mdlName));
		if (res < 0 || res > int(sizeof(mdlName)))
			strcpy(mdlName, "FileLog");
	}
	else 
		snprintf(mdlName, sizeof(mdlName) - 1, "%s", appName);

	//write syslog
	//open sys log
//	printf("app naem is %s\n",mdlName);
	m_SysLog.open(mdlName,eventLogLevel);

	//set local month
	time_t tmt;
	time(&tmt);
	struct tm* ptm;
	ptm = localtime(&tmt);
	m_currentMonth = ptm->tm_mon + 1;//base from 0
#endif
	// 获得文件名
	if (NULL == filename || strlen(filename) == 0)
	{
		SYSTEMFILELOG(Log::L_EMERG,"file name is empty!");
		throw FileLogException("file name is empty!");
	}
	snprintf(m_FileName, sizeof(m_FileName) - 1, "%s", filename);
	{
		// 若扩展名不是".log"，则加上".log"。
#ifdef ZQ_OS_MSWIN
		if (stricmp(nRightStr(m_FileName, 4).c_str(), ".log") != 0)
#else
		if (strcasecmp(nRightStr(m_FileName, 4).c_str(), ".log") != 0)
#endif
		{
			std::string tmp_str = m_FileName;
			tmp_str += ".log";
			snprintf(m_FileName, sizeof(m_FileName) - 1, "%s", tmp_str.c_str());
		}
	}
	
	// 设置log的文件大小为1MB的整数倍，并且最小值为2MB，最大值为2000MB
	int nLogSizeIn1MB = fileSize / (1024 * 1024);
	if (fileSize % (1024 * 1024) != 0)
		nLogSizeIn1MB += 1;
	if (nLogSizeIn1MB < 2)
	{
		nLogSizeIn1MB = 2;
	}
	if (nLogSizeIn1MB > 2000)
	{
		nLogSizeIn1MB = 2000;
	}
	m_nMaxFileSize = nLogSizeIn1MB * 1024 * 1024;
	
	m_nMaxLogfileNum = logFileNum;					//设置log文件的数目, Min_FileNum ~ Max_FileNum个
	if(m_nMaxLogfileNum > Max_FileNum)
		m_nMaxLogfileNum = Max_FileNum;
	if (m_nMaxLogfileNum < Min_FileNum)
		m_nMaxLogfileNum = Min_FileNum;

	if (flushInterval < 2)
		flushInterval = 2;
	if (flushInterval > 10)
		flushInterval = 10;
	m_nFlushInterval = flushInterval;

	m_nCurrentFileSize		= 0;							//设置当前的log文件大小
	m_eventLogLevel = eventLogLevel;						//设置写入系统EventLog的log级别

	int buffSizeIn8KB = buffersize / (8 * 1024);
	if (buffersize % (8 * 1024) != 0)
		buffSizeIn8KB ++;
	if (buffSizeIn8KB < 1)
		buffSizeIn8KB = 1; // 8 KB
	if (buffSizeIn8KB > 1024)
		buffSizeIn8KB = 1024; // 8 MB


	for( size_t i = 0 ; i < 3; i ++ ) {
		size_t	m_nMaxBuffSize = buffSizeIn8KB * (8 * 1024);
		char* m_Buff = new char[m_nMaxBuffSize];					//分配缓冲区
		LogBuffer* buf = new LogBuffer(m_Buff, m_nMaxBuffSize);
		makeBufferAvail(buf);
	}
	
	// 根据文件名创建所需的directory
	std::string pathName = getPath(m_FileName);	
	createDir(pathName);

	if (NULL == _pNest->m_FileStream)
		_pNest->m_FileStream = new std::ofstream();
	if (NULL ==_pNest->m_FileStream)
	{
		SYSTEMFILELOG(Log::L_EMERG,"failed to create fstream obj");
		throw FileLogException("failed to create fstream obj");
	}

	if (IsFileExsit(m_FileName, m_nCurrentFileSize))
	{
		//如果文件已经存在，打开文件，此时该文件大小已经存放在m_nCurrentFileSize中		
		_pNest->m_FileStream->open(m_FileName, std::ios::app | std::ios::binary);
		if(!_pNest->m_FileStream->is_open())
		{
			delete _pNest->m_FileStream;
			_pNest->m_FileStream = NULL;

			SYSTEMFILELOG(Log::L_EMERG,"Open file [%s] fail!",m_FileName);
			throw FileLogException("Open file fail!");
		}
	}
	else
	{
		//文件不存在，创建文件
		_pNest->m_FileStream->open(m_FileName,std::ios::out | std::ios::binary);
		if(!_pNest->m_FileStream->is_open())
		{
			delete _pNest->m_FileStream;
			_pNest->m_FileStream = NULL;

			SYSTEMFILELOG(Log::L_EMERG,"Create file [%s] fail!",m_FileName);
			throw FileLogException("Create file fail!");
		}
		m_nCurrentFileSize = 0;
	}

	{
		MutexGuard lk(m_lockLastInstIdent);
		// 如果m_staticThread没有创建，创建并启动之
		if (NULL == m_staticThread)
		{
			m_staticThread = new LogThread();
			if (NULL != m_staticThread)
				m_staticThread->start();
			else 
			{
			SYSTEMFILELOG(Log::L_EMERG,"alloc static thread failed");
			throw FileLogException("alloc static thread failed");
			}
		}
		// 此时m_staticThread必须不为NULL
		if (NULL == m_staticThread)
		{
			SYSTEMFILELOG(Log::L_EMERG,"Create static thread failed");
			throw FileLogException("Create static thread failed");
		}
		m_instIdent = ++ m_lastInstIdent;
		m_staticThread->addLogInst(this); // 向static thread注册
	}
	mbRunning = true;
	NativeThread::start();
}

//向buffer 中写入数据，要对buffer进行加锁
void FileLog::writeMessage(const char *msg, int level)
{
	// DO: 构造msg为指定的结构，带有时间和回车换行符
	int nMsgSize = strlen(msg);
	if(nMsgSize == 0)//信息长度为0则返回
		return;

#ifdef ZQ_OS_MSWIN
	SYSTEMTIME time;
	GetLocalTime(&time);

	// 如果日期发生改变
	if(time.wMonth != m_currentMonth)
	{
		m_currentMonth = time.wMonth;
		{
			flush();
			RenameAndCreateFile();
		}
	}
	char line[ZQLOG_DEFAULT_MAXLINESIZE];

	// 注意不能将"\r\n0"放在snprintf()中，如果要copy的缓冲区过大的话，"\r\n0"不会被copy到log中。
	int nCount = _snprintf(line,ZQLOG_DEFAULT_MAXLINESIZE-3, "%02d-%02d %02d:%02d:%02d.%03d [ %7s ] %s",
		           time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, getVerbosityStr(level), msg);
#else
	struct timeval tv;
	struct tm* ptm=NULL, tmret;
	gettimeofday(&tv,  (struct timezone*)NULL);
	ptm = localtime_r(&tv.tv_sec, &tmret);
	if(ptm == NULL)
	{
		SYSTEMFILELOG(Log::L_ERROR, "writeMessage() failed get localtime_r code[%d] message[%s]", errno, msg);
		return;
	}
	
	//month has change
	if(ptm->tm_mon +1 != m_currentMonth)
	{
		m_currentMonth = ptm->tm_mon + 1;
		{
			flush();
			RenameAndCreateFile();
		}
	}
	
	char line[ZQLOG_DEFAULT_MAXLINESIZE] = {0};
	int nCount = snprintf(line,ZQLOG_DEFAULT_MAXLINESIZE-3,"%02d-%02d %02d:%02d:%02d.%03d [ %7s ] %s",ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec,(int)tv.tv_usec/1000,getVerbosityStr(level), msg);
#endif
#ifdef ZQ_OS_MSWIN
	if(nCount < 0)
	{
		line[ZQLOG_DEFAULT_MAXLINESIZE-3] = '\r';
		line[ZQLOG_DEFAULT_MAXLINESIZE-2] = '\n';
		line[ZQLOG_DEFAULT_MAXLINESIZE-1] = '\0';
	}
	else 
	{
		line[nCount] = '\r';
		line[nCount+1] = '\n';
		line[nCount+2] = '\0';
	}
#else //in linux os delete "\r"
	if(nCount < 0)
	{
		line[ZQLOG_DEFAULT_MAXLINESIZE-2] = '\n';
		line[ZQLOG_DEFAULT_MAXLINESIZE-1] = '\0';
	}
	else 
	{
		line[nCount] = '\n';
		line[nCount+1] = '\0';
	}

#endif

	// 判断level是否小于m_eventLogLevel
	if (level <= m_eventLogLevel)
	{
		SYSTEMFILELOG(level, msg);
	}

	int nLineSize = 0;
	if (nCount < 0)
		nLineSize = ZQLOG_DEFAULT_MAXLINESIZE - 1;
	else
	{
#ifdef ZQ_OS_MSWIN
		nLineSize = nCount + 2;
#else
		nLineSize = nCount + 1;
#endif
	}

	while(true)
	{
		boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		while( true ){
			if(mRunningBuffer != NULL)
				break;
			if(mAvailBuffers.size() != 0 ) {
				mRunningBuffer = *mAvailBuffers.begin();
				break;
			}
			(reinterpret_cast<boost::condition_variable_any*>(m_semAvail))->wait(gd);
		}
		assert(mRunningBuffer != NULL);
		int totalBuffSize = mRunningBuffer->m_nCurrentBuffSize + nLineSize;
		if(totalBuffSize >= mRunningBuffer->m_nMaxBuffSize)
		{
			//若缓冲区不能容纳，则将缓冲区信息flush到文件里面
			makeBufferToBeFlush( mRunningBuffer );
			continue;
		}
		memcpy( mRunningBuffer->m_Buff + mRunningBuffer->m_nCurrentBuffSize, line, nLineSize);
		mRunningBuffer->m_nCurrentBuffSize += nLineSize;
		break;
	}

	return;
}

void FileLog::writeMessage(const wchar_t *msg, int level)
{
	char temp[ZQLOG_DEFAULT_MAXLINESIZE];
	sprintf(temp,"%S",msg);
	writeMessage(temp,level);
}

/*
//在将buffer中的数据写入文件时，不用再加锁，应为该函数指挥通过writeMessage函数调用，而在writeMessage中已经对buffer进行加锁
void FileLog::flushData()
{
	if (m_nCurrentBuffSize == 0)
		return;

	int totalFileSize = m_nCurrentFileSize + m_nCurrentBuffSize;
	if (totalFileSize > m_nMaxFileSize)
	{
		//若大于文件总大小，则创建新文件，并修改文件名。
		RenameAndCreateFile();
		if (NULL == _pNest->m_FileStream)
			return;

		_pNest->m_FileStream->write(m_Buff, m_nCurrentBuffSize);
		_pNest->m_FileStream->flush();
		m_nCurrentFileSize += m_nCurrentBuffSize;
		m_nCurrentBuffSize=0;
	}
	else
	{
		//若不大于文件的maxsize，则写入文件
		if( NULL == _pNest->m_FileStream)
		{
			SYSTEMFILELOG(Log::L_DEBUG, "The file[%s] has not been opened, reopen", m_FileName);
			_pNest->m_FileStream = new std::ofstream();
			if(NULL == _pNest->m_FileStream)
				throw FileLogException("Failed to new a fstream to write");

			_pNest->m_FileStream->open(m_FileName, std::ios::app | std::ios::binary);
			if(!_pNest->m_FileStream->is_open())
			{	
	 			delete _pNest->m_FileStream;
				_pNest->m_FileStream = NULL;
				SYSTEMFILELOG(Log::L_EMERG,"Create file [%s] fail!", m_FileName);
				return;
			}
		}

		_pNest->m_FileStream->write(m_Buff,m_nCurrentBuffSize);
		_pNest->m_FileStream->flush();

		m_nCurrentFileSize = totalFileSize;
		m_nCurrentBuffSize=0;
	}
}

*/

void FileLog::flushData()
{
	std::vector<LogBuffer*> buffers;

	{
		boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		buffers.swap( mToBeFlushBuffers );
	}

	std::vector<LogBuffer*>::const_iterator it = buffers.begin();
	
	char*			m_Buff;						//缓冲区指针
	int				m_nMaxBuffSize;				//缓冲区最大size
	int				m_nCurrentBuffSize;			//当前缓冲区size

	for( ; it != buffers.end() ; it ++ ) {
		//若不大于文件的maxsize，则写入文件
		LogBuffer* buf = *it;
		m_Buff = buf->m_Buff;
		m_nMaxBuffSize = buf->m_nMaxBuffSize;
		m_nCurrentBuffSize = buf->m_nCurrentBuffSize;
		
		if( NULL == _pNest->m_FileStream)
		{
			SYSTEMFILELOG(Log::L_DEBUG, "opening logfile[%s]", m_FileName);
			_pNest->m_FileStream = new std::ofstream();
			if(NULL == _pNest->m_FileStream)
				throw FileLogException("failed to instance an fstream to write");

			// refresh the filesize by reading file system
			m_nCurrentFileSize = 0;
			FILE *fp = NULL;
			if (NULL != (fp=fopen(m_FileName, "rb")))
			{
				fseek(fp, 0, SEEK_END);
				m_nCurrentFileSize = ftell(fp);
				fclose(fp);
			}

			_pNest->m_FileStream->open(m_FileName, std::ios::app | std::ios::binary);
			if (!_pNest->m_FileStream->is_open())
			{	
				delete _pNest->m_FileStream;
				_pNest->m_FileStream = NULL;
				SYSTEMFILELOG(Log::L_EMERG, "failed to open logfile[%s]", m_FileName);
				return;
			}
		}

		_pNest->m_FileStream->write(m_Buff, m_nCurrentBuffSize);
		_pNest->m_FileStream->flush();
		m_nCurrentFileSize += m_nCurrentBuffSize;
		m_nCurrentBuffSize = 0;
		buf->m_nCurrentBuffSize = m_nCurrentBuffSize;
		makeBufferAvail( buf );

		// rotate the files if file size exceeded the limitation
		if (m_nCurrentFileSize > m_nMaxFileSize) {
			RenameAndCreateFile();
		}

	}
}

int FileLog::run() {

	while(mbRunning ) {
		{
			boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
			if(mToBeFlushBuffers.size() == 0 )  {
				(reinterpret_cast<boost::condition_variable_any*>(m_semFlush))->wait(gd);
			}
		}
		flushData();
	}
	return 0;
}

void FileLog::makeBufferAvail( LogBuffer* buf ) {
	assert( buf != NULL );
	{
		buf->m_nCurrentBuffSize = 0;
		boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		mAvailBuffers.push_back(buf);
	}
	(reinterpret_cast<boost::condition_variable_any*>(m_semAvail))->notify_one();
}

void FileLog::makeBufferToBeFlush( LogBuffer* buf ) {
	assert( buf != 0 );
	{
		//caller already hold the locker
		assert(buf == mRunningBuffer);
		assert(mRunningBuffer == mAvailBuffers[0]);
		mAvailBuffers.erase(mAvailBuffers.begin());
		if(mAvailBuffers.size()>0) {
			mRunningBuffer = mAvailBuffers[0];
		} else {
			mRunningBuffer = NULL;
		}

		mToBeFlushBuffers.push_back(buf);
	}
	(reinterpret_cast<boost::condition_variable_any*>(m_semFlush))->notify_one();
}

bool FileLog::getAvailBuffer( ) {
	return false;
}


bool FileLog::IsFileExsit(const char* filename,int& retFileSize)
{
#ifdef ZQ_OS_MSWIN
	WIN32_FIND_DATAA finddata;
	HANDLE hHandle;
	hHandle = ::FindFirstFileA(filename, &finddata);

	//filename里面有通配符*?
	if(strstr(filename,"*") != NULL || strstr(filename,"?") != NULL)
	{
		throw FileLogException("Invalid file name!");
		return false;
	}

	//找不到文件
	if(hHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	//找到的文件为文件夹
	if((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		FindClose(hHandle);
		return false;
	}

	//得到文件大小
	retFileSize = (finddata.nFileSizeHigh * MAXDWORD + finddata.nFileSizeHigh) + finddata.nFileSizeLow;
	FindClose(hHandle);
#else
	//file is exist ?
	if(access(filename, F_OK) != 0)
		return false;
	struct stat buf;
	if(stat(filename, &buf) != 0)
		return false;
	
	retFileSize = buf.st_size;	
#endif

	return true;
}

void FileLog::increaseInsert(std::vector<int>& vctInts, int valInt)
{
	std::vector<int>::iterator itor = vctInts.begin();
	bool bAdded = false;
	for (; itor != vctInts.end(); itor ++)
	{
		if (valInt <= *itor)
		{
			vctInts.insert(itor, valInt);
			bAdded = true;
			break;
		}
	}
	if (false == bAdded)
		vctInts.push_back(valInt);
}

void FileLog::RenameAndCreateFile()
{
	boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
	if (m_cYield >0)
	{
		--m_cYield;
		return;
	}
	else
		m_cYield = 0;

	std::string shortname, extname, dirname(".");
	char* p = ::strrchr(m_FileName, FNSEPC);
	if (NULL != p)
	{
		dirname = ::std::string(m_FileName, p-m_FileName);
		shortname = p+1;
	}
	else
		shortname = m_FileName;

	size_t pos = shortname.find_last_of(".");
	if (std::string::npos != pos && pos >1)
	{
		extname = shortname.substr(pos+1);
		shortname = shortname.substr(0, pos);
	}

	char searchFor[MAX_PATH];
	snprintf(searchFor, MAX_PATH-2, "%s" FNSEPS "%s.*.%s", dirname.c_str(), shortname.c_str(), extname.c_str());

	bool bQuit = false;
    bool bReportFailureToSystem = m_stampLastReportedFailure == 0 || (now() - m_stampLastReportedFailure) > 5000; // not report twice in 5 seconds

    int bubblePos = m_nMaxLogfileNum - 1;

#ifdef ZQ_OS_MSWIN
    std::vector<int> files; // keep the history file indices here

	WIN32_FIND_DATAA finddata;
	HANDLE hFind;
	bool done = false;
	for (hFind = ::FindFirstFileA(searchFor, &finddata); !bQuit && INVALID_HANDLE_VALUE != hFind && !done;
		done = (0 ==::FindNextFileA(hFind, &finddata)))
	{
		//get logfile index
		int fileidx =-1;
		char filename[MAX_PATH];
		strcpy(filename, finddata.cFileName);
		char* lastDot = strrchr(filename, '.');
		
		if (NULL != lastDot)
		{
			*lastDot = '\0'; // cut off the extension name
			lastDot = strrchr(filename, '.');
			if (NULL != lastDot)
			{
				// check if the token before extname is a number
				const char* pFileIdx = lastDot + 1;
				fileidx = atoi(pFileIdx);
				if(fileidx == 0 && 0 != strcmp(pFileIdx, "0"))
				{
					// conversion failure
					fileidx = -1;
				}

				// cut off the token before extname, and double check if the shortname matches
				*lastDot = '\0';
				if (0 != stricmp(filename, shortname.c_str()))
					fileidx = -1;
			}
		}

		if(fileidx < 0)
			continue;

		files.push_back(fileidx);
	}
	
	::FindClose(hFind);
    // we got the file indices, find the bubble position now
    std::vector<int>::iterator itOutOfDate = std::remove_if(files.begin(), files.end(), std::bind2nd(std::greater<int>(), m_nMaxLogfileNum - 1));

    std::vector<int> oldFiles;
    oldFiles.assign(itOutOfDate, files.end());
    files.erase(itOutOfDate, files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());

    std::sort(files.begin(), files.end());
    {
        bubblePos = -1;
        for(size_t i = 0; i < files.size(); ++i)
        {
            if(i != files[i])
            {
                bubblePos = i;
                break;
            }
        }

        if(bubblePos < 0)
        { // no bubble in the valid range, we should make one
            if(files.size() == m_nMaxLogfileNum)
            {
                oldFiles.push_back(files.back());
                files.pop_back();
            }
            bubblePos = files.size();
        }

        // remove the out of date files
        for(size_t i = 0; i < oldFiles.size(); ++i)
        {
            char idxBuf[12] = {0};
            sprintf(idxBuf, "%d", oldFiles[i]);

            std::string strdel = dirname + FNSEPS + shortname + "." + idxBuf + "." + extname;

            if (0 !=::unlink(strdel.c_str()))
            {
                if (oldFiles[i] == m_nMaxLogfileNum-1)
                {
                    if(bReportFailureToSystem)
                    {
                        SYSTEMFILELOG(Log::L_EMERG, "quit log rolling per delete file[%s] failed, errno[%d]: %s", strdel.c_str(), errno, ::strerror(errno));
                        m_stampLastReportedFailure = now();
                    }
                    bQuit = true;
                }
            }
        } // end for
    }
#else
	char scanFor[MAX_PATH];
	snprintf(scanFor, MAX_PATH-2, "%s" FNSEPS "%s.%%d.%s", dirname.c_str() ,shortname.c_str(), extname.c_str());
	glob_t gl;

	if (!glob(searchFor, GLOB_PERIOD|GLOB_TILDE, 0, &gl)) 
	{
		for(size_t i = 0; i < gl.gl_pathc; ++i) 
		{
			int fileidx =-1;
			if (::sscanf(gl.gl_pathv[i], scanFor, &fileidx) ==1 && fileidx >= m_nMaxLogfileNum-1)
			{	
				if (0 !=::unlink(gl.gl_pathv[i]) && fileidx == m_nMaxLogfileNum-1)
				{
                    if(bReportFailureToSystem)
                    {
                        SYSTEMFILELOG(Log::L_EMERG, "quit log rolling per delete file[%s] failed, errno[%d]: %s", gl.gl_pathv[i], errno, ::strerror(errno));
                        m_stampLastReportedFailure = now();
                    }
					bQuit = true;
				}
			}	
		}
	}

	globfree(&gl);
#endif

    // rename all the files before bubble position
	for (int i = bubblePos; !bQuit && i >0; i--)
	{
		char fromName[MAX_PATH], toName[MAX_PATH];
		snprintf(toName, sizeof(toName) -2, "%s" FNSEPS "%s.%d.%s", dirname.c_str(), shortname.c_str(), i, extname.c_str());
		snprintf(fromName, sizeof(fromName) -2, "%s" FNSEPS "%s.%d.%s", dirname.c_str(), shortname.c_str(), i-1, extname.c_str());
		if (0 == ::access(fromName, 0) && 0 !=::rename(fromName, toName))
		{
            if(bReportFailureToSystem)
            {
                SYSTEMFILELOG(Log::L_EMERG, "quit log rolling per renaming file(%s=>%s) failed: %s", fromName, toName, ::strerror(errno));
                m_stampLastReportedFailure = now();
            }
			bQuit = true;
		}
	}

	if (bQuit)
	{
		m_cYield = 100;
		return;
	}
    else
    { // no error during the rolling
        // reset the rolling failure stamp
        m_stampLastReportedFailure = 0;
    }

/*
	// noExtName: 没有扩展名的带有路径的文件名
	// extName: 扩展名
	// dirName: 路径名结尾没有"\\"
	::std::string noExtName, extName, dirName;
	noExtName = getLeftStr(m_FileName, ".", false);
	extName = getRightStr(m_FileName, ".", false);
	dirName = getPath(m_FileName);
	//                 |       noExtName      | extName |
	// 假设当前文件名为"directoryname\filename.extension"
	//                 |   dirName   |
	// DO: directoryname\filename.*.extension匹配的文件名
	std::vector<std::string> tempArray;

#ifdef ZQ_OS_MSWIN
	WIN32_FIND_DATAA findData;
	HANDLE hHandle;
	char findBuff[MAX_PATH];
	snprintf(findBuff, sizeof(findBuff) - 1, "%s.*.%s", noExtName.c_str(), extName.c_str());
	hHandle = ::FindFirstFileA(findBuff, &findData);
	if (INVALID_HANDLE_VALUE != hHandle)
	{
		do 
		{
			char fullName[MAX_PATH];
			fullName[sizeof(fullName) - 1] = '\0';
			snprintf(fullName, sizeof(fullName) - 1, "%s\\%s", dirName.c_str(), findData.cFileName);
			tempArray.push_back(fullName);
		}
		while (::FindNextFileA(hHandle, &findData));
	}
	::FindClose(hHandle);
#else
	for(int ncount = 0; ncount<= m_nMaxLogfileNum; ncount ++)
	{
		char chName[MAX_PATH];
		snprintf(chName, sizeof(chName) - 1, "%s.%d.%s", noExtName.c_str(), ncount,extName.c_str());
		if(access(chName,F_OK) == 0)
			tempArray.push_back(chName);
	}
#endif

	// 将文件按新老排序，最近的放在数组的前面
	std::vector<int> vctInts;
	vctInts.clear();
	int i = 0,count = 0;
	for (i = 0, count = tempArray.size(); i < count; i ++)
	{
		std::string idxStr;
		idxStr = getRightStr(getLeftStr(tempArray[i], ".", false), ".", false);
		if (true == isInt(idxStr))
			increaseInsert(vctInts, atoi(idxStr.c_str()));
	}
	std::vector<std::string> fileArray;
	fileArray.clear();
	for (i = 0, count = vctInts.size(); i < count; i ++)
	{
		char fullName[MAX_PATH];
		fullName[sizeof(fullName) - 1] = '\0';
		snprintf(fullName, sizeof(fullName) - 1, "%s.%d.%s", noExtName.c_str(), vctInts[i], extName.c_str());
		fileArray.push_back(fullName);
	}

	// 删除过期的备份文件
#ifdef ZQ_OS_MSWIN
	while (int(fileArray.size()) >= m_nMaxLogfileNum)
	{
		if (0 == ::DeleteFileA(fileArray.back().c_str()))
			SYSTEMFILELOG(Log::L_EMERG, "delete file [%s] failed.", fileArray.back().c_str());
		fileArray.pop_back();
	}
#else
	while (int(fileArray.size()) >= m_nMaxLogfileNum)
	{
		if (0 != remove(fileArray.back().c_str()))
			SYSTEMFILELOG(Log::L_EMERG, "delete file [%s] failed.", fileArray.back().c_str());//syslog 
		fileArray.pop_back();
	}

#endif
	// 开始rename
	for (i = fileArray.size() - 1; i >= 0; i --)
	{
		char newName[MAX_PATH];
		newName[sizeof(newName) - 1] = '\0';
		snprintf(newName, sizeof(newName) - 1, "%s.%d.%s", noExtName.c_str(), vctInts[i] + 1, extName.c_str());
		rename(fileArray[i].c_str(), newName);
	}
*/

	// 关闭当前读写的文件并修改其名称
	if( NULL != _pNest->m_FileStream && _pNest->m_FileStream->is_open())
	{
		_pNest->m_FileStream->close();
		delete _pNest->m_FileStream;
		_pNest->m_FileStream = NULL;
	}

	bool bRenameSuccess = true;
	char newName[MAX_PATH];
	newName[sizeof(newName) - 1] = '\0';
	snprintf(newName, sizeof(newName) - 1, "%s" FNSEPS "%s.0.%s", dirname.c_str(), shortname.c_str(), extname.c_str());
	if (0 != rename(m_FileName, newName))
	{
		SYSTEMFILELOG(Log::L_EMERG, "failed to rename file(%s=>%s) failed: errno[%d]: %s", m_FileName, newName, errno, ::strerror(errno));
		bRenameSuccess = false;
	}
	
	if(NULL == _pNest->m_FileStream)
		_pNest->m_FileStream = new std::ofstream();

	if(NULL == _pNest->m_FileStream)
		throw FileLogException("Failed to new a fstream");

	// 创建一个新的文件
	if (true == bRenameSuccess)
	{
		// 之前rename过程没有错误，则创建一个新的文件，并置当前文件大小为0
		_pNest->m_FileStream->open(m_FileName, std::ios::out | std::ios::binary);
		m_nCurrentFileSize = 0;
	}
	else 
	{
		_pNest->m_FileStream->open(m_FileName, std::ios::app | std::ios::binary);
		m_nCurrentFileSize -= 256 * 1024;
		if (m_nCurrentFileSize <= 0)
			m_nCurrentFileSize = 0;
	}

	if(!_pNest->m_FileStream->is_open())
	{	
		delete _pNest->m_FileStream;
		_pNest->m_FileStream = NULL;

		SYSTEMFILELOG(Log::L_EMERG,"Create file [%s] fail!", m_FileName);
		throw FileLogException("Create file fail!");
	}
}

int FileLog::run_interval()
{
	flushData();
	return 0;
}

void FileLog::flush()
{
	{
		boost::recursive_mutex::scoped_lock gd(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		if(mRunningBuffer != NULL && mAvailBuffers.size() > 0 && mRunningBuffer->m_nCurrentBuffSize > 0 ) {
			makeBufferToBeFlush(mRunningBuffer);
		}
	}
}

const char* FileLog::getLogFilePathname() const
{
	return m_FileName;
}

void FileLog::setFileSize(const int& fileSize)
{
	boost::recursive_mutex::scoped_lock lk(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));

	// 设置log的文件大小为1MB的整数倍，并且最小值为2MB，最大值为2000MB
	int nLogSizeIn1MB = fileSize / (1024 * 1024);
	if (fileSize % (1024 * 1024) != 0)
		nLogSizeIn1MB += 1;
	if (nLogSizeIn1MB < 2)
	{
		nLogSizeIn1MB = 2;
	}
	if (nLogSizeIn1MB > 2000)
	{
		nLogSizeIn1MB = 2000;
	}
	m_nMaxFileSize = nLogSizeIn1MB * 1024 * 1024;
}

void FileLog::setFileCount(const int& fileCount)
{
	boost::recursive_mutex::scoped_lock lk(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
	
	m_nMaxLogfileNum = fileCount;					//设置log文件的数目, Min_FileNum ~ Max_FileNum个
	if(m_nMaxLogfileNum > Max_FileNum)
		m_nMaxLogfileNum = Max_FileNum;
	if (m_nMaxLogfileNum < Min_FileNum)
		m_nMaxLogfileNum = Min_FileNum;
}

void FileLog::setBufferSize(const int& buffSize)
{


	flush();
	flushData();

	{
		boost::recursive_mutex::scoped_lock lk(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
		//do not support changing buffer size on the fly
		std::vector<LogBuffer*>::iterator it  = mAvailBuffers.begin();

		for( ; it != mAvailBuffers.end() ; it ++ ) {
			LogBuffer* buf = *it;
			if (NULL != buf->m_Buff)
				delete [](buf->m_Buff);
			delete buf;
		}
		mAvailBuffers.clear();
	}
	
	int buffSizeIn8KB = buffSize / (8 * 1024);
	if (buffSize % (8 * 1024) != 0)
		buffSizeIn8KB ++;
	if (buffSizeIn8KB < 1)
		buffSizeIn8KB = 1; // 8 KB
	if (buffSizeIn8KB > 1024)
		buffSizeIn8KB = 1024; // 8 MB
	int m_nMaxBuffSize = buffSizeIn8KB * (8 * 1024);

	for( size_t i = 0 ; i < 3; i ++ ) {
		char* m_Buff = new char[m_nMaxBuffSize];					//分配缓冲区
		LogBuffer* buf = new LogBuffer(m_Buff, m_nMaxBuffSize);
		makeBufferAvail(buf);
	}
	mRunningBuffer = mAvailBuffers[0];
}

void FileLog::setLevel(const int& level)
{
	boost::recursive_mutex::scoped_lock lk(*(reinterpret_cast<boost::recursive_mutex*>(m_buffMtx)));
	setVerbosity(level);
}

}// namespace common
}// namespace ZQ

