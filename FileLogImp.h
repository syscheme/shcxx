#ifndef __FILE_LOG_IMP_H
#define __FILE_LOG_IMP_H


//////////////////////////////////////////////////////////////////////////
// FileLog.h: interface for the FileLog class.
// Author: copyright (c) Han Guan
//////////////////////////////////////////////////////////////////////

// ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע��
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
// ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע�⣬ʹ��ʱ��ע��

#include "ZQ_common_conf.h"
#include "NativeThread.h"
#include "Locks.h"

#include <vector>

#define		Min_FileNum 2
#define		Max_FileNum 30
#define		ZQLOG_DEFAULT_FILENUM		10					// �������м���log�����ļ�
#define		ZQLOG_DEFAULT_FILESIZE		1024*1024*10		// Ĭ��log�ļ���С����λ�ֽ�
#define		ZQLOG_DEFAULT_BUFFSIZE		8*1024				// Ĭ�ϻ�������С����λ�ֽ�
#define		ZQLOG_DEFAULT_MAXLINESIZE	2*1024				// Ĭ��ÿ������ַ���
#define		ZQLOG_DEFAULT_FLUSHINTERVAL 2					// ��λ�룬ÿ��೤ʱ�佫�������е�����д���ļ������ܻ�������û����
#define		ZQLOG_DEFAULT_EVENTLOGLEVEL	ZQ::common::Log::L_EMERG	// ��С�ڻ���ڸü����log�ᱻͬʱд�뵽ϵͳ��EventLog����

namespace ZQ {
namespace common {


class ZQ_COMMON_API FileLogImp;
class ZQ_COMMON_API FileLogException;

class FileLogException : public IOException
{
public:
	FileLogException(const std::string &what_arg); // throw();
	virtual ~FileLogException(); // throw();
};

class FileLogImp;
class FileLogNest;

class LogThread : public NativeThread
{
	friend class FileLogImp;
protected: 
	LogThread();
	virtual ~LogThread();
	void stop();
	void addLogInst(FileLogImp* logInst);
	void rmvLogInst(FileLogImp* logInst);

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
	typedef std::vector<FileLogImp*> LogVector;
	typedef LogVector::iterator LogItor;
	LogVector _logInsts;
	Mutex _lockLogInsts;
};

class FileLogImp : public Log
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
	// filename��ָ��log�ļ�������
	// verbosity��ָ��д��log�ļ�����ͼ��𣬵�����������log���ᱻд�뵽log�ļ���
	// logFileNum��ָ��������log�����ļ�����Ŀ��Ĭ��ֵΪ ZQLOG_DEFAULT_FILENUM = 5
	// fileSize��ָ��log�ļ��Ĵ�С��Ĭ��ֵΪ ZQLOG_DEFAULT_FILESIZE = 1024*1024*10(byte)������10MB��
	// buffSize��ָ����log�ļ��䱸�Ļ������Ĵ�С��Ĭ��ֵΪ ZQLOG_DEFAULT_BUFFSIZE = 8024(byte)��
	// flushInterval��ָ����ʱ��������������д�뵽log�ļ���ʱ����Ĭ��ֵΪ ZQLOG_DEFAULT_FLUSHINTERVAL = 2(s)
	// eventLogLevel����д���log�ļ���С�ڻ����ָ���ļ���ʱ����logҲ����д�뵽ϵͳ��EventLog�У�Ĭ�ϼ���Ϊ ZQLOG_DEFAULT_EVENTLOGLEVEL (Log::L_CRIT = 2)��
	// appName����ʾ��ϵͳEventLog�е�����
	// Exception��
	// ����ļ��������ˣ����׳�һ��FileLogException�쳣
	FileLogImp(const char* filename, \
		const int verbosity=L_ERROR, \
		int logFileNum = ZQLOG_DEFAULT_FILENUM, \
		int fileSize =ZQLOG_DEFAULT_FILESIZE, \
		int buffersize =ZQLOG_DEFAULT_BUFFSIZE, \
		int flushInterval =ZQLOG_DEFAULT_FLUSHINTERVAL, \
		int eventLogLevel = ZQLOG_DEFAULT_EVENTLOGLEVEL, \
		const char* appName = NULL);

	FileLogImp();

	// Params:
	// filename��ָ��log�ļ�������
	// verbosity��ָ��д��log�ļ�����ͼ��𣬵�����������log���ᱻд�뵽log�ļ���
	// logFileNum��ָ��������log�����ļ�����Ŀ��Ĭ��ֵΪ ZQLOG_DEFAULT_FILENUM = 5
	// fileSize��ָ��log�ļ��Ĵ�С��Ĭ��ֵΪ ZQLOG_DEFAULT_FILESIZE = 1024*1024*10(byte)������10MB��
	// buffSize��ָ����log�ļ��䱸�Ļ������Ĵ�С��Ĭ��ֵΪ ZQLOG_DEFAULT_BUFFSIZE = 8024(byte)��
	// flushInterval��ָ����ʱ��������������д�뵽log�ļ���ʱ����Ĭ��ֵΪ ZQLOG_DEFAULT_FLUSHINTERVAL = 2(s)
	// eventLogLevel����д���log�ļ���С�ڻ����ָ���ļ���ʱ����logҲ����д�뵽ϵͳ��EventLog�У�Ĭ�ϼ���Ϊ ZQLOG_DEFAULT_EVENTLOGLEVEL (Log::L_CRIT = 2)��
	// appName����ʾ��ϵͳEventLog�е�����
	// Exception��
	// ����ļ��������ˣ����׳�һ��FileLogException�쳣
	void open(const char* filename, \
		const int verbosity=L_ERROR, \
		int logFileNum = ZQLOG_DEFAULT_FILENUM, \
		int fileSize =ZQLOG_DEFAULT_FILESIZE, \
		int buffersize =ZQLOG_DEFAULT_BUFFSIZE, \
		int flushInterval =ZQLOG_DEFAULT_FLUSHINTERVAL, \
		int eventLogLevel = ZQLOG_DEFAULT_EVENTLOGLEVEL, \
		const char* appName = NULL);

	virtual ~FileLogImp();

	// ǿ�ƽ��������������д���ļ�
	void flush();

	const char* getLogFilePathname() const;

	void setFileSize(const int& fileSize);
	void setFileCount(const int& fileCount);
	void setBufferSize(const int& buffSize);
	void setLevel(const int& level);

protected: 
	// ������Դ
	void clear();

	// called by LogThread
	int run_interval();

	//�򻺳���д������
	virtual void writeMessage(const char *msg, int level=-1);
	virtual void writeMessage(const wchar_t *msg, int level=-1);

	//ǿ�ƽ�����������д��log�ļ�
	virtual void flushData();

	//��log�ļ���صı���
protected:
	SysLog		m_SysLog;					//ϵͳ�¼�log
	int			m_nMaxLogfileNum;			//���log�ļ���Ŀ
	int			m_nCurrentFileSize;			//��ǰ�ļ�size
	int			m_nMaxFileSize;				//�ļ����size
	char		m_FileDir[MAX_PATH];		//log·��
	char		m_FileName[MAX_PATH];		//log�ļ���
	char		m_FileShortName[MAX_PATH];  //log��ȥʱ���׺��".log"����ļ���
	char        m_FileNameAppend[MAX_PATH]; //��ǰ�ļ���ʱ���׺
	FileLogNest* _pNest;

	//��log��������صı���
protected:
	void	createDir(const std::string& dirName);
	void	RemoveAndCreateFile();
	int     CreateFileVec(std::vector<std::string>& outVector, const char* dirname, const char* filename, const char* extname);
	bool	IsFileExsit(const char* filename,int& retFileSize);
	char*			m_Buff;						//������ָ��
	int				m_nMaxBuffSize;				//���������size
	int				m_nCurrentBuffSize;			//��ǰ������size
	Mutex			m_buffMtx;					//������������
	int				m_nFlushInterval;			//��ʱ���������е�����д��log�ļ�
	uint16			m_currentMonth;				//��ǰ�·�
	int				m_eventLogLevel;			//д��ϵͳEventLog��log����
	int				m_instIdent;

	static LogThread* m_staticThread;
	static int		m_lastInstIdent;			// ��¼��һ��log instance��identity
	Mutex			m_lockLastInstIdent;
	int				m_cYield;
	int64 m_stampLastReportedFailure; // the timestamp of last rolling file failure that had been reported to system
};

}//namespace common
}//namespace zq

#endif//__FILE_LOG_IMP_H