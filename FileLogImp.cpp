
#include "FileLogImp.h"
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
			friend class FileLogImp;
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

		void LogThread::addLogInst(FileLogImp* logInst)
		{
			MutexGuard lk(_lockLogInsts);
			_logInsts.push_back(logInst);
		}

		void LogThread::rmvLogInst(FileLogImp* logInst)
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

		LogThread* FileLogImp::m_staticThread = NULL;
		int FileLogImp::m_lastInstIdent = 0;

		std::string FileLogImp::leftStr(const std::string& cstStr, int pos)
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

		std::string FileLogImp::getLeftStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
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

		std::string FileLogImp::rightStr(const std::string& cstStr, int pos)
		{
			if (pos <= -1)
				return cstStr;

			std::string strRet;
			int len = cstStr.size();
			for (int cur = pos + 1; cur < len; cur++)
				strRet += cstStr[cur];

			return strRet;
		}

		std::string FileLogImp::getRightStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
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

		std::string FileLogImp::midStr(const std::string& cstStr, int f_pos, int l_pos)
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

		void FileLogImp::splitStr(const std::string& cstStr, const std::string split, std::vector<std::string>& strVect)
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

		std::string FileLogImp::nLeftStr(const std::string& cstStr, int num)
		{
			return leftStr(cstStr, num);
		}

		std::string FileLogImp::nRightStr(const std::string& cstStr, int num)
		{
			return rightStr(cstStr, cstStr.size() - num - 1);
		}

		bool FileLogImp::isInt(const std::string& cstStr)
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

		std::string FileLogImp::getPath(const std::string& cstStr)
		{
			return getLeftStr(cstStr, "\\/",false);
		}

		void FileLogImp::createDir(const std::string& dirName)
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

		FileLogImp::FileLogImp() 
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

		FileLogImp::FileLogImp(const char* filename, const int verbosity, int logFileNum, int fileSize, int buffersize, int flushInterval, int eventLogLevel, const char* appName)
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
			memset(m_FileNameAppend, 0, sizeof(m_FileNameAppend));

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

		FileLogImp::~FileLogImp()
		{
			// 清理对象
			try {clear();} catch (...) {}
		}

		void FileLogImp::clear()
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

			//销毁缓冲区
			if (NULL != m_Buff)
				delete []m_Buff;
			m_Buff = NULL;

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

		void FileLogImp::open(const char* filename, const int verbosity, int logFileNum, int fileSize, int buffersize, int flushInterval, int eventLogLevel, const char* appName)
		{
			// 设置log级别
			setVerbosity(verbosity);

			// 设置此log在eventlog中显示的名称
			char mdlName[MAX_PATH];
			memset(mdlName, 0, sizeof(mdlName));

#ifdef ZQ_OS_MSWIN
			// 打开event log
			m_SysLog.open(appName, eventLogLevel);

			//设置当前日期，以便发现日期改变
			SYSTEMTIME time;
			GetLocalTime(&time);
			m_currentMonth = time.wMonth;
			int nCount = _snprintf(m_FileNameAppend, MAX_PATH-3, "%04d-%02d-%02d_%02d-%02d_%02d-%03d",
				time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#else
			if (NULL == appName || strlen(appName) == 0)
			{
				int res = readlink("/proc/self/exe", mdlName, sizeof(mdlName));
				if (res < 0 || res > int(sizeof(mdlName)))
					strcpy(mdlName, "FileLogImp");
			}
			else 
				snprintf(mdlName, sizeof(mdlName) - 1, "%s", appName);

			//write syslog
			//open sys log
			m_SysLog.open(mdlName,eventLogLevel);

			time_t tmt;
			struct tm* ptm;
			struct timeval tv;
						
			time(&tmt);
			ptm = localtime(&tmt);
			gettimeofday(&tv,  (struct timezone*)NULL);
			m_currentMonth = ptm->tm_mon + 1;//set local month, base from 0
			int nCount = snprintf(m_FileNameAppend, MAX_PATH-3, "%04d-%02d-%02d_%02d-%02d_%02d-%03d",
				ptm->tm_year + 1900, ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec,(int)tv.tv_usec/1000);
#endif
			// 获得文件名
			if (NULL == filename || strlen(filename) == 0)
			{
				SYSTEMFILELOG(Log::L_EMERG,"file name is empty!");
				throw FileLogException("file name is empty!");
			}

			std::string shortname;
			std::string extname("log");
			std::string dirname(".");
			snprintf(m_FileName, sizeof(m_FileName) - 1, "%s", filename);
			char* p = ::strrchr(m_FileName, FNSEPC);
			if (NULL != p)
			{
				dirname = ::std::string(m_FileName, p - m_FileName);
				shortname = p + 1;
			}
			else
				shortname = m_FileName;

			size_t pos = shortname.find_last_of(".");
			if (std::string::npos != pos && pos >1)
			{
				extname = shortname.substr(pos+1);
				size_t posShort = shortname.find_last_of(".", pos - 1);
				if (std::string::npos != posShort && posShort >1)
					shortname = shortname.substr(0, posShort);
				else
					shortname = shortname.substr(0, pos);
			}
			snprintf(m_FileDir, sizeof(m_FileDir) - 1, "%s", dirname.c_str());
			snprintf(m_FileShortName, sizeof(m_FileShortName) - 1, "%s", shortname.c_str());
			snprintf(m_FileName, sizeof(m_FileName) - 1, "%s/%s.%s.%s", 
				m_FileDir, m_FileShortName, m_FileNameAppend, extname.c_str() );

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
			m_nCurrentBuffSize		= 0;							//设置当前的buffer size
			m_eventLogLevel = eventLogLevel;						//设置写入系统EventLog的log级别

			int buffSizeIn8KB = buffersize / (8 * 1024);
			if (buffersize % (8 * 1024) != 0)
				buffSizeIn8KB ++;
			if (buffSizeIn8KB < 1)
				buffSizeIn8KB = 1; // 8 KB
			if (buffSizeIn8KB > 1024)
				buffSizeIn8KB = 1024; // 8 MB
			m_nMaxBuffSize = buffSizeIn8KB * (8 * 1024);
			m_Buff = new char[m_nMaxBuffSize];					//分配缓冲区
			memset(m_Buff, 0, m_nMaxBuffSize);

			// 根据文件名创建所需的directory
			createDir(dirname);

			if (NULL == _pNest->m_FileStream)
				_pNest->m_FileStream = new std::ofstream();
			if (NULL ==_pNest->m_FileStream)
			{
				SYSTEMFILELOG(Log::L_EMERG,"failed to create fstream obj");
				throw FileLogException("failed to create fstream obj");
			}

			std::vector<std::string> fileVec;
			CreateFileVec(fileVec, m_FileDir, m_FileShortName, "log");
			if (0 < fileVec.size())
			{
				std::string fileName = fileVec.back();
				memset(m_FileName, 0, sizeof(m_FileName));
				memset(m_FileNameAppend, 0, sizeof(m_FileNameAppend));
				snprintf(m_FileName, sizeof(m_FileName) - 1, "%s", fileName.c_str() );
				char* searchBase = strrchr(m_FileName, FNSEPC);
				searchBase = (NULL == searchBase) ? m_FileName : searchBase;
				sscanf(searchBase,"%*[^.].%[0-9_-].%*[^.]", m_FileNameAppend); 
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
		}

		//向buffer 中写入数据，要对buffer进行加锁
		void FileLogImp::writeMessage(const char *msg, int level)
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
					MutexGuard lk(m_buffMtx);
					flushData();
					RemoveAndCreateFile();
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
					MutexGuard mg(m_buffMtx);
					flushData();
					RemoveAndCreateFile();
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

			{
				MutexGuard lk(m_buffMtx);
				int totalBuffSize = m_nCurrentBuffSize + nLineSize;
				if(totalBuffSize > m_nMaxBuffSize)
				{
					//若缓冲区不能容纳，则将缓冲区信息flush到文件里面
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

		void FileLogImp::writeMessage(const wchar_t *msg, int level)
		{
			char temp[ZQLOG_DEFAULT_MAXLINESIZE];
			sprintf(temp,"%S",msg);
			writeMessage(temp,level);
		}

		void FileLogImp::flushData()
		{
			if (m_nCurrentBuffSize == 0)
				return; // no need to flush buffer data

			//若不大于文件的maxsize，则写入文件
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
			m_nCurrentBuffSize =0;

			// rotate the files if file size exceeded the limitation
			if (m_nCurrentFileSize > m_nMaxFileSize)
				RemoveAndCreateFile();
		}


		bool FileLogImp::IsFileExsit(const char* filename,int& retFileSize)
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

		void FileLogImp::increaseInsert(std::vector<int>& vctInts, int valInt)
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

		int  FileLogImp::CreateFileVec(std::vector<std::string>& outVector, const char* dirname, const char* filename, const char* extname)
		{
			char searchFor[MAX_PATH] = {0};
			char fileNameAppend[MAX_PATH];
			char match[MAX_PATH];
#ifdef ZQ_OS_MSWIN
			WIN32_FIND_DATAA finddata;
			HANDLE hFind;
			bool done = false;
			snprintf(searchFor, MAX_PATH - 2, "%s" FNSEPS "%s.*.%s", dirname, filename, extname);
			for (hFind = ::FindFirstFileA(searchFor, &finddata);
				INVALID_HANDLE_VALUE != hFind && !done;
				done = (0 ==::FindNextFileA(hFind, &finddata)))
			{
				memset(fileNameAppend, 0, sizeof(fileNameAppend));
				int matchYes = sscanf(finddata.cFileName,"%*[^.].%[0-9_-].%*[^.]", fileNameAppend);
				if (-1 < matchYes
					&& -1 < sscanf(fileNameAppend, "%[^-]-%[^_]_%[^-]-%[^_]_%[^-]", match, match, match, match, match))
				{
					outVector.push_back(std::string(finddata.cFileName));
				}
			}

			::FindClose(hFind);
#else
			glob_t gl;
			snprintf(searchFor, MAX_PATH-2, "%s" FNSEPS "%s.*.%s", dirname, filename, extname);
			if (!glob(searchFor, GLOB_PERIOD|GLOB_TILDE, 0, &gl)) 
			{
				for(size_t i = 0; i < gl.gl_pathc; ++i) 
				{
					char* searchBase = strrchr(gl.gl_pathv[i], FNSEPC);
					searchBase = (NULL == searchBase) ? gl.gl_pathv[i] : searchBase;
					memset(fileNameAppend, 0, sizeof(fileNameAppend));
					int matchYes = sscanf(searchBase,"%*[^.].%[0-9_-].%*[^.]", fileNameAppend);
					if (-1 < matchYes
						&& 5 == sscanf(fileNameAppend, "%[^-]-%[^_]_%[^-]-%[^_]_%[^-]", match, match, match, match, match))
					{
					    outVector.push_back(std::string(gl.gl_pathv[i]));
					}
				}
			}

			globfree(&gl);
#endif
			std::sort(outVector.begin(), outVector.end());
			return true;
		}

		void FileLogImp::RemoveAndCreateFile()
		{
			if (m_cYield >0)
			{
				--m_cYield;
				return;
			}
			else
				m_cYield = 0;

			std::vector<std::string> fileVec;
			CreateFileVec(fileVec, m_FileDir, m_FileShortName, "log");
			std::vector<std::string>::iterator itFile = fileVec.begin();
			for (int fileNum = fileVec.size(); itFile != fileVec.end() && fileNum >= m_nMaxLogfileNum;
				++itFile, --fileNum)
			{
				std::string fileNameTmp(*itFile);
				::remove(fileNameTmp.c_str());
			}

#ifdef ZQ_OS_MSWIN
			SYSTEMTIME time;
			GetLocalTime(&time);
			m_currentMonth = time.wMonth;//设置当前日期，以便发现日期改变
			int nCount = _snprintf(m_FileNameAppend, MAX_PATH-3, "%04d-%02d-%02d_%02d-%02d_%02d-%03d",
				time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#else
			time_t tmt;
			struct tm* ptm;
			struct timeval tv;
			
			time(&tmt);
			ptm = localtime(&tmt);
			gettimeofday(&tv,  (struct timezone*)NULL);
			m_currentMonth = ptm->tm_mon + 1;//set local month, base from 0
			int nCount = snprintf(m_FileNameAppend, MAX_PATH-3, "%04d-%02d-%02d_%02d-%02d_%02d-%03d",
				ptm->tm_year + 1900, ptm->tm_mon + 1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec,(int)tv.tv_usec/1000);
#endif
			if( NULL != _pNest->m_FileStream && _pNest->m_FileStream->is_open())
			{// 关闭当前读写的文件
				_pNest->m_FileStream->close();
				delete _pNest->m_FileStream;
				_pNest->m_FileStream = NULL;
			}

			if(NULL == _pNest->m_FileStream)
				_pNest->m_FileStream = new std::ofstream();

			if(NULL == _pNest->m_FileStream)
				throw FileLogException("Failed to new a fstream");

			snprintf(m_FileName, sizeof(m_FileName)-1, "%s/%s.%s.log", m_FileDir, m_FileShortName, m_FileNameAppend);
			_pNest->m_FileStream->open(m_FileName, std::ios::out | std::ios::binary);// 创建一个新的文件，并置文件大小为0
			m_nCurrentFileSize = 0;
			if(!_pNest->m_FileStream->is_open())
			{	
				m_cYield = 100;
				delete _pNest->m_FileStream;
				_pNest->m_FileStream = NULL;

				SYSTEMFILELOG(Log::L_EMERG,"Create file [%s] fail!", m_FileName);
				throw FileLogException("Create file fail!");
			}

			m_stampLastReportedFailure = 0;
		}

		int FileLogImp::run_interval()
		{
			try{
				flush();
			}
			catch(...) {}

			return 0;
		}

		void FileLogImp::flush()
		{
			MutexGuard lk(m_buffMtx);
			flushData();
		}

		const char* FileLogImp::getLogFilePathname() const
		{
			return m_FileName;
		}

		void FileLogImp::setFileSize(const int& fileSize)
		{
			MutexGuard lk(m_buffMtx);

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

		void FileLogImp::setFileCount(const int& fileCount)
		{
			MutexGuard lk(m_buffMtx);

			m_nMaxLogfileNum = fileCount;					//设置log文件的数目, Min_FileNum ~ Max_FileNum个
			if(m_nMaxLogfileNum > Max_FileNum)
				m_nMaxLogfileNum = Max_FileNum;
			if (m_nMaxLogfileNum < Min_FileNum)
				m_nMaxLogfileNum = Min_FileNum;
		}

		void FileLogImp::setBufferSize(const int& buffSize)
		{
			MutexGuard lk(m_buffMtx);

			flushData();

			//销毁缓冲区
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
			m_Buff = new char[m_nMaxBuffSize];					//分配缓冲区
			memset(m_Buff, 0, m_nMaxBuffSize);
		}

		void FileLogImp::setLevel(const int& level)
		{
			MutexGuard lk(m_buffMtx);
			setVerbosity(level);
		}

	}// namespace common
}// namespace ZQ

