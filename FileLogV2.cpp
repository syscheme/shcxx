
//////////////////////////////////////////////////////////////////////////
// FileLog.cpp: implementation of the FileLog class.
// Author: copyright (c) Han Guan
//////////////////////////////////////////////////////////////////////

#include "FileLogV2.h"
#include "TimeUtil.h"
#include <iostream>
#include <string>
#include <fstream>
#include <new>
#include <algorithm>
#include <functional>

extern "C" {

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

// -----------------------------
/// class FileLogException
/// -----------------------------
FileLogException::FileLogException(const std::string &what_arg ) throw()
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
	std::ofstream*	m_FileStream;			//log�ļ���
};

//////////////////////////////////////////////////////////////////////////
// LogThread implementation
//////////////////////////////////////////////////////////////////////////

class LogThread : public NativeThread
{
	friend class FileLog;
protected: 
	LogThread() : _bQuit(false) {}
	virtual ~LogThread()
	{
		stop();
	}

	void stop()
	{
		try { 
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
		catch (...) {}
	}

	void addLogInst(FileLog* logInst)
	{
		MutexGuard lk(_lockLogInsts);
		_logInsts.push_back(logInst);
	}

	void rmvLogInst(FileLog* logInst)
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

protected: // derived from native thread
	bool init(void)
	{
#ifdef ZQ_OS_MSWIN
		_event = NULL;
		_event = ::CreateEvent(NULL,TRUE,FALSE,NULL);
		return (NULL != _event);
#else
		sem_init(&_pthsem,0,0);
#endif
		return true;
	}

	int run()
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

	void final(void) {}

private: 
#ifdef ZQ_OS_MSWIN
	HANDLE _event;
#else
	sem_t			_pthsem;
#endif
	bool	_bQuit;
typedef std::vector<FileLog*> LogVector;
typedef LogVector::iterator LogItor;
	LogVector _logInsts;
	Mutex _lockLogInsts;
};
	
/// -----------------------------
/// class FileLog
/// -----------------------------
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

void FileLog::createDir()
{
	std::vector<std::string> dirVct;
	std::string dirStr = m_dirName;
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
	, m_Buff(NULL)
	, m_nMaxBuffSize(ZQLOG_DEFAULT_BUFFSIZE)
	, m_nCurrentBuffSize(0)
	, m_nFlushInterval(ZQLOG_DEFAULT_FLUSHINTERVAL)
	, m_currentMonth(0)
	, m_eventLogLevel(ZQLOG_DEFAULT_EVENTLOGLEVEL)
	, m_instIdent(0)
	, m_cYield(0)
    , m_stampLastReportedFailure(0)
{
	memset(m_FileName, 0, sizeof(m_FileName));

	// ��������ļ���
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
	, m_Buff(NULL)
	, m_nMaxBuffSize(ZQLOG_DEFAULT_BUFFSIZE)
	, m_nCurrentBuffSize(0)
	, m_nFlushInterval(ZQLOG_DEFAULT_FLUSHINTERVAL)
	, m_currentMonth(0)
	, m_eventLogLevel(ZQLOG_DEFAULT_EVENTLOGLEVEL)
	, m_instIdent(0)
	, m_cYield(0)
    , m_stampLastReportedFailure(0)
{
	memset(m_FileName, 0, sizeof(m_FileName));

	// ��������ļ���
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
	// �������
	try {clear();} catch (...) {}
}

void FileLog::clear()
{
	// ��static thread����ע��
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

	// ǿ��д�ļ�
	flush();

	//���ٻ�����
	if (NULL != m_Buff)
		delete []m_Buff;
	m_Buff = NULL;

	// �ر��ļ�
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
	// ����log����
	setVerbosity(verbosity);

	// ���ô�log��eventlog����ʾ������
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

	// ��event log
	m_SysLog.open(appName, eventLogLevel);
	
	//���õ�ǰ���ڣ��Ա㷢�����ڸı�
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
//	time_t tmt;
//	time(&tmt);
//	struct tm* ptm;
//	ptm = localtime(&tmt);
//	m_currentMonth = ptm->tm_mon + 1;//base from 0

	struct timeval tv;
	struct tm* ptm=NULL, tmret;
	gettimeofday(&tv,  (struct timezone*)NULL);
	ptm = localtime_r(&tv.tv_sec, &tmret);
	m_currentMonth = ptm->tm_mon + 1;//base from 0
#endif
	// ����ļ���
	if (NULL == filename || strlen(filename) == 0)
	{
		SYSTEMFILELOG(Log::L_ERROR, "file name is empty!");
		throw FileLogException("file name is empty!");
	}
	snprintf(m_FileName, sizeof(m_FileName) - 1, "%s", filename);
	{
		// ����չ������".log"�������".log"��
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
	// ���Ӧ����
	char* left  = ::strrchr(m_FileName, FNSEPC);
	char* right = ::strrchr(m_FileName, '.');
	if(left != NULL)
	{
		m_appName = ::std::string(left + 1, right - left - 1); 
	}
	else
	{	
		m_appName = ::std::string(m_FileName, right - m_FileName);
	}
		
	// ����log���ļ���СΪ1MB����������������СֵΪ2MB�����ֵΪ2000MB
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
	
	m_nMaxLogfileNum = logFileNum;					//����log�ļ�����Ŀ, Min_FileNum ~ Max_FileNum��
	if(m_nMaxLogfileNum > Max_FileNum)
		m_nMaxLogfileNum = Max_FileNum;
	if (m_nMaxLogfileNum < Min_FileNum)
		m_nMaxLogfileNum = Min_FileNum;

	if (flushInterval < 2)
		flushInterval = 2;
	if (flushInterval > 10)
		flushInterval = 10;
	m_nFlushInterval = flushInterval;

	m_nCurrentFileSize		= 0;							//���õ�ǰ��log�ļ���С
	m_nCurrentBuffSize		= 0;							//���õ�ǰ��buffer size
	m_eventLogLevel = eventLogLevel;						//����д��ϵͳEventLog��log����

	int buffSizeIn8KB = buffersize / (8 * 1024);
	if (buffersize % (8 * 1024) != 0)
		buffSizeIn8KB ++;
	if (buffSizeIn8KB < 1)
		buffSizeIn8KB = 1; // 8 KB
	if (buffSizeIn8KB > 1024)
		buffSizeIn8KB = 1024; // 8 MB
	m_nMaxBuffSize = buffSizeIn8KB * (8 * 1024);
	m_Buff = new char[m_nMaxBuffSize];					//���仺����
	memset(m_Buff, 0, m_nMaxBuffSize);
	
	// �����ļ������������directory
	m_dirName = getPath(m_FileName);	
	createDir();

	if (NULL == _pNest->m_FileStream)
		_pNest->m_FileStream = new std::ofstream();
	if (NULL ==_pNest->m_FileStream)
	{
		SYSTEMFILELOG(Log::L_ERROR, "failed to create fstream obj");
		throw FileLogException("failed to create fstream obj");
	}

	if (IsFileExist(m_nCurrentFileSize))
	{
		//����ļ��Ѿ����ڣ����ļ�����ʱ���ļ���С�Ѿ������m_nCurrentFileSize��		
		_pNest->m_FileStream->open(m_FileName, std::ios::app | std::ios::binary);
		if(!_pNest->m_FileStream->is_open())
		{
			delete _pNest->m_FileStream;
			_pNest->m_FileStream = NULL;

			SYSTEMFILELOG(Log::L_ERROR,"failed to open file [%s]", m_FileName);
			throw FileLogException("Open file fail!");
		}
	}
	else
	{
		//�ļ������ڣ������ļ�
		_pNest->m_FileStream->open(m_FileName,std::ios::out | std::ios::binary);
		if(!_pNest->m_FileStream->is_open())
		{
			delete _pNest->m_FileStream;
			_pNest->m_FileStream = NULL;

			SYSTEMFILELOG(Log::L_ERROR,"failed to create file [%s]",m_FileName);
			throw FileLogException("Create file fail!");
		}
		m_nCurrentFileSize = 0;
	}

	{
		MutexGuard lk(m_lockLastInstIdent);
		// ���m_staticThreadû�д�������������֮
		if (NULL == m_staticThread)
		{
			m_staticThread = new LogThread();
			if (NULL != m_staticThread)
				m_staticThread->start();
			else 
			{
			SYSTEMFILELOG(Log::L_ERROR, "failed to instantize thread");
			throw FileLogException("alloc static thread failed");
			}
		}
		// ��ʱm_staticThread���벻ΪNULL
		if (NULL == m_staticThread)
		{
			SYSTEMFILELOG(Log::L_ERROR, "failed to instanize thread");
			throw FileLogException("Create static thread failed");
		}
		m_instIdent = ++ m_lastInstIdent;
		m_staticThread->addLogInst(this); // ��static threadע��
	}
}

//��buffer ��д�����ݣ�Ҫ��buffer���м���
void FileLog::writeMessage(const char *msg, int level)
{
	// DO: ����msgΪָ���Ľṹ������ʱ��ͻس����з�
	int nMsgSize = strlen(msg);
	if(nMsgSize == 0)//��Ϣ����Ϊ0�򷵻�
		return;

#ifdef ZQ_OS_MSWIN
	SYSTEMTIME time;
	GetLocalTime(&time);

	// ������ڷ����ı�
	if(time.wMonth != m_currentMonth)
	{
		m_currentMonth = time.wMonth;
		{
			MutexGuard lk(m_buffMtx);
			flushData();
			roll();
		}
	}
	char line[ZQLOG_DEFAULT_MAXLINESIZE];

	// ע�ⲻ�ܽ�"\r\n0"����snprintf()�У����Ҫcopy�Ļ���������Ļ���"\r\n0"���ᱻcopy��log�С�
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
			MutexGuard mg(m_buffMtx);
			flushData();
			roll();
		}
	}

	char line[ZQLOG_DEFAULT_MAXLINESIZE] = {0};
	int nCount = snprintf(line,ZQLOG_DEFAULT_MAXLINESIZE-2,"%02d-%02d %02d:%02d:%02d.%03d [ %7s ] %s",ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec,(int)tv.tv_usec/1000,getVerbosityStr(level), msg);
	
#endif
#ifdef ZQ_OS_MSWIN
	if(nCount < 0 )
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
	if(nCount < 0 )
	{
		line[ZQLOG_DEFAULT_MAXLINESIZE-2] = '\n';
		line[ZQLOG_DEFAULT_MAXLINESIZE-1] = '\0';
	}
	else if(nCount > ZQLOG_DEFAULT_MAXLINESIZE-2)
	{
		line[ZQLOG_DEFAULT_MAXLINESIZE-3] = '\n';
		line[ZQLOG_DEFAULT_MAXLINESIZE-2] = '\0';
	}
	else 
	{
		line[nCount] = '\n';
		line[nCount+1] = '\0';
	}

#endif

	// �ж�level�Ƿ�С��m_eventLogLevel
	if (level <= m_eventLogLevel)
	{
		SYSTEMFILELOG(level, msg);
	}

	int nLineSize = 0;

/*	if(nCount < 0)
	{
		nLineSize = ZQLOG_DEFAULT_MAXLINESIZE - 1;
	}
	else if(nCount > ZQLOG_DEFAULT_MAXLINESIZE-3)
	{
#ifdef ZQ_OS_MSWIN
		nLineSize = nCount + 2;
#else
		nLineSize = nCount + 1;
#endif
	}
*/	
#ifdef ZQ_OS_MSWIN
	if(nCount < 0)
	{
		nLineSize = ZQLOG_DEFAULT_MAXLINESIZE - 1;
	}
	else
	{
		nLineSize = nCount + 2;
	}
#else
	if(nCount < 0)
	{
		nLineSize = ZQLOG_DEFAULT_MAXLINESIZE -1;
	}
	else if(nCount > ZQLOG_DEFAULT_MAXLINESIZE-2)
	{
		nLineSize = ZQLOG_DEFAULT_MAXLINESIZE -2;
	}
	else
	{
		nLineSize = nCount + 1;
	}
#endif

	{
		MutexGuard lk(m_buffMtx);
		int totalBuffSize = m_nCurrentBuffSize + nLineSize;
		if(totalBuffSize > m_nMaxBuffSize)
		{
			//������������ɣ��򽫻�������Ϣflush���ļ�����
			flushData();
			memcpy(m_Buff, line, nLineSize);
			m_nCurrentBuffSize = nLineSize;
		}
		else
		{
			memcpy(m_Buff + m_nCurrentBuffSize, line, nLineSize);
			m_nCurrentBuffSize += nLineSize;
		}
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
//�ڽ�buffer�е�����д���ļ�ʱ�������ټ�����ӦΪ�ú���ָ��ͨ��writeMessage�������ã�����writeMessage���Ѿ���buffer���м���
void FileLog::flushData()
{
	if (m_nCurrentBuffSize == 0)
		return;

	int totalFileSize = m_nCurrentFileSize + m_nCurrentBuffSize;
	if (totalFileSize > m_nMaxFileSize)
	{
		//������ļ��ܴ�С���򴴽����ļ������޸��ļ�����
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
		//������ļ���maxsize����д���ļ�
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
				SYSTEMFILELOG(Log::L_ERROR,"failed to create file [%s]", m_FileName);
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
	if (m_nCurrentBuffSize == 0)
		return; // no need to flush buffer data

	//������ļ���maxsize����д���ļ�
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
			SYSTEMFILELOG(Log::L_ERROR, "failed to open logfile[%s]", m_FileName);
			return;
		}
	}

	_pNest->m_FileStream->write(m_Buff, m_nCurrentBuffSize);
	_pNest->m_FileStream->flush();
	m_nCurrentFileSize += m_nCurrentBuffSize;
	m_nCurrentBuffSize =0;

	// rotate the files if file size exceeded the limitation
	if (m_nCurrentFileSize > m_nMaxFileSize)
		roll();
}


bool FileLog::IsFileExist(int& retFileSize)
{
#ifdef ZQ_OS_MSWIN
	WIN32_FIND_DATAA finddata;
	HANDLE hHandle;
	// m_FileName������ͨ���*?
	if(strstr(m_FileName,"*") != NULL || strstr(m_FileName,"?") != NULL)
	{
		throw FileLogException("Invalid file name!");
		return false;
	}

	hHandle = ::FindFirstFileA(m_FileName, &finddata); //TODO-of-Wei: replace FindFirstFileA with GetFileAttributeEx()

	//�Ҳ����ļ�
	if(hHandle == INVALID_HANDLE_VALUE)
		return false;

	//�ҵ����ļ�Ϊ�ļ���
	if((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		FindClose(hHandle);
		return false;
	}

	//�õ��ļ���С
	retFileSize = (finddata.nFileSizeHigh * MAXDWORD + finddata.nFileSizeHigh) + finddata.nFileSizeLow;
	FindClose(hHandle);
#else
	//file is exist ?
	if(access(m_FileName, F_OK) != 0)
		return false;
	struct stat buf;
	if(stat(m_FileName, &buf) != 0)
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

void FileLog::roll()
{
	if (m_cYield >0)
	{
		--m_cYield;
		return;
	}

	m_cYield = 0;
	char searchFor[MAX_PATH];
	snprintf(searchFor, MAX_PATH-2, "%s" FNSEPS "%s.*.log", m_dirName.c_str(), m_appName.c_str());

	bool bQuit = false;
    bool bReportFailureToSystem = m_stampLastReportedFailure == 0 || (now() - m_stampLastReportedFailure) > 5000; // not report twice in 5 seconds

//  int bubblePos = m_nMaxLogfileNum - 1;			comment by weizhaoguang

	std::vector<std::string> files;				// keep the history file indices here
	std::vector<std::string> filesOfLastYear; 

#ifdef ZQ_OS_MSWIN

	SYSTEMTIME filetime;
	SYSTEMTIME currenttime;
	GetLocalTime(&currenttime);

	WIN32_FIND_DATAA	finddata;
	HANDLE hFind;
	bool done = false;
	for (hFind = ::FindFirstFileA(searchFor, &finddata); !bQuit && INVALID_HANDLE_VALUE != hFind && !done;
		done = (0 ==::FindNextFileA(hFind, &finddata)))
	{
		__int64 timestamp =0;
		char extname[20]="\0";
		if (sscanf(finddata.cFileName +m_appName.length(), ".%lld%s", &timestamp, extname) <=1 || 0 != stricmp(extname, ".log"))
			continue; // skip those files that are not in the pattern of rolling filenames

		if(timestamp < 1010000000 || timestamp > 12312359599) // 01010000000 is the minimum timestamp ,12312359599 is the maximum timestamp
			continue;

		FileTimeToSystemTime(&finddata.ftCreationTime, &filetime); 
		
		if (filetime.wYear < currenttime.wYear || (timestamp/1000000000) > currenttime.wMonth)
			filesOfLastYear.push_back(finddata.cFileName);
		else files.push_back(finddata.cFileName);
	}

	::FindClose(hFind);

#else

	//Wei-TODO:
	struct timeval tv;
	struct tm* filetime=NULL,tmret_file;
	struct tm* currenttime=NULL,tmret_current;
	
	gettimeofday(&tv,(struct timezone*)NULL);
	currenttime = localtime_r(&tv.tv_sec,&tmret_current);
	
	glob_t gl;
	struct stat fileinfo;

	if(!glob(searchFor, GLOB_PERIOD|GLOB_TILDE, 0, &gl))
	{
		for(size_t i = 0 ; i < gl.gl_pathc ; ++i)
		{
			int64 timestamp = 0;
			char extname[20] = "\0";
			char filename[MAX_PATH] = "\0";
			if(sscanf(gl.gl_pathv[i]+ m_dirName.length()+1 + m_appName.length(),".%lld%s" ,&timestamp, &extname) <= 1 && 0 != strcasecmp(extname, ".log") )
				continue; // skip those files that are not in the pattern of rolling filenames

			if(timestamp < 1010000000 || timestamp > 12312359599) // 01010000000 is the minimum timestamp ,12312359599 is the maximum timestamp
				continue;
			
			stat(gl.gl_pathv[i], &fileinfo);
			filetime = localtime_r(&fileinfo.st_mtime,&tmret_file);
			snprintf(filename, MAX_PATH-2, "%s.%011lld.log", m_appName.c_str(), timestamp);
			if(filetime->tm_year < currenttime->tm_year || (timestamp/1000000000) > currenttime->tm_mon+1)
				filesOfLastYear.push_back(filename);
			else files.push_back(filename);
			
		}
	}
	globfree(&gl);
#endif

	sort(filesOfLastYear.begin(), filesOfLastYear.end());
	sort(files.begin(), files.end());
	files.insert(files.begin(), filesOfLastYear.begin(), filesOfLastYear.end());

	// clean up the old log files
	if (files.size() >= m_nMaxLogfileNum)
	{
		std::vector<std::string> oldFiles;
		size_t n = files.size() - m_nMaxLogfileNum;
		for (size_t i =0; i <=n; i++)
			oldFiles.push_back(files[i]);
		files.erase(files.begin(), files.begin() +n);

		// remove the out of date files
		for (size_t i = 0; i < oldFiles.size(); ++i)
		{
			char strdel[MAX_PATH];
			snprintf(strdel,sizeof(strdel)-2, "%s" FNSEPS "%s", m_dirName.c_str(), oldFiles[i].c_str());
			if (0 !=::unlink(strdel))
			{
				if (bReportFailureToSystem)
				{
					SYSTEMFILELOG(Log::L_ERROR, "quit log rolling per failed to delete file[%s], errno[%d]: %s", strdel, errno, ::strerror(errno));
					m_stampLastReportedFailure = now();
				}

				bQuit = true;	
			}
		} // end for
	}

	if (bQuit)
	{
		m_cYield = 100;
		return;
	}

	// reset the rolling failure stamp
    m_stampLastReportedFailure = 0;

	// �رյ�ǰ��д���ļ�
	if( NULL != _pNest->m_FileStream && _pNest->m_FileStream->is_open())
	{
		_pNest->m_FileStream->close();
		delete _pNest->m_FileStream;
		_pNest->m_FileStream = NULL;
	}

	// ��log��������RtspProxy.log -> RtspProxy.xxxxxxxxxxxxx.log
	char seqname[20] ="\0";

#ifdef ZQ_OS_MSWIN
	snprintf(seqname, sizeof(seqname)-1, "%02d%02d%02d%02d%02d%d", 
		currenttime.wMonth, currenttime.wDay, currenttime.wHour, currenttime.wMinute, currenttime.wSecond, currenttime.wMilliseconds/100);
#else
	snprintf(seqname, sizeof(seqname)-1, "%02d%02d%02d%02d%02d%d", 
		currenttime->tm_mon+1, currenttime->tm_mday, currenttime->tm_hour, currenttime->tm_min, currenttime->tm_sec, (int)(tv.tv_usec/100000));
#endif

	char newName[MAX_PATH];
	snprintf(newName, sizeof(newName)-2, "%s" FNSEPS "%s.%s.log", m_dirName.empty() ? "." : m_dirName.c_str(), m_appName.c_str(), seqname);

	bool bRenameSuccess = true;
	if (0 != rename(m_FileName, newName))
	{
		SYSTEMFILELOG(Log::L_ERROR, "failed to rename file(%s=>%s): errno[%d]: %s, or someone keep the file open", m_FileName, newName, errno, ::strerror(errno));
		bRenameSuccess = false;
	}

	if(NULL == _pNest->m_FileStream)
		_pNest->m_FileStream = new std::ofstream();

	if(NULL == _pNest->m_FileStream)
		throw FileLogException("failed to new a fstream");

	if (bRenameSuccess)
	{
		// ֮ǰrename����û�д����򴴽�һ���µ��ļ������õ�ǰ�ļ���СΪ0
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

	if (!_pNest->m_FileStream->is_open())
	{	
		delete _pNest->m_FileStream;
		_pNest->m_FileStream = NULL;

		SYSTEMFILELOG(Log::L_ERROR,"failed to create file[%s]", m_FileName);
		throw FileLogException("failed to create file");
	}
}

int FileLog::run_interval()
{
	try{
		flush();
	}
	catch(...) {}
//	while (false == _bQuit)
//	{
//		flush();
//#ifdef ZQ_OS_MSWIN
//		WaitForSingleObject(_event, m_nFlushInterval*1000);
//#else
//		struct timespec ts;
//		struct timeb tb;
//		
//		ftime(&tb);
//		tb.time += m_nFlushInterval;		
//		ts.tv_sec = tb.time;
//		ts.tv_nsec = tb.millitm * 1000000;
//		sem_timedwait(&_pthsem,&ts);
//
//#endif
//	}
	return 0;
}

void FileLog::flush()
{
	MutexGuard lk(m_buffMtx);
	flushData();
}

const char* FileLog::getLogFilePathname() const
{
	return m_FileName;
}

void FileLog::setFileSize(const int& fileSize)
{
	MutexGuard lk(m_buffMtx);

	// ����log���ļ���СΪ1MB����������������СֵΪ2MB�����ֵΪ2000MB
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
	MutexGuard lk(m_buffMtx);
	
	m_nMaxLogfileNum = fileCount;					//����log�ļ�����Ŀ, Min_FileNum ~ Max_FileNum��
	if(m_nMaxLogfileNum > Max_FileNum)
		m_nMaxLogfileNum = Max_FileNum;
	if (m_nMaxLogfileNum < Min_FileNum)
		m_nMaxLogfileNum = Min_FileNum;
}

void FileLog::setBufferSize(const int& buffSize)
{
	MutexGuard lk(m_buffMtx);

	flushData();

	//���ٻ�����
	if (NULL != m_Buff)
		delete []m_Buff;
	m_Buff = NULL;

	int buffSizeIn8KB = buffSize / (8 * 1024);
	if (buffSize % (8 * 1024) != 0)
		buffSizeIn8KB ++;
	if (buffSizeIn8KB < 1)
		buffSizeIn8KB = 1; // 8 KB
	if (buffSizeIn8KB > 1024)
		buffSizeIn8KB = 1024; // 8 MB
	m_nMaxBuffSize = buffSizeIn8KB * (8 * 1024);
	m_Buff = new char[m_nMaxBuffSize];					//���仺����
	memset(m_Buff, 0, m_nMaxBuffSize);
}

void FileLog::setLevel(const int& level)
{
	MutexGuard lk(m_buffMtx);
	setVerbosity(level);
}

}// namespace common
}// namespace ZQ

