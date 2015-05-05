#ifndef _XODBC_H_
#define _XODBC_H_

#ifdef WIN32
	#include <windows.h>
	#include <sqltypes.h>
#else
	#include <sqltypes.h>	// Will define same windows data type on linux platform
	#include <wintype.h>	// Additive windows types
#endif

#include <sql.h>
#include <sqlext.h>

#include <xcomm.h>
#include <xstring.h>
#include <xlist.h>
#include <xmap.h>
#include <tchar.h>

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

//////////////////////////////////////////////////////////////////////////

#define MAX_BUFFER			256
#define MAX_COL_NAME_LEN	128
#define MAX_CURSOR_NAME		32
#define MAX_CONNECT_LEN		512

//////////////////////////////////////////////////////////////////////////
#define FIELD_STATUS_NULL		0	// indecat the field is NULL
#define FIELD_STATUS_DIRTY		1	// indecat the field is dirty

class XDBException {
public:
	XDBException(SQLHANDLE hSrc, RETCODE rc)
	{
		m_hSrc = hSrc;
		m_rc = rc;
	}

	RETCODE		m_rc;
	SQLHANDLE	m_hSrc;
};

class XDBObj {
protected:
	virtual void ThrowDBException(SQLRETURN nRetCode) = 0;
};

class XDatabase : public XDBObj {
public:

	enum drvCompletion
	{
		sqlNoPrompt = SQL_DRIVER_NOPROMPT,
		sqlPrompt = SQL_DRIVER_PROMPT
	};

protected:

	virtual BOOL CheckRetCode(SQLRETURN nRetCode) const;

	void FreeDbc();
	SQLHDBC AllocDbc();

public:
	XDatabase();
	~XDatabase();

	SQLHDBC			m_hDbc;

protected:
	virtual void ThrowDBException(SQLRETURN nRetCode)
	{
		throw XDBException(m_hDbc, nRetCode);
	}

	static SQLHENV GetEnv();
	static void ReleaseEnv();

protected:
	static SQLHENV	s_hEnv;
	static long		s_lRefCount;

	LONG			m_lLoginTimeout;
	LONG			m_lConnectionTimeout;
	BOOL			m_bIsConnected;
	int				m_nRowsAffected;

	char			m_chIDQuoteChar;

public:
	BOOL Open(LPCTSTR lpstrDSN, LPCTSTR lpstrUser = NULL, LPCTSTR lpstrPass = NULL);
	BOOL DriverConnect(LPCTSTR szConnStr, LPCTSTR szConnStrOut = NULL, HWND hWnd = NULL, enum drvCompletion drvConn = sqlNoPrompt);
	void SetReadOnly(BOOL bReadOnly = TRUE);
	void SetConnectionTimeout(LONG nSeconds);
	LONG GetConnectionTimeout();
	void SetLoginTimeout(LONG nSeconds)
		{m_lLoginTimeout = nSeconds;};
	BOOL Execute(LPCTSTR szSqlStr);
	int GetRowsAffected()
		{return m_nRowsAffected;};
	BOOL Browse(UCHAR* lpstrConnStrIn, UCHAR* lpstrConnStrOut);
	BOOL IsOpen()
		{return m_bIsConnected;};
	void Close();
	void ReplaceBrackets(LPTSTR lpchSQL);
	
};

struct XFiledInfo {
	BYTE	m_ucStatus;
	int		m_nDataType;
	int		m_nDataLen;
};

class XDBDateTime;
class XFieldExchange;

class XRecordset : public XDBObj {
public:
	enum datatypeEnum
	{
		typeChar = SQL_CHAR, 
		typeVarChar = SQL_VARCHAR, 
		typeLongVarChar = SQL_LONGVARCHAR, 
		typeWChar = SQL_WCHAR, 
		typeWVarChar = SQL_WVARCHAR,
		typeWLongVarChar = SQL_WLONGVARCHAR,
		typeDecimal = SQL_DECIMAL,
		typeNumeric = SQL_NUMERIC,
		typeSmallint = SQL_SMALLINT,
		typeInteger = SQL_INTEGER,
		typeReal = SQL_REAL,
		typeFloat = SQL_FLOAT,
		typeDouble = SQL_DOUBLE,
		typeBit = SQL_BIT,
		typeTinyint = SQL_TINYINT,
		typeBigInt = SQL_BIGINT,
		typeBinary = SQL_BINARY,
		typeVarBinary = SQL_VARBINARY,
		typeLongVarBinary =  SQL_LONGVARBINARY,
		typeDate = SQL_TYPE_DATE,
		typeTime = SQL_TYPE_TIME,
		typeTimeStamp = SQL_TYPE_TIMESTAMP,
		typeIntervalMonth = SQL_INTERVAL_MONTH,
		typeIntervalYear = SQL_INTERVAL_YEAR,
		typeIntervalYearToMonth = SQL_INTERVAL_YEAR_TO_MONTH,
		typeIntervalDay = SQL_INTERVAL_DAY,
		typeIntervalHour = SQL_INTERVAL_HOUR,
		typeIntervalMinute = SQL_INTERVAL_MINUTE,
		typeIntervalSecond = SQL_INTERVAL_SECOND,
		typeIntervalDayToHour = SQL_INTERVAL_DAY_TO_HOUR,
		typeIntervalDayToMinute = SQL_INTERVAL_DAY_TO_MINUTE,
		typeIntervalDayToSecond = SQL_INTERVAL_DAY_TO_SECOND,
		typeIntervalHourToMinute = SQL_INTERVAL_HOUR_TO_MINUTE,
		typeIntervalHourToSecond = SQL_INTERVAL_HOUR_TO_SECOND,
		typeIntervalMinuteToSecond = SQL_INTERVAL_MINUTE_TO_SECOND,
		typeGUID = SQL_GUID		
	};

	enum EditMode
	{
		noMode,
		editMode,
		addnewMode,
		deleteMode
	};

	XRecordset(XDatabase* pDb = NULL);
	~XRecordset();

	BOOL Open(LPCTSTR szSqlStr = NULL);
	void Close();

	LONG GetRecordCount();

	LONG GetFieldLength(int nField);
	int GetFieldIndex(LPCTSTR szFieldName);
	BOOL GetFieldName(int nField, LPTSTR szFieldName);
	BOOL GetFieldAttributes(int nField, LPCTSTR szFieldName, int& nType, int& nLength);
	int GetFieldCount();

	// LPTSTR
	BOOL GetFieldValue(int nField, LPTSTR szData);
	BOOL GetFieldValue(LPCTSTR szFieldName, LPTSTR  szData);

	// XString
	BOOL GetFieldValue(int nField, XString& strData);
	BOOL GetFieldValue(LPCTSTR szFieldName, XString& strData);

	// long
	BOOL GetFieldValue(int nField, LONG& lData);
	BOOL GetFieldValue(LPCTSTR szFieldName, LONG& lData);

	// doublue
	BOOL GetFieldValue(int nField, DOUBLE& dblData);
	BOOL GetFieldValue(LPCTSTR szFieldName, DOUBLE& dblData);

	// date time
	BOOL GetFieldValue(int nField, XDBDateTime& dt);
	BOOL GetFieldValue(LPCTSTR szFieldName, XDBDateTime& dt);

	void Move(long nRows, WORD wFetchType = SQL_FETCH_RELATIVE);
	void MoveFirst();
	void MoveNext();
	void MovePrevious();
	void MoveLast();
	
	BOOL IsEof() 
		{return m_bIsEOF;};
	BOOL IsBof()
		{return m_bIsBOF;};

	virtual void Edit();
	virtual void AddNew();
	virtual void Delete();
	virtual BOOL Update();

	virtual void DoFieldExchange(XFieldExchange* ) {}

	virtual BOOL CheckRetCode(RETCODE nRetCode) const;

	virtual void ThrowDBException(SQLRETURN nRetCode)
	{
		throw XDBException(m_hStmt, nRetCode);
	}


protected:

	// void AllocFieldInfo();
	// void FreeFieldInfo();

	XFiledInfo* GetFieldInfo(int nIndex);
	
	// Include UPDATE , INSERT and DELETE SQL
	// If the filed data was changed, I'll update all, 
	// I wan't code about cache.
	void BuildUpdateSQL();
	void ExecuteUpdateSQL();

	void BuildEdit();
	void BuildAddNew();
	void BuildDelete();
	void PrepareUpdateHstmt();

	// get table name and updatable
	void AnalyseSelectSQL();

	void BindColums();
	void AppendNames();
	void AppendValues();
	void AppendNamesValues();
	void SetFieldNull();

	HSTMT AllocStmt();
	void FreeStmt();

	virtual void GetDefaultConnect(XString& strDSN, XString& strUser, XString& strPwd) 
	{
	
	}

	virtual XString GetDefaultSQL()
	{
		return _T("");
	}

	void GetCursorName();
	
	RETCODE FetchData(SDWORD nRow, UWORD wFetchType);

public:

	SQLHSTMT	m_hStmt;
protected:
	
	BOOL		m_bIsEOF;
	BOOL		m_bIsBOF;

	int			m_nResultCols;	// catch the count of columns in the statement
	int			m_nBoundCols;	// the count of bound columns
	BOOL		m_bUpdatable;	// query is updatable
	
	/*
	 * if it wasn't specified database object, 
	 * recrodset must make one by oneself.
	 */

	XDatabase*	m_pDatabase;
	BOOL		m_bRecordsetDb;	

	SQLHSTMT	m_hStmtUpdate;
	EditMode	m_nEditMode;
	XString		m_strSQL;
	XString		m_strCursorName;
	XString		m_strUpdateSQL;
	XString		m_strTableName;
	XString		m_strSeparator;

	//////////////////////////////////////////////////////////////////////////
	// For Fetch Data
	DWORD	m_dwRowsFetched;
};

class XFieldExchange {

public:
	enum RFX_Operation
	{
		exBindFieldToColumn,	// register users fields with ODBC SQLBindCol

		exSetFieldNull,			// Set status bit for null value
		
		exName,					// append field name
		exNameValue,			// append filed name = value
		exValue,				// append field value
	};

	XFieldExchange(int nOperation, XRecordset* pRs);

	void Default(LPCTSTR lpszName , void* pValue, long* pnLen , 
		int nType, int cbValue, int cbPrecision);

public:
	XRecordset*	m_pRs;
	int			m_nOperation;
	XString*	m_pstr;
	XString*	m_pstrSeparator;
	int			m_nParamFields;
	HSTMT		m_hStmt;
};

class XDBDateTime {
public:
	XDBDateTime();
	XDBDateTime(tm t);
	XDBDateTime(SQL_TIMESTAMP_STRUCT ts);
	XDBDateTime(int nYear, int nMonth, int nDay, int nHour = 0, int nMinute = 0, 
		int nSecond = 0, int nMicrosecond = 0);

	BOOL IsValidDateTime();

	XDBDateTime& operator=(const tm& t);

	operator SQL_TIMESTAMP_STRUCT&();
	operator tm() const;

	XDBDateTime& operator+(const XDBDateTime& t);
	XDBDateTime& operator-(const XDBDateTime& t);

	XDBDateTime& operator=(tm& tDest);
	XDBDateTime&  operator=(SQL_TIMESTAMP_STRUCT& tsDest);

	XString Format(LPCTSTR pszFormat);

	void Invalidate();

protected:
	void tm2SQL_TIMESTAMP(const tm& tSrc, SQL_TIMESTAMP_STRUCT& tsDest) const;
	void SQL_TIMESTAMP2tm(const SQL_TIMESTAMP_STRUCT& tsDest, tm& tSrc) const;

protected:
	SQL_TIMESTAMP_STRUCT	m_timestamp;
	BOOL					m_bValid;
};

// text data
void RFX_Text(XFieldExchange* pFX, LPCTSTR szName, XString& value,
	int nMaxLength = 255, int nColumnType = SQL_VARCHAR);

void RFX_Text(XFieldExchange* pFX, LPCTSTR szName, LPTSTR value,
	int nMaxLength, int nColumnType = SQL_VARCHAR);

// integer data
void RFX_Long(XFieldExchange* pFX, LPCTSTR szName, long& value);
void RFX_Int(XFieldExchange* pFX, LPCTSTR szName, int& value);
void RFX_Single(XFieldExchange* pFX, LPCTSTR szName, float& value);
void RFX_Double(XFieldExchange* pFX, LPCTSTR szName, double& value);

// date and time
void RFX_Date(XFieldExchange* pFX, LPCTSTR szName, struct tm& value);

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XODBC_H_
