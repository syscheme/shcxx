
//////////////////////////////////////////////////////////////////////////
// FileLog.h: interface for the FileLog class.
// Author: copyright (c) Han Guan
//////////////////////////////////////////////////////////////////////

// 使用时请注意，使用时请注意，使用时请注意，使用时请注意，使用时请注意，使用时请注意
//////////////////////////////////////////////////////////////////////////
// You can create an object of FileLog like the following.
// 1. FileLog _logFile;
//    _logFile.open(parameter list);
// 2. FileLog* _logFile = new FileLog(parameter list);

// I don't think it's a good idea to use the FileLog like the second method.
// Because when the application crashes, you don't have the opportunity to call
// destructor of FileLog in order to flush the data remained in the buffer of
// the FileLog object. then we'll lose lose the data in the buffer. 
// If you use the first method, the data in the buffer will be automatically flushed 
// into the file. Because the C run-time will call the destructor of FileLog which 
// will flush the data in the buffer to file when the application crushes.
//////////////////////////////////////////////////////////////////////////
// 使用时请注意，使用时请注意，使用时请注意，使用时请注意，使用时请注意，使用时请注意


#ifndef __ZQ_COMMON_FileLog_H__
#define __ZQ_COMMON_FileLog_H__

#include "ZQ_common_conf.h"
#include "NativeThread.h"
#include "Locks.h"

#include <vector>

#define		Min_FileNum 2
#define		Max_FileNum 30
#define		ZQLOG_DEFAULT_FILENUM		10					// 最多可以有几个log备份文件
#define		ZQLOG_DEFAULT_FILESIZE		1024*1024*10		// 默认log文件大小，单位字节
#define		ZQLOG_DEFAULT_BUFFSIZE		8*1024				// 默认缓冲区大小，单位字节
#define		ZQLOG_DEFAULT_MAXLINESIZE	2*1024				// 默认每行最多字符数
#define		ZQLOG_DEFAULT_FLUSHINTERVAL 2					// 单位秒，每隔多长时间将缓冲区中的数据写入文件，不管缓冲区有没有满
#define		ZQLOG_DEFAULT_EVENTLOGLEVEL	ZQ::common::Log::L_EMERG	// 当小于或等于该级别的log会被同时写入到系统的EventLog里面

namespace ZQ {
namespace common {


class ZQ_COMMON_API FileLog;
class ZQ_COMMON_API FileLogException;

class FileLogException : public IOException
{
public:
	FileLogException(const std::string &what_arg) throw();
	virtual ~FileLogException() throw();
};

class FileLog;
class FileLogNest;

class LogThread : public NativeThread
{
	friend class FileLog;
protected: 
	LogThread();
	virtual ~LogThread();
	void stop();
	void addLogInst(FileLog* logInst);
	void rmvLogInst(FileLog* logInst);

protected: // derived from native thread
	int run();
	void final(void);
	bool init(void);

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
	
class FileLog : public Log
{
	friend class FileLogNest;
	friend class LogThread;
	
protected: 
	std::string leftStr(const std::string& cstStr, int pos);
	std::string getLeftStr(const std::string& cstStr, const std::string& splitStr, bool first = true);	
	std::string rightStr(const std::string& cstStr, int pos);
	std::string getRightStr(const std::string& cstStr, const std::string& splitStr, bool first = true);
	std::string midStr(const std::string& cstStr, int f_pos, int l_pos);
	void splitStr(const std::string& cstStr, const std::string split, std::vector<std::string>& strVect);
	std::string nLeftStr(const std::string& cstStr, int num);
	std::string nRightStr(const std::string& cstStr, int num);
	bool isInt(const std::string& cstStr);
	std::string getPath(const std::string& cstStr);
	void increaseInsert(std::vector<int>& vctInts, int valInt);

public:
	// Params:
		// filename：指定log文件的名字
		// verbosity：指定写入log文件的最低级别，低于这个级别的log不会被写入到log文件中
		// logFileNum：指定产生的log备份文件的数目，默认值为 ZQLOG_DEFAULT_FILENUM = 5
		// fileSize：指定log文件的大小，默认值为 ZQLOG_DEFAULT_FILESIZE = 1024*1024*10(byte)，等于10MB。
		// buffSize：指定与log文件配备的缓冲区的大小，默认值为 ZQLOG_DEFAULT_BUFFSIZE = 8024(byte)。
		// flushInterval：指定定时将缓冲区的数据写入到log文件的时间间隔，默认值为 ZQLOG_DEFAULT_FLUSHINTERVAL = 2(s)
		// eventLogLevel：当写入的log的级别小于或等于指定的级别时，该log也将被写入到系统的EventLog中，默认级别为 ZQLOG_DEFAULT_EVENTLOGLEVEL (Log::L_CRIT = 2)。
		// appName：显示在系统EventLog中的名字
	// Exception：
		// 如果文件创建不了，会抛出一个FileLogException异常
	FileLog(const char* filename, \
		const int verbosity=L_ERROR, \
		int logFileNum = ZQLOG_DEFAULT_FILENUM, \
		int fileSize =ZQLOG_DEFAULT_FILESIZE, \
		int buffersize =ZQLOG_DEFAULT_BUFFSIZE, \
		int flushInterval =ZQLOG_DEFAULT_FLUSHINTERVAL, \
		int eventLogLevel = ZQLOG_DEFAULT_EVENTLOGLEVEL, \
		const char* appName = NULL);

	FileLog();

	// Params:
		// filename：指定log文件的名字
		// verbosity：指定写入log文件的最低级别，低于这个级别的log不会被写入到log文件中
		// logFileNum：指定产生的log备份文件的数目，默认值为 ZQLOG_DEFAULT_FILENUM = 5
		// fileSize：指定log文件的大小，默认值为 ZQLOG_DEFAULT_FILESIZE = 1024*1024*10(byte)，等于10MB。
		// buffSize：指定与log文件配备的缓冲区的大小，默认值为 ZQLOG_DEFAULT_BUFFSIZE = 8024(byte)。
		// flushInterval：指定定时将缓冲区的数据写入到log文件的时间间隔，默认值为 ZQLOG_DEFAULT_FLUSHINTERVAL = 2(s)
		// eventLogLevel：当写入的log的级别小于或等于指定的级别时，该log也将被写入到系统的EventLog中，默认级别为 ZQLOG_DEFAULT_EVENTLOGLEVEL (Log::L_CRIT = 2)。
		// appName：显示在系统EventLog中的名字
	// Exception：
		// 如果文件创建不了，会抛出一个FileLogException异常
	void open(const char* filename, \
		const int verbosity=L_ERROR, \
		int logFileNum = ZQLOG_DEFAULT_FILENUM, \
		int fileSize =ZQLOG_DEFAULT_FILESIZE, \
		int buffersize =ZQLOG_DEFAULT_BUFFSIZE, \
		int flushInterval =ZQLOG_DEFAULT_FLUSHINTERVAL, \
		int eventLogLevel = ZQLOG_DEFAULT_EVENTLOGLEVEL, \
		const char* appName = NULL);

	virtual ~FileLog();

	// 强制将缓冲区里的数据写入文件
	void flush();

	const char* getLogFilePathname() const;

	void setFileSize(const int& fileSize);
	void setFileCount(const int& fileCount);
	void setBufferSize(const int& buffSize);
	void setLevel(const int& level);

protected: 
	// 清理资源
	void clear();

	// called by LogThread
	int run_interval();
	
	//向缓冲区写入数据
	virtual void writeMessage(const char *msg, int level=-1);
	virtual void writeMessage(const wchar_t *msg, int level=-1);
	
	//强制将缓冲区数据写入log文件
	virtual void flushData();

	//与log文件相关的变量
protected:
	SysLog		m_SysLog;					//系统事件log
	int			m_nMaxLogfileNum;			//最多log文件数目
	int			m_nCurrentFileSize;			//当前文件size
	int			m_nMaxFileSize;				//文件最大size
	char		m_FileName[MAX_PATH];		//log文件名
	FileLogNest* _pNest;

	//与log缓冲区相关的变量
protected:
	void	createDir(const std::string& dirName);
	void	RenameAndCreateFile();
	bool	IsFileExsit(const char* filename,int& retFileSize);
	char*			m_Buff;						//缓冲区指针
	int				m_nMaxBuffSize;				//缓冲区最大size
	int				m_nCurrentBuffSize;			//当前缓冲区size
	Mutex			m_buffMtx;					//缓冲区保护锁
	int				m_nFlushInterval;			//定时将缓冲区中的数据写入log文件
	uint16			m_currentMonth;				//当前月份
	int				m_eventLogLevel;			//写入系统EventLog的log级别
	int				m_instIdent;

	static LogThread* m_staticThread;
	static int		m_lastInstIdent;			// 记录上一个log instance的identity
	Mutex			m_lockLastInstIdent;
	int				m_cYield;
    int64 m_stampLastReportedFailure; // the timestamp of last rolling file failure that had been reported to system
};

}//namespace common
}//namespace zq

#endif  // __ZQ_COMMON_FileLog_H__
