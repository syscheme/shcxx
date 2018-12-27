#include "MdbLog.h"

// "create table ote_cm_app(cm_group_id int primary key NOT NULL, app_type char(200))"

namespace ZQ
{
namespace common
{

MdbLogError::MdbLogError(const std::string &what_arg) // throw()
: IOException(what_arg)
{
}

MdbLogError::~MdbLogError() // throw()
{
}

//////////////////////////////////////////////////////////////////////////
// the implemetation of ModLogThread
//////////////////////////////////////////////////////////////////////////

MdbLogThread::MdbLogThread() : _bQuit(false)
{
}

MdbLogThread::~MdbLogThread()
{
	try {stop();} catch (...) {}
}

bool MdbLogThread::init(void)
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

void MdbLogThread::final(void)
{
}

int MdbLogThread::run()
{
	while (!_bQuit)
	{
		{
		MutexGuard lk(_lockLogInsts);
		LogItor itor = _logInsts.begin();
		for (; itor != _logInsts.end(); itor ++)
		{
			(*itor)->run_interval();
		}
		}

#ifdef ZQ_OS_MSWIN
		WaitForSingleObject(_event, CheckDuration * 1000);
#else
		struct timespec ts;
		struct timeb tb;
		ftime(&tb);
		tb.time += CheckDuration;
		ts.tv_sec = tb.time;
		ts.tv_nsec = tb.millitm * 1000000;
		sem_timedwait(&_pthsem,&ts);
#endif
	}
	return 0;
}

void MdbLogThread::stop()
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

void MdbLogThread::addLogInst(MdbLog* logInst)
{
	MutexGuard lk(_lockLogInsts);
	_logInsts.push_back(logInst);
}

void MdbLogThread::rmvLogInst(MdbLog* logInst)
{
	MutexGuard lk(_lockLogInsts);
	LogItor itor = _logInsts.begin();
	for (; itor != _logInsts.end(); itor ++)
	{
		if ((*itor)->m_instIdent == logInst->m_instIdent)
		{
			_logInsts.erase(itor);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// the implemetation of ModLogThread
//////////////////////////////////////////////////////////////////////////

MdbLogThread* MdbLog::m_staticThread = NULL;
int MdbLog::m_lastInstIdent = 0;

MdbLog::MdbLog()
{
	_hDbEnv = SQL_NULL_HANDLE;
	_hDbConn = SQL_NULL_HANDLE;
	_event = NULL;
	_bExit = false;
}

MdbLog::~MdbLog()
{
	try {uninitialize();} catch (...) {}
}

void MdbLog::setError(SQLRETURN errRtn, SQLSMALLINT errHdlType, SQLHANDLE errHdl, char* errBuff, unsigned int buffSize)
{
	if (errRtn != SQL_ERROR && errRtn != SQL_SUCCESS_WITH_INFO)
		return;
	SQLCHAR sqlState[MAX_PATH], messageTest[MAX_PATH];
	memset(sqlState, 0, sizeof(sqlState));
	memset(messageTest, 0, sizeof(messageTest));
	SQLINTEGER nativeError;
	SQLSMALLINT textLenght;
	SQLGetDiagRec(errHdlType, errHdl, 1, sqlState, &nativeError, messageTest, sizeof(messageTest) / sizeof(SQLCHAR), &textLenght);
	errBuff[buffSize - 1] = '\0';
	_snprintf(errBuff, buffSize - 1, "%s: %s", (char*) sqlState, (char*) messageTest);
}

bool MdbLog::internalExec(const char* pSqlStr, char* errBuff, unsigned int errBuffSize)
{
	ZQ::common::MutexGuard lk(_lock);
	if (_bExit)
		return false;
	errBuff[errBuffSize - 1] = '\0';
	SQLHANDLE hStmt = SQL_NULL_HANDLE;
	SQLRETURN sqlRet = ::SQLAllocHandle(SQL_HANDLE_STMT, _hDbConn, &hStmt);
	if (sqlRet != SQL_SUCCESS/* && sqlRet != SQL_SUCCESS_WITH_INFO*/)
	{
		_snprintf(errBuff, errBuffSize - 1, "SQLAllocHandle SQL_HANDLE_STMT failed");
		setError(sqlRet, SQL_HANDLE_DBC, _hDbConn, errBuff, errBuffSize);
		return false;
	}
	sqlRet = ::SQLExecDirect(hStmt, (SQLCHAR*) pSqlStr, SQL_NTS);
	if (sqlRet != SQL_SUCCESS/* && sqlRet != SQL_SUCCESS_WITH_INFO*/)
	{
		_snprintf(errBuff, errBuffSize - 1, "SQLExecDirect %s failed", pSqlStr);
		setError(sqlRet, SQL_HANDLE_STMT, hStmt, errBuff, errBuffSize);
		::SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		return false;
	}
	::SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	return true;
}

bool MdbLog::executeSql(const char* pSqlStr, char* errBuff, unsigned int errBuffSize)
{
	return internalExec(pSqlStr, errBuff, sizeof(errBuff));
}

void MdbLog::executeSql(const char* pSqlStr)
{
	char errBuff[1024];
	if (!internalExec(pSqlStr, errBuff, sizeof(errBuff)))
		throw MdbLogError(errBuff);
}

int MdbLog::run_interval()
{
	WIN32_FIND_DATA finddata;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = ::FindFirstFile(_dbPath.c_str(), &finddata);
	if (hFile == INVALID_HANDLE_VALUE)
		return 1;
	::FindClose(hFile);
	
	// check the file is a directory name
	if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		return 1;
	
	unsigned int tFileSize;
	tFileSize = finddata.nFileSizeHigh * MAXDWORD + finddata.nFileSizeHigh + finddata.nFileSizeLow;
	if (tFileSize > _maxDBSize)
	{
		ZQ::common::MutexGuard lk(_lock);
		char errBuff[1024];
		closeDB(); // close database
		backupDB(); // backup current database and rename all the backuped database
		// create a new database by copying from the template database, I set parameter bFailIfExists
		// = TRUE so thast ::CopyFile will return FALSE when rename database has failed.
		BOOL bCopySucc = ::CopyFile(_dbTemplate.c_str(), _dbPath.c_str(), TRUE);
		openDB(errBuff, sizeof(errBuff)); // open database
		// if bCopySucc = FALSE means rename operation has failed and the database is the old database.
		// so the database contains all the scheme and I needn't to create it again.
		if (TRUE == bCopySucc && _dbScheme.size())
		{
			 if (!createScheme(errBuff, sizeof(errBuff)))
			 {
				 // here I can only ignore the errors when createScheme failed
				 _sysLog(::ZQ::common::Log::L_EMERG, "createScheme %s caught %s", _dbPath.c_str(), errBuff);
			 }
		}
	}
	return 0;
}

bool MdbLog::internalInit(const char* pDbPath, const char* pDbTemplate, 
						const std::vector<std::string> dbScheme, 
						unsigned int maxDBSize, unsigned int maxDBNum, 
						char* errBuff, unsigned int buffSize)
{
	// gain current module name
	char mdlName[MAX_PATH];
	memset(mdlName, 0, sizeof(mdlName));
	::GetModuleFileName(NULL, mdlName, sizeof(mdlName));
	_moduleName = mdlName;
	_sysLog.open(_moduleName.c_str(), 7);

	errBuff[buffSize - 1] = '\0';
	_maxDBSize = maxDBSize;
	_maxDBNum = maxDBNum;
	_dbPath = (NULL != pDbPath) ? pDbPath : "";
	_dbTemplate = (NULL != pDbTemplate) ? pDbTemplate : "";
	_dbScheme = dbScheme;
    // The DSN name length are limited to 32 char.
	_dsnName = getRightStr(_moduleName, "\\/", false) + "_";
	//_dsnName += _dbPath;
	//replaceChars(_dsnName, ".:\\/", '_');
	_user = getRightStr(_moduleName, "\\/", false);
	_pwd = getRightStr(_moduleName, "\\/", false);

	// ����db���ļ���СΪ1MB����������������СֵΪ2MB�����ֵΪ2000MB
	int nDBSizeInMB = _maxDBSize / (1024 * 1024);
	if (_maxDBSize % (1024 * 1024) != 0)
		nDBSizeInMB ++;
	if (nDBSizeInMB < 2)
		nDBSizeInMB = 2;
	if (nDBSizeInMB > 2000)
		nDBSizeInMB = 2000;
	_maxDBSize = nDBSizeInMB * 1024 * 1024;

	//����log�ļ�����Ŀ, Min_FileNum ~ Max_FileNum��
	if(_maxDBNum > Max_DBNum)
		_maxDBNum = Max_DBNum;
	if (_maxDBNum < Min_DBNum)
		_maxDBNum = Min_DBNum;

	_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == _event)
	{
		_snprintf(errBuff, buffSize - 1, "CreateEvent failed");
		return false;
	}

	// create ODBC environment handle
	SQLRETURN sqlRet = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hDbEnv);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLAllocHandle SQL_HANDLE_ENV failed");
		return false;
	}

	sqlRet = ::SQLSetEnvAttr(_hDbEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLSetEnvAttr SQL_ATTR_ODBC_VERSION failed");
		setError(sqlRet, SQL_HANDLE_ENV, _hDbEnv, errBuff, buffSize);
		::SQLFreeHandle(SQL_HANDLE_ENV, _hDbEnv);
		_hDbEnv = SQL_NULL_HANDLE;
		return false;
	}

	WIN32_FIND_DATA finddata;
	HANDLE hFile = ::FindFirstFile(_dbPath.c_str(), &finddata);
	if (hFile != INVALID_HANDLE_VALUE && ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
		backupDB();
	else if (hFile != INVALID_HANDLE_VALUE && ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
	{
		_snprintf(errBuff, buffSize - 1, "%s is a directory", _dbPath.c_str());
		_sysLog(ZQ::common::Log::L_EMERG, "%s", errBuff);
		return false;
	}
	::FindClose(hFile);

	if (FALSE == ::CopyFile(_dbTemplate.c_str(), _dbPath.c_str(), false))
	{
		_snprintf(errBuff, buffSize - 1, "copy [%s] to [%s] failed", 
			_dbTemplate.c_str(), _dbPath.c_str());
		return false;
	}

	char driver[MAX_PATH], attr[MAX_PATH];
	memset(driver, 0, sizeof(driver));
	memset(attr, 0, sizeof(attr));
	_snprintf(driver, sizeof(driver) - 1, "Microsoft Access Driver (*.MDB)");
	_snprintf(attr, sizeof(attr) - 1, "DSN=%s;Uid=%s;pwd=%s;DBQ=%s", 
		_dsnName.c_str(), _user.c_str(), _pwd.c_str(), _dbPath.c_str());
	SQLConfigDataSource(0, ODBC_REMOVE_DSN, driver, attr);
	_snprintf(attr, sizeof(attr) - 1, "DSN=%s;Uid=%s;pwd=%s;DBQ=%s", 
		_dsnName.c_str(), _user.c_str(), _pwd.c_str(), _dbPath.c_str());
	BOOL bSetOdbc = SQLConfigDataSource(0, ODBC_ADD_DSN, driver, attr);
	if (bSetOdbc == FALSE)
	{
		_snprintf(errBuff, buffSize - 1, "create DSN failed [%s]", attr);
		return false;
	}

	// DO: open the database
	{
		ZQ::common::MutexGuard lk(_lock);
		if (!openDB(errBuff, buffSize))
		{
			_sysLog(ZQ::common::Log::L_EMERG, "openDB %s caught error: %s", 
				_dbPath.c_str(), errBuff);
			return false;
		}
		if (_dbScheme.size() && !createScheme(errBuff, buffSize))
		{
			_sysLog(ZQ::common::Log::L_EMERG, "createScheme %s caught error: %s", 
				_dbPath.c_str(), errBuff);
			return false;
		}
	}

	{
		MutexGuard lk(m_lockLastInstIdent);
		// ���m_staticThreadû�д�������������֮
		if (NULL == m_staticThread)
		{
			m_staticThread = new MdbLogThread();
			if (NULL != m_staticThread)
				m_staticThread->start();
			else 
			{
				_snprintf(errBuff, buffSize - 1, "alloc static thread failed");
				_sysLog(ZQ::common::Log::L_EMERG, errBuff);
				return false;
			}
		}
		// ��ʱm_staticThread���벻ΪNULL
		if (NULL == m_staticThread)
		{
			_snprintf(errBuff, buffSize - 1, "Create static thread failed");
			_sysLog(ZQ::common::Log::L_EMERG, errBuff);
			return false;
		}
		m_instIdent = ++m_lastInstIdent;
		m_staticThread->addLogInst(this); // ��static threadע��
	}

	return true;
}

bool MdbLog::initialize(const char* pDbPath, const char* pDbTemplate, 
			const std::vector<std::string> dbScheme, 
			unsigned int maxDBSize, unsigned int maxDBNum, 
			char* errBuff, unsigned int buffSize)
{
	if (!internalInit(pDbPath, pDbTemplate, dbScheme, maxDBSize, maxDBNum, errBuff, buffSize))
		return false;
	return true;
}

void MdbLog::initialize(const char* pDbPath, const char* pDbTemplate, 
		const std::vector<std::string> dbScheme, 
		unsigned int maxDBSize, unsigned int maxDBNum)
{
	char errBuff[1024];
	if (!internalInit(pDbPath, pDbTemplate, dbScheme, maxDBSize, maxDBNum, errBuff, sizeof(errBuff)))
		throw MdbLogError(errBuff);
}

void MdbLog::uninitialize()
{
	// ��static thread����ע��
	if (m_staticThread)
	{
		MutexGuard lk(m_lockLastInstIdent);
		m_staticThread->rmvLogInst(this);
		if (m_staticThread->_logInsts.size() == 0)
		{
			try {delete m_staticThread;} catch (...) {}
			m_staticThread = NULL;
		}
	}

	// close database
	{
		ZQ::common::MutexGuard lk(_lock);
		closeDB();
	}

	// close ODBC environment handle
	if (_hDbEnv != SQL_NULL_HANDLE)
		::SQLFreeHandle(SQL_HANDLE_ENV, _hDbEnv);
	_hDbEnv = SQL_NULL_HANDLE;

	char driver[MAX_PATH], attr[MAX_PATH];
	memset(driver, 0, sizeof(driver));
	memset(attr, 0, sizeof(attr));
	_snprintf(driver, sizeof(driver) - 1, "Microsoft Access Driver (*.MDB)");
	_snprintf(attr, sizeof(attr) - 1, "DSN=%s;Uid=%s;pwd=%s;DBQ=%s", 
		_dsnName.c_str(), _user.c_str(), _pwd.c_str(), _dbPath.c_str());
	SQLConfigDataSource(0, ODBC_REMOVE_DSN, driver, attr);
}

bool MdbLog::backupDB()
{
	// noExtName: û����չ���Ĵ���·�����ļ���
	// extName: ��չ��
	// dirName: ·������βû��"\\"
	::std::string noExtName, extName, dirName;
	noExtName = getLeftStr(_dbPath, ".", false);
	extName = getRightStr(_dbPath, ".", false);
	dirName = getPath(_dbPath);
	//                 |       noExtName      | extName |
	// ���赱ǰ�ļ���Ϊ"directoryname\filename.extension"
	//                 |   dirName   |
	// DO: directoryname\filename.*.extensionƥ����ļ���
	::std::vector<::std::string> tempArray;
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

	// ���ļ���������������ķ��������ǰ��
	std::vector<int> vctInts;
	vctInts.clear();
	int i =0;
	int count = 0;
	for ( i = 0, count = tempArray.size(); i < count; i ++)
	{
		::std::string idxStr;
		idxStr = getRightStr(getLeftStr(tempArray[i], ".", false), ".", false);
		if (true == isInt(idxStr))
			increaseInsert(vctInts, atoi(idxStr.c_str()));
	}
	::std::vector<::std::string> fileArray;
	fileArray.clear();
	for ( i = 0, count = vctInts.size(); i < count; i ++)
	{
		char fullName[MAX_PATH];
		fullName[sizeof(fullName) - 1] = '\0';
		snprintf(fullName, sizeof(fullName) - 1, "%s.%d.%s", noExtName.c_str(), vctInts[i], extName.c_str());
		fileArray.push_back(fullName);
	}

	// ɾ�����ڵı����ļ�
	while (fileArray.size() >= _maxDBNum)
	{
		if (0 == ::DeleteFileA(fileArray.back().c_str()))
			_sysLog(ZQ::common::Log::L_WARNING, "fail to delete [%s]", fileArray.back().c_str());
		fileArray.pop_back();
	}

	// ��ʼrename
	for (i = fileArray.size() - 1; i >= 0; i --)
	{
		char newName[MAX_PATH];
		newName[sizeof(newName) - 1] = '\0';
		snprintf(newName, sizeof(newName) - 1, "%s.%d.%s", noExtName.c_str(), vctInts[i] + 1, extName.c_str());
		if (0 != rename(fileArray[i].c_str(), newName))
			_sysLog(ZQ::common::Log::L_WARNING, "fail to rename [%s] to [%s]", fileArray[i].c_str(), newName);
	}

	char newName[MAX_PATH];
	newName[sizeof(newName) - 1] = '\0';
	snprintf(newName, sizeof(newName) - 1, "%s.0.%s", noExtName.c_str(), extName.c_str());
	if (0 != rename(_dbPath.c_str(), newName))
		_sysLog(ZQ::common::Log::L_WARNING, "fail to rename [%s] to [%s]", _dbPath.c_str(), newName);

	return true;
}

bool MdbLog::openDB(char* errBuff, unsigned int buffSize)
{
	errBuff[buffSize - 1] = '\0';
	SQLRETURN sqlRet;
	if (_hDbConn != SQL_NULL_HANDLE)
	{
		::SQLDisconnect(_hDbConn);
		::SQLFreeHandle(SQL_HANDLE_DBC, _hDbConn);
	}
	_hDbConn = SQL_NULL_HANDLE;

	sqlRet = ::SQLAllocHandle(SQL_HANDLE_DBC, _hDbEnv, &_hDbConn);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLAllocHandle SQL_HANDLE_DBC failed");
		setError(sqlRet, SQL_HANDLE_ENV, _hDbEnv, errBuff, buffSize);
		::SQLFreeHandle(SQL_HANDLE_ENV, _hDbEnv);
		_hDbEnv = SQL_NULL_HANDLE;
		return false;
	}

	sqlRet = ::SQLSetConnectAttr(_hDbConn, SQL_LOGIN_TIMEOUT, (SQLPOINTER*) 5, 0);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLSetConnectAttr SQL_LOGIN_TIMEOUT failed");
		setError(sqlRet, SQL_HANDLE_DBC, _hDbConn, errBuff, buffSize);
		::SQLFreeHandle(SQL_HANDLE_DBC, _hDbConn);
		_hDbConn = SQL_NULL_HANDLE;
		::SQLFreeHandle(SQL_HANDLE_ENV, _hDbEnv);
		_hDbEnv = SQL_NULL_HANDLE;
		return false;
	}

	sqlRet = ::SQLConnect(_hDbConn, (SQLCHAR*) _dsnName.c_str(), SQL_NTS, (SQLCHAR*) _user.c_str(), SQL_NTS, (SQLCHAR*) _pwd.c_str(), SQL_NTS);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLConnect failed,  Parameter is (Source: %s, User: %s, Password: %s)", _dsnName.c_str(), _user.c_str(), _pwd.c_str());
		setError(sqlRet, SQL_HANDLE_DBC, _hDbConn, errBuff, buffSize);
		::SQLFreeHandle(SQL_HANDLE_DBC, _hDbConn);
		_hDbConn = SQL_NULL_HANDLE;
		::SQLFreeHandle(SQL_HANDLE_ENV, _hDbEnv);
		_hDbEnv = SQL_NULL_HANDLE;
		return false;
	}

//	if (!createScheme(errBuff, buffSize))
//		return false;

	return true;
}

bool MdbLog::createScheme(char* errBuff, unsigned int buffSize)
{
	errBuff[buffSize - 1] = '\0';
	SQLHANDLE hStmt = SQL_NULL_HANDLE;
	SQLRETURN sqlRet = ::SQLAllocHandle(SQL_HANDLE_STMT, _hDbConn, &hStmt);
	if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
	{
		_snprintf(errBuff, buffSize - 1, "SQLAllocHandle SQL_HANDLE_STMT failed");
		setError(sqlRet, SQL_HANDLE_DBC, _hDbConn, errBuff, buffSize);
		return false;
	}

	// do: create database scheme
	for (int i = 0, count = _dbScheme.size(); i < count; i ++)
	{
		sqlRet = ::SQLExecDirect(hStmt, (SQLCHAR*) _dbScheme[i].c_str(), SQL_NTS);
		if (sqlRet != SQL_SUCCESS && sqlRet != SQL_SUCCESS_WITH_INFO)
		{
			_snprintf(errBuff, buffSize - 1, "SQLExecDirect %s failed", _dbScheme[i].c_str());
			setError(sqlRet, SQL_HANDLE_STMT, hStmt, errBuff, buffSize);
			::SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
			return false;
		}
	}

	::SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	return true;
}

void MdbLog::closeDB()
{
	if (_hDbConn != SQL_NULL_HANDLE)
	{
		::SQLDisconnect(_hDbConn);
		::SQLFreeHandle(SQL_HANDLE_DBC, _hDbConn);
	}
	_hDbConn = NULL;
}

std::string MdbLog::leftStr(const std::string& cstStr, int pos)
{
	int size = cstStr.size();
	if (size -1 < pos)
	{
		return cstStr;
	}
	int cur = 0;
	std::string strRet;
	for (; cur < pos; cur++)
	{
		strRet += cstStr[cur];
	}
	return strRet;
}

std::string MdbLog::getLeftStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
{
	std::string::size_type null_pos = -1, find_pos = -1;
	if (first)
	{
		find_pos = cstStr.find_first_of(splitStr);
		if (find_pos != null_pos) 
		{
			return (leftStr(cstStr,find_pos));
		}
		else 
		{
			return std::string("");
		}
	}
	else 
	{
		find_pos = cstStr.find_last_of(splitStr);
		if (find_pos != null_pos) 
		{
			return (leftStr(cstStr,find_pos));
		}
		else 
		{
			return std::string("");
		}
	}
}

std::string MdbLog::rightStr(const std::string& cstStr, int pos)
{
	if (pos <= -1)
	{
		return cstStr;
	}
	std::string strRet;
	int len = cstStr.size();
	for (int cur = pos + 1; cur < len; cur++)
	{
		strRet += cstStr[cur];
	}
	return strRet;
}

std::string MdbLog::getRightStr(const std::string& cstStr, const std::string& splitStr, bool first/* = true*/)
{
	std::string::size_type null_pos = -1, find_pos = -1;
	if (first)
	{
		find_pos = cstStr.find_first_of(splitStr);
		if (find_pos != null_pos)
		{
			return rightStr(cstStr,find_pos);
		}
		else
		{
			return std::string("");
		}
	}
	else
	{
		find_pos = cstStr.find_last_of(splitStr);
		if (find_pos != null_pos)
		{
			return rightStr(cstStr,find_pos);
		}
		else
		{
			return std::string("");
		}
	}
}

std::string MdbLog::replaceChar(std::string& Str, const char& from, const char& to)
{
	int size = Str.size();
	for (int i = 0; i < size; i++)
	{
		if (Str[i] == from)
		{
			Str[i] = to;
		}
	}
	return Str;
}

std::string MdbLog::replaceChars(std::string& Str, const std::string& from, const char& to)
{
	int size = from.size();
	for (int tmp_cur = 0; tmp_cur < size; tmp_cur++)
	{
		replaceChar(Str, from[tmp_cur], to);
	}
	return Str;
}

std::string MdbLog::midStr(const std::string& cstStr, int f_pos, int l_pos)
{
	if (f_pos >= l_pos)
	{
		return "";
	}
	if (f_pos < -1)
	{
		f_pos = -1;
	}
	int size = cstStr.size();
	if (l_pos > size)
	{
		l_pos = size;
	}
	int cur = f_pos + 1;
	std::string strRet;
	for (; cur < l_pos; cur++) {
		strRet += cstStr[cur];
	}
	return strRet;
}

void MdbLog::splitStr(const std::string& cstStr, const std::string split, std::vector<std::string>& strVect)
{
	std::string tmp;
	strVect.clear();
	std::string::size_type null_pos = -1, find_pos = -1, last_find_pos = -1;
	find_pos = cstStr.find_first_of(split);
	while (find_pos != null_pos)
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

std::string MdbLog::nLeftStr(const std::string& cstStr, int num)
{
	return leftStr(cstStr, num);
}

std::string MdbLog::nRightStr(const std::string& cstStr, int num)
{
	return rightStr(cstStr, cstStr.size() - num - 1);
}

bool MdbLog::isInt(const std::string& cstStr)
{
	std::string::size_type null_pos = -1;
	std::string::size_type find_pos = -1;
	int size = cstStr.size();
	if (size == 0)
		return false;
	find_pos = cstStr.find_first_not_of("0123456789");
	if (find_pos != null_pos)
		return false;
	return true;
}

std::string MdbLog::getPath(const std::string& cstStr)
{
	return getLeftStr(cstStr, "\\/",false);
}

void MdbLog::increaseInsert(std::vector<int>& vctInts, int valInt)
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

} // namespace common
} // namespace ZQ

