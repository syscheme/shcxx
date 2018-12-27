#ifndef __ZQ_COMMON_MdbLog_H__
#define __ZQ_COMMON_MdbLog_H__

#include "NativeThread.h"
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <Odbcinst.h>
#include <string>
#include <vector>
#include <Exception.h>
#include <Log.h>
#include <Locks.h>

namespace ZQ
{
namespace common
{

#define Min_DBNum 3
#define Max_DBNum 30
#define DEFAULT_DBNUM 5
#define DEFAULT_DBSIZE 10 * 1024 * 1024

class MdbLogError : public ::ZQ::common::IOException
{
public: 
	MdbLogError(const std::string &what_arg) throw();
	virtual ~MdbLogError() throw();
};

class MdbLog;
class MdbLogThread : public NativeThread
{
#define CheckDuration 5 // defines the backup database interval in seconds
	friend class MdbLog;
protected: 
	MdbLogThread();
	virtual ~MdbLogThread();
	void stop();
	void addLogInst(MdbLog* logInst);
	void rmvLogInst(MdbLog* logInst);

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
typedef std::vector<MdbLog*> LogVector;
typedef LogVector::iterator LogItor;
	LogVector _logInsts;
	Mutex _lockLogInsts;
};
	
//////////////////////////////////////////////////////////////////////////
// the class is provided for writing access database only, it doesn't support query sentences like "select * ..."
// the access database could be renamed and be backuped when it's size is larger than the max size which
// you can specify when you call its initialize function, then a new database will be created by copying
// from the template database. as for template database it must contains all the schemes(tables, precedures etc) when
// the initialize parameter dbScheme's size is zero, the parameter dbScheme can also specified database schemes.
// 1. define an instance
// 2. call the initialize before you start use it to store data
// 3. if the instance is created dynamically(by new operation), you should delete 
// it when you won't use it again.
//////////////////////////////////////////////////////////////////////////
class MdbLog
{
	friend class MdbLogThread;
public: 
	MdbLog();
	virtual ~MdbLog();

	// execute the given sql string, this function doesn't return a record set
	// sqlStr[in], the sql statement to be execute
	// errBuff[out], the buff to stores error messages when this operation failed
	// errBuffSize[in], the size of errBuff
	bool executeSql(const char* pSqlStr, char* errBuff, unsigned int errBuffSize);

	// execute the given sql string, this function doesn't return a record set
	// sqlStr[in], the sql statement to be execute
	// throw ZQ::common::MdbLogError to indicates a error
	void executeSql(const char* pSqlStr);

	// pDbPath[in], the database's path in file system
	// pDbTemplate[in], specify a path for the template database. more about template database please
	// refer to the class reference above
	// maxDBSize[in], the maximum db site in byte
	// maxDBNum[in], the max number of backup db files
	// errBuff[out], the buff to stores error messages when this operation failed
	// buffSize[in], the size of errBuff
	// if an error occurs this function return false, and the error message is stored in the buffer you specified
	bool initialize(const char* pDbPath, const char* pDbTemplate, 
		const std::vector<std::string> dbScheme, 
		unsigned int maxDBSize, unsigned int maxDBNum, 
		char* errBuff, unsigned int buffSize);

	// pDbPath[in], the database's path in file system
	// pDbTemplate[in], specify a path for the template database. more about template database please
	// refer to the class reference above
	// maxDBSize[in], the maximum db site in byte
	// maxDBNum[in], the max number of backup db files
	// if an error occurs this function will throw an exception of MdbLogError
	void initialize(const char* pDbPath, const char* pDbTemplate, 
		const std::vector<std::string> dbScheme, 
		unsigned int maxDBSize, unsigned int maxDBNum);

protected: 
	bool internalInit(const char* pDbPath, const char* pDbTemplate, 
		const std::vector<std::string> dbScheme, 
		unsigned int maxDBSize, unsigned int maxDBNum, 
		char* errBuff, unsigned int buffSize);
	bool internalExec(const char* pSqlStr, char* errBuff, unsigned int errBuffSize);

	void uninitialize();

	// functionss for operation string
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
	std::string replaceChar(std::string& Str, const char& from, const char& to);
	std::string replaceChars(std::string& Str, const std::string& from, const char& to);

	bool createScheme(char* errBuff, unsigned int buffSize);
	bool openDB(char* errBuff, unsigned int buffSize);
	bool backupDB();
	void closeDB();
	void setError(SQLRETURN errRtn, SQLSMALLINT errHdlType, SQLHANDLE errHdl, char* errBuff, unsigned int buffSize);

	virtual int run_interval();

protected: 
	std::string _dsnName;
	std::string _dbPath;
	std::string _dbTemplate;
	std::string _user;
	std::string _pwd;
	unsigned int _maxDBSize;
	unsigned int _maxDBNum;
	SQLHANDLE _hDbEnv;
	SQLHANDLE _hDbConn;
	HANDLE _event;
	bool _bExit;
	::ZQ::common::SysLog _sysLog;
	::ZQ::common::Mutex _lock;
	::std::vector<std::string> _dbScheme;
	std::string _moduleName;
	int				m_instIdent;

	static MdbLogThread* m_staticThread;
	static int		m_lastInstIdent;			// ��¼��һ��log instance��identity
	Mutex			m_lockLastInstIdent;
};

} // namespace common
} // namespace ZQ

#endif __ZQ_COMMON_MdbLog_H__

