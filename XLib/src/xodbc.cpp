#include <xodbc.h>
#include <time.h>
#include "./odbc.h"
#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif
//////////////////////////////////////////////////////////////////////////
// ODBC API helper functions
#ifdef _DEBUG
#define XODBC_TRACE(s, c)	DbgTrace(_T("%s(%d): %s (Code: %d | 0x%x)\n"), \
	__FILE__, __LINE__, (s), (c), (c))
void XODBC_DUMPERR(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   MsgLen;
	for (int i = 1;; i ++) {
		SQLRETURN rt = SQLGetDiagRec(HandleType, Handle, i, 
			SqlState, &NativeError, Msg, sizeof(Msg), &MsgLen);
		if (rt == SQL_NO_DATA || rt == SQL_ERROR)
			break;
	  DbgTrace(_T("%s, State:%s, (Code: %d | 0x%x)\n"), Msg, SqlState, 
		  NativeError, NativeError);
	  i++;
	}

}

#else
#define XODBC_DUMPERR(Handle, HandleType)
#define XODBC_TRACE(s, c)	
#endif
static const TCHAR LiteralSeparator = '\'';
//////////////////////////////////////////////////////////////////////////
BOOL XCheckRetCode(SQLSMALLINT HandleType, SQLHANDLE handle, SQLRETURN nRetCode)
{
	UNUSED(HandleType);
	UNUSED(handle);	
	switch (nRetCode)
	{
	case SQL_SUCCESS_WITH_INFO:
		// Fall through
	case SQL_SUCCESS:
	case SQL_NO_DATA_FOUND:
		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// class XDatabase
/* static */ SQLHENV XDatabase::s_hEnv =  SQL_NULL_HENV;
/* static */ long XDatabase::s_lRefCount = 0;
/* static */ SQLHENV XDatabase::GetEnv()
{
	if (s_hEnv == SQL_NULL_HENV) {
		_xassert(s_lRefCount == 0);
		RETCODE nRetCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &s_hEnv);
		if (s_hEnv == SQL_NULL_HENV)
			return SQL_NULL_HENV;
		nRetCode = SQLSetEnvAttr(s_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
		if (!XCheckRetCode(SQL_HANDLE_ENV, s_hEnv, nRetCode)) {
			SQLFreeHandle(SQL_HANDLE_ENV, s_hEnv);
			s_hEnv = SQL_NULL_HENV;
			return FALSE;
		}

	}

	s_lRefCount ++;
	return s_hEnv;
}

/* static */ void XDatabase::ReleaseEnv()
{
	if (s_lRefCount <= 0)
		return;
	_xassert(s_hEnv != SQL_NULL_HENV);
	SQLFreeHandle(SQL_HANDLE_ENV, s_hEnv);
	s_hEnv = SQL_NULL_HENV;
	s_lRefCount --;
}

XDatabase::XDatabase()
{
	m_hDbc					= SQL_NULL_HDBC;
	m_lConnectionTimeout	= 0;
	m_lLoginTimeout			= 0;
	m_bIsConnected			= FALSE;
	m_nRowsAffected			= 0;
}

XDatabase::~XDatabase()
{
	Close();
	m_lConnectionTimeout	= 0;
	m_lLoginTimeout			= 0;
	m_bIsConnected			= FALSE;
	m_nRowsAffected			= 0;
}

/* virtual */ BOOL XDatabase::CheckRetCode(SQLRETURN nRetCode) const
{
	switch (nRetCode)
	{
	case SQL_SUCCESS_WITH_INFO:
		// Fall through
	case SQL_SUCCESS:
	case SQL_NO_DATA_FOUND:
		return TRUE;
	}

	return FALSE;
}

SQLHDBC XDatabase::AllocDbc()
{
	HENV hEnv = GetEnv();
	if (hEnv == SQL_NULL_HENV)
		return SQL_NULL_HDBC;
	
	RETCODE rc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &m_hDbc); 
	if (!CheckRetCode(rc)) {
		ReleaseEnv();
		return SQL_NULL_HDBC;
	}

	return m_hDbc;
}

void XDatabase::FreeDbc()
{
	if (SQL_NULL_HDBC == m_hDbc)
		return;
	SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
	m_hDbc = SQL_NULL_HDBC;
	ReleaseEnv();
}

void XDatabase::Close()
{
	if (m_hDbc == SQL_NULL_HDBC) {
		_xassert(!m_bIsConnected);
		return;
	}

	if (!m_bIsConnected)
		SQLDisconnect(m_hDbc);
	FreeDbc();
	m_bIsConnected = FALSE;
}

BOOL XDatabase::Open(LPCTSTR szDSN,LPCTSTR szUser, LPCTSTR szPass)
{
	if (m_bIsConnected) {
		_xassert(m_hDbc != SQL_NULL_HDBC);
		SQLDisconnect(m_hDbc);
	}

	if (m_hDbc == SQL_NULL_HDBC) {
		if (AllocDbc() == SQL_NULL_HDBC) {
			XODBC_TRACE(_T("%s(%d): AllocDbc() failed\n"), -1);
			return FALSE;
		}

	}

	SQLRETURN nRetCode;
	if(m_lConnectionTimeout > 0) {
		SQLSetConnectAttr(m_hDbc, SQL_ATTR_CONNECTION_TIMEOUT, 
			(SQLPOINTER )m_lConnectionTimeout, 0);
	}

	
	if (m_lLoginTimeout > 0) {
		SQLSetConnectAttr(m_hDbc, SQL_ATTR_LOGIN_TIMEOUT, 
			(SQLPOINTER)m_lLoginTimeout, 0);
	}

	_SQL_SYNC(::SQLSetConnectOption(m_hDbc, SQL_ODBC_CURSORS, SQL_CUR_USE_ODBC));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Driver don't support cursor."), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_DBC, m_hDbc);
	}

	nRetCode = SQLConnect(m_hDbc, 
					(SQLCHAR*)szDSN, 
					_tcslen(szDSN), 
					(SQLCHAR*)szUser, 
					_tcslen(szUser), 
					(SQLCHAR*)szPass, 
					_tcslen(szPass));
	/*
	UCHAR szConnectOutput[MAX_CONNECT_LEN];
	SQLSMALLINT nLen;
	UWORD wConnectOption = SQL_DRIVER_COMPLETE;
	_SQL_SYNC(SQLDriverConnect(m_hDbc, NULL, (SQLCHAR*)szDSN, _tcslen(szDSN), 
		szConnectOutput, sizeof(szConnectOutput), &nLen, wConnectOption));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Connect failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_DBC, m_hDbc);
	}

	*/
	m_bIsConnected = nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
	m_chIDQuoteChar = ' ';
	if (m_bIsConnected) {
		char szIDQuoteChar[2];
		SWORD nResult;
		_SQL_SYNC(::SQLGetInfo(m_hDbc, SQL_IDENTIFIER_QUOTE_CHAR,
			szIDQuoteChar, _countof(szIDQuoteChar), &nResult));
		if (CheckRetCode(nRetCode) && nResult == 1)
			m_chIDQuoteChar = szIDQuoteChar[0];
	}

	return m_bIsConnected;
}

BOOL XDatabase::Browse(UCHAR* szConnStrIn, UCHAR* szConnStrOut)
{
	SQLRETURN nRetCode;
	SWORD swLenOut = 0;
	nRetCode = SQLBrowseConnect(m_hDbc, 
							(SQLCHAR*)szConnStrIn, 
							sizeof(szConnStrIn), 
							(SQLCHAR*)szConnStrOut, 
							MAX_BUFFER, 
							&swLenOut);
	m_bIsConnected = nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
	return m_bIsConnected;
}

void XDatabase::SetConnectionTimeout(LONG nSeconds)
{
	if(m_hDbc)
		SQLSetConnectAttr(m_hDbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)nSeconds, 0);
	m_lConnectionTimeout = nSeconds;
}

BOOL XDatabase::DriverConnect(LPCTSTR szConnStr, LPCTSTR szConnStrOut, HWND hWnd, enum drvCompletion drvConn)
{
	SQLRETURN nRetCode;
	SQLSMALLINT pcbConnStrOut;
	if(drvConn == sqlPrompt && hWnd == NULL)
		return FALSE;
	if(m_lConnectionTimeout > 0)
		SQLSetConnectAttr(m_hDbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)m_lConnectionTimeout, 0);
	
	SQLSetConnectAttr(m_hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)m_lLoginTimeout, 0);
	
	nRetCode = SQLDriverConnect(m_hDbc, 
							hWnd, 
							(SQLCHAR*)szConnStr, 
							SQL_NTS, 
							(SQLCHAR*)szConnStrOut,
							sizeof(szConnStrOut), 
							&pcbConnStrOut, 
							(SQLUSMALLINT)drvConn);
	
	m_bIsConnected = nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
	return m_bIsConnected;
}

void XDatabase::SetReadOnly(BOOL bReadOnly)
{
	SQLSetConnectAttr(m_hDbc, SQL_ATTR_ACCESS_MODE, (SQLPOINTER )
		(bReadOnly? SQL_MODE_READ_ONLY : SQL_MODE_READ_WRITE), 0);
}

LONG XDatabase::GetConnectionTimeout()
{
	LONG nSeconds;
	SQLGetConnectAttr(m_hDbc, SQL_ATTR_CONNECTION_TIMEOUT, &nSeconds, NULL, 0);
	return nSeconds;
}

BOOL XDatabase::Execute(LPCTSTR szSqlStr)
{
	SQLRETURN nRetCode;
	SQLHSTMT hStmt = NULL;
	SQLINTEGER nRowCount;
	nRetCode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
	if (hStmt == SQL_NULL_HANDLE) {
		XODBC_TRACE(_T("SQLAllocHandle(SQL_HANDLE_STMT) Failed"), nRetCode);
		return FALSE;
	}

	nRetCode = SQLExecDirect(hStmt, (SQLCHAR*)szSqlStr, SQL_NTS);
	if (!CheckRetCode(nRetCode)) {
		m_nRowsAffected = 0;
		XODBC_TRACE(_T("SQLExecDirect Faield"), nRetCode);
		return FALSE;
	}

	SQLRowCount(hStmt, &nRowCount);
	m_nRowsAffected = nRowCount;
	return TRUE;
}

//Replace brackets in SQL string with SQL_IDENTIFIER_QUOTE_CHAR
void XDatabase::ReplaceBrackets(LPTSTR lpchSQL)
{
	BOOL bInLiteral = FALSE;
	LPTSTR lpchNewSQL = lpchSQL;
	while (*lpchSQL != '\0')
	{
		if (*lpchSQL == '\'')
			{
				// Handle escaped literal
				if (*_tcsinc(lpchSQL) == '\'')
				{
					*lpchNewSQL = *lpchSQL;
					lpchSQL = _tcsinc(lpchSQL);
					lpchNewSQL = _tcsinc(lpchNewSQL);
				}

				else
					bInLiteral = !bInLiteral;
				*lpchNewSQL = *lpchSQL;
			}

		else if (!bInLiteral && (*lpchSQL == '['))
		{
			if (*_tcsinc(lpchSQL) == '[')
			{
				// Handle escaped left bracket by inserting one '['
				*lpchNewSQL = *lpchSQL;
				lpchSQL = _tcsinc(lpchSQL);
			}

			else
				*lpchNewSQL = m_chIDQuoteChar;
		}

		else if (!bInLiteral && (*lpchSQL == ']'))
		{
			if (*_tcsinc(lpchSQL) == ']')
			{
				// Handle escaped right bracket by inserting one ']'
				*lpchNewSQL = *lpchSQL;
				lpchSQL = _tcsinc(lpchSQL);
			}

			else
				*lpchNewSQL = m_chIDQuoteChar;
		}

		else
			*lpchNewSQL = *lpchSQL;
		lpchSQL = _tcsinc(lpchSQL);
		lpchNewSQL = _tcsinc(lpchNewSQL);
	}

}

/////////////////////////////////////////////////////////////
//
// XRecordset Class
//
XRecordset::XRecordset(XDatabase* pDb /* = NULL */)
{
	m_pDatabase	= pDb;
	m_hStmt		= SQL_NULL_HSTMT;
	m_bIsEOF	= FALSE;
	m_bIsBOF	= FALSE;
	m_bRecordsetDb	= FALSE;
	m_nEditMode		= noMode;
	m_nResultCols	= 0;
	m_nBoundCols	= 0;
	m_bUpdatable	= FALSE;
	m_strSeparator	= _T(",");
	m_hStmtUpdate	= NULL;
};
XRecordset::~XRecordset()
{
	Close();
	m_hStmt = NULL;
	m_bIsEOF = FALSE;
	m_bIsBOF = FALSE;
};
/* virtual */ BOOL XRecordset::CheckRetCode(RETCODE nRetCode) const
{
	switch (nRetCode)
	{
	case SQL_SUCCESS_WITH_INFO:
		// Fall through
	case SQL_SUCCESS:
	case SQL_NO_DATA_FOUND:
		return TRUE;
	}

	return FALSE;
}

HSTMT XRecordset::AllocStmt()
{
	SQLRETURN nRetCode;
	if (m_pDatabase == NULL) {
		m_pDatabase = new XDatabase();
		m_bRecordsetDb = TRUE;
	}

	if (!m_pDatabase->IsOpen()) {
		XString strDSN, strUser, strPwd;
		GetDefaultConnect(strDSN, strUser, strPwd);
		if (!m_pDatabase->Open(strDSN , strUser, strPwd)) {
			if (m_bRecordsetDb) {
				delete m_pDatabase;
				m_bRecordsetDb = FALSE;
			}

			m_pDatabase = NULL;
			return SQL_NULL_HSTMT;
		}

	}

	_SQL_SYNC(SQLAllocHandle(SQL_HANDLE_STMT, m_pDatabase->m_hDbc, &m_hStmt));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLAllocHandle failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_DBC, m_pDatabase->m_hDbc);
		return SQL_NULL_HSTMT;
	}

	//nRetCode = SQLSetStmtAttr(hstmtS, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_VALUES, 0); 
	//nRetCode = SQLSetStmtAttr(hstmtS, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_STATIC, 0); 
	_SQL_SYNC(SQLSetStmtAttr(m_hStmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_STATIC, 0));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLSetStmtOption() failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
		return SQL_NULL_HSTMT;
	}

	// Attempt to reset the concurrency model.
	_SQL_SYNC(SQLSetStmtAttr(m_hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_VALUES, 0));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLSetStmtOption() failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
		return SQL_NULL_HSTMT;
	}

	_SQL_SYNC(::SQLSetStmtOption(m_hStmt, SQL_ATTR_ROW_ARRAY_SIZE, 1));
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLSetStmtOption() failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
		return SQL_NULL_HSTMT;
	}

	return m_hStmt;
}

void XRecordset::FreeStmt()
{
	if(m_hStmt != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
	m_hStmt = NULL;
	if (m_bRecordsetDb) {
		delete m_pDatabase;
		m_pDatabase = NULL;
		m_bRecordsetDb = FALSE;
	}

}

void XRecordset::BindColums()
{
	XFieldExchange fx(XFieldExchange::exBindFieldToColumn, this);
	DoFieldExchange(&fx);
	m_nBoundCols = fx.m_nParamFields;
}

void XRecordset::AppendNames()
{
	XFieldExchange fx(XFieldExchange::exName, this);
	fx.m_pstr = &m_strUpdateSQL;
	fx.m_pstrSeparator = &m_strSeparator;
	DoFieldExchange(&fx);
}

void XRecordset::AppendValues()
{
	XFieldExchange fx(XFieldExchange::exValue, this);
	fx.m_pstr = &m_strUpdateSQL;
	fx.m_pstrSeparator = &m_strSeparator;
	fx.m_hStmt = m_hStmtUpdate;
	DoFieldExchange(&fx);
}

void XRecordset::AppendNamesValues()
{
	XFieldExchange fx(XFieldExchange::exNameValue, this);
	fx.m_pstr = &m_strUpdateSQL;
	fx.m_pstrSeparator = &m_strSeparator;
	fx.m_hStmt = m_hStmtUpdate;
	DoFieldExchange(&fx);
}

void XRecordset::SetFieldNull()
{
	XFieldExchange fx(XFieldExchange::exSetFieldNull, this);
	DoFieldExchange(&fx);
}

LPCTSTR FindSQLToken(LPCTSTR lpszSQL, LPCTSTR lpszSQLToken)
{
	BOOL bInLiteral;
	BOOL bInBrackets;
	int nLeftBrackets;
	int nRightBrackets;
	LPCTSTR lpch;
	LPCTSTR lpchSQLStart;
	LPCTSTR lpszFoundToken;
	int nTokenOffset = 0;
	XString strSQL = lpszSQL;
	strSQL.MakeUpper();
	lpszFoundToken = strSQL.GetBuffer(0);
	lpchSQLStart = lpszFoundToken;
	do
	{
		lpszFoundToken = _tcsstr(lpszFoundToken + nTokenOffset, lpszSQLToken);
		if (lpszFoundToken == NULL)
		{
			strSQL.ReleaseBuffer();
			return NULL;
		}

		bInLiteral = bInBrackets = FALSE;
		nLeftBrackets = nRightBrackets = 0;
		// Check if embedded in literal or brackets
		for (lpch = lpchSQLStart; lpch < lpszFoundToken; lpch = _tcsinc(lpch))
		{
			if (*lpch == LiteralSeparator)
			{
				// Skip if escape literal
				if (*_tcsinc(lpch) == LiteralSeparator)
					lpch = _tcsinc(lpch);
				else
					bInLiteral = !bInLiteral;
			}

			else if (!bInLiteral && (*lpch == '['))
			{
				// Skip if escape left bracket
				if (*_tcsinc(lpch) == '[')
					lpch = _tcsinc(lpch);
				else
				{
					nLeftBrackets++;
					if ((nLeftBrackets - nRightBrackets) > 0)
						bInBrackets = TRUE;
					else
						bInBrackets = FALSE;
				}

			}

			else if (!bInLiteral && (*lpch == ']'))
			{
				// Skip if escape right bracket
				if (*_tcsinc(lpch) == ']')
					lpch = _tcsinc(lpch);
				else
				{
					nRightBrackets++;
					if ((nLeftBrackets - nRightBrackets) > 0)
						bInBrackets = TRUE;
					else
						bInBrackets = FALSE;
				}

			}

		}

		// If first iteration, reset the offset to jump over found token
		if (nTokenOffset == 0)
			nTokenOffset = lstrlen(lpszSQLToken);
	} while (bInLiteral || bInBrackets);
	lpszFoundToken = lpszSQL + (lpszFoundToken - lpchSQLStart);
	strSQL.ReleaseBuffer();
	return lpszFoundToken;
}

BOOL PASCAL IsJoin(LPCTSTR lpszJoinClause)
{
	// Look for comma in join clause
	if (FindSQLToken(lpszJoinClause, _T(",")) != NULL)
		return TRUE;
	// Look for outer join clause
	if (FindSQLToken(lpszJoinClause, _T(" JOIN ")) != NULL)
		return TRUE;
	return FALSE;
}

void XRecordset::AnalyseSelectSQL()
{
	LPCTSTR lpchTokenFrom;
	LPCTSTR lpchToken;
	LPCTSTR lpchTokenNext;
	LPTSTR lpszSQLStart;
	XString strSQL = m_strSQL;
	m_bUpdatable = FALSE;
	lpchTokenFrom = FindSQLToken(strSQL, _T(" FROM "));
	if (lpchTokenFrom == NULL)
	{
		XODBC_TRACE(_T("Warning: Missing ' FROM ', recordset not updatable"), -1);
		return;
	}

	lpchToken = FindSQLToken(strSQL, _T(" GROUP BY "));
	if (lpchToken != NULL)
	{
		XODBC_TRACE(_T("Warning: SQL contains ' GROUP BY ', recordset not updatable"), -1);
		return;
	}

	lpchToken = FindSQLToken(strSQL, _T(" UNION "));
	if (lpchToken != NULL)
	{
		XODBC_TRACE(_T("Warning: SQL contains ' UNION ', recordset not updatable"), -1);
		return;
	}

	// Find next token after FROM (can't have HAVING clause without GROUP BY)
	lpchToken = FindSQLToken(strSQL, _T(" WHERE "));
	lpchTokenNext = FindSQLToken(strSQL, _T("ORDER BY "));
	lpszSQLStart = strSQL.GetBuffer(0);
	if (lpchTokenNext == NULL)
		lpchTokenNext = lpchToken;
	else if (lpchToken != NULL && lpchToken < lpchTokenNext)
		lpchTokenNext = lpchToken;
	if (lpchTokenNext != NULL)
	{
		int nFromLength = lpchTokenNext - lpchTokenFrom;
		memcpy(lpszSQLStart, lpchTokenFrom, nFromLength*sizeof(TCHAR));
		lpszSQLStart[nFromLength] = '\0';
	}

	else
		lstrcpy(lpszSQLStart, lpchTokenFrom);
	strSQL.ReleaseBuffer();
	if (IsJoin(strSQL))
	{
		XODBC_TRACE(_T("Warning: SQL contains join, recordset not updatable"), -1);
		return;
	}

	// Cache table name (skip over " FROM ")
	m_strTableName = strSQL.Right(strSQL.GetLength()-6);
	m_bUpdatable = TRUE;
}

BOOL XRecordset::Open(LPCTSTR szSqlStr /* = NULL */)
{
	SQLRETURN nRetCode;
	if (AllocStmt() == SQL_NULL_HSTMT)
		return FALSE;
	if (szSqlStr == NULL) {
		m_strSQL = GetDefaultSQL();		
	} else 
		m_strSQL = szSqlStr;
	_SQL_SYNC(SQLPrepare(m_hStmt, (SQLCHAR* )(LPCTSTR )m_strSQL, SQL_NTS));
	
	if(!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLPrepare failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
		return FALSE;
	}

	_SQL_SYNC(SQLExecute(m_hStmt));
	if(!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLExecute failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
		return FALSE;
	}

	//nRetCode = SQLExecDirect(m_hStmt, (SQLCHAR* )(LPCTSTR )m_strSQL, SQL_NTS);
	try {
		m_nResultCols = GetFieldCount();
		AnalyseSelectSQL();
		BindColums();
		//MoveNext();
		nRetCode = FetchData(1, SQL_FETCH_NEXT);
		if (nRetCode == SQL_NO_DATA) {
			m_bIsBOF = m_bIsEOF = TRUE;
		} else if (!CheckRetCode(nRetCode)) {
			Close();
			return FALSE;
		}

	}

	catch(XDBException& ) {
		Close();
		return FALSE;	
	}

	return TRUE;
}

void XRecordset::Close()
{
	FreeStmt();
	m_bIsEOF	= FALSE;
	m_bIsBOF	= FALSE;
	m_bRecordsetDb	= FALSE;
	m_nEditMode		= noMode;
	m_nResultCols	= 0;
	m_nBoundCols	= 0;
	m_bUpdatable	= FALSE;
	m_bIsBOF = m_bIsEOF = FALSE;
	m_strSQL.Empty();
	m_strTableName.Empty();
}

BOOL XRecordset::GetFieldValue(int nField, LPTSTR szData)
{
	SQLRETURN nRetCode;
	SQLINTEGER cbValue;
	int nLength = GetFieldLength(nField) + 1;
	
	nRetCode = SQLGetData(m_hStmt, (SQLUSMALLINT )nField + 1, 
		SQL_C_CHAR, szData, nLength, &cbValue) == SQL_SUCCESS;
	return nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
}

BOOL XRecordset::GetFieldValue(LPCTSTR szFieldName, LPTSTR szData)
{
	return GetFieldValue(GetFieldIndex(szFieldName), szData);	
}

BOOL XRecordset::GetFieldValue(int nField, LONG& lData)
{
	SQLRETURN nRetCode;
	SQLINTEGER cbValue;
	int nLength = GetFieldLength(nField) + 1;
	
	nRetCode = SQLGetData(m_hStmt, (SQLUSMALLINT)nField + 1, SQL_C_LONG, 
		&lData, nLength, &cbValue) == SQL_SUCCESS;
	return nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
}

BOOL XRecordset::GetFieldValue(LPCTSTR szFieldName, LONG& lData)
{
	return GetFieldValue(GetFieldIndex(szFieldName), lData);	
}

BOOL XRecordset::GetFieldValue(int nField, DOUBLE& dblData)
{
	SQLINTEGER cbValue;
	SQLRETURN nRetCode;
	int nLength = GetFieldLength(nField) + 1;
	
	nRetCode = SQLGetData(m_hStmt, (SQLUSMALLINT)nField + 1, SQL_C_DOUBLE, 
		&dblData, nLength, &cbValue) == SQL_SUCCESS;
	return nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;
}

BOOL XRecordset::GetFieldValue(LPCTSTR szFieldName, DOUBLE& dblData)
{
	return GetFieldValue(GetFieldIndex(szFieldName), dblData);	
}

	BOOL GetFieldValue(int nField, XDBDateTime& dt);
	BOOL GetFieldValue(LPCTSTR szFieldName, XDBDateTime& dt);
BOOL XRecordset::GetFieldValue(int nField, XDBDateTime& dt)
{
	SQLINTEGER cbValue;
	SQLRETURN nRetCode;
	int nLength = GetFieldLength(nField) + 1;
	SQL_TIMESTAMP_STRUCT sqltm;
	
	nRetCode = SQLGetData(m_hStmt, (SQLUSMALLINT)nField + 1, SQL_C_TYPE_TIMESTAMP, 
		&sqltm, nLength, &cbValue) == SQL_SUCCESS;
	if(nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO) {	
		dt = sqltm;
		return TRUE;
	}

	return FALSE;
}

BOOL XRecordset::GetFieldValue(LPCTSTR szFieldName, XDBDateTime& dt)
{
	return GetFieldValue(GetFieldIndex(szFieldName), dt);	
}

BOOL XRecordset::GetFieldValue(int nField, XString& strData)
{
	TCHAR szBuff[1028];
	if (!GetFieldValue(nField, szBuff))
		return FALSE;
	strData = szBuff;
	return TRUE;
}

BOOL XRecordset::GetFieldValue(LPCTSTR szFieldName, XString& strData)
{
	TCHAR szBuff[1028];
	if (!GetFieldValue(szFieldName, szBuff))
		return FALSE;
	strData = szBuff;
	return TRUE;
}

BOOL XRecordset::GetFieldName(int nField, LPTSTR szFieldName)
{
	int nType, nLength;
	return GetFieldAttributes(nField, szFieldName, nType, nLength);
}

int XRecordset::GetFieldIndex(LPCTSTR szFieldName)
{
	SQLSMALLINT nCols;
	int nCol = 1;
	CHAR szColName[MAX_COL_NAME_LEN];
	SQLSMALLINT cbColNameLen, fSqlType, ibScale, fNullable;
	SQLUINTEGER cbColDef;
	SQLNumResultCols(m_hStmt, &nCols);
	while(nCol < nCols)
	{
		memset(szColName, 0, 32 * sizeof(CHAR));
		SQLDescribeCol(m_hStmt, nCol, (SQLCHAR*)szColName, MAX_COL_NAME_LEN, 
			&cbColNameLen, &fSqlType, &cbColDef, &ibScale, &fNullable);
		if(_stricmp(szColName, szFieldName) == 0)
			return nCol - 1;
		nCol++;
	}

	return -1;
}

RETCODE XRecordset::FetchData(SDWORD nRow, UWORD wFetchType)
{
	RETCODE nRetCode;
	WORD wStatus;
	_ODBC_CALL(::SQLExtendedFetch(m_hStmt, wFetchType,
		nRow, &m_dwRowsFetched, &wStatus));
	if (!CheckRetCode(nRetCode)) {
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmt);
	}

	return nRetCode;
}

void XRecordset::Move(long nRows, WORD wFetchType /* = SQL_FETCH_RELATIVE */)
{
	RETCODE nRetCode;
	nRetCode = FetchData(nRows, wFetchType);
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Fetch Data Faild"), nRetCode);
		ThrowDBException(nRetCode);
	}

}

void XRecordset::MoveFirst()
{
	RETCODE nRetCode = FetchData(1, SQL_FETCH_FIRST);
	if (nRetCode == SQL_NO_DATA) {
		m_bIsBOF = m_bIsEOF = TRUE;
	} else if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Fetch Data Faild"), nRetCode);
		ThrowDBException(nRetCode);
	}

}

void XRecordset::MoveNext()
{
	SQLRETURN nRetCode = FetchData(1, SQL_FETCH_NEXT);
	if (nRetCode == SQL_NO_DATA) {
		m_bIsEOF =TRUE;
	} else if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Fetch Data Faild"), nRetCode);
		ThrowDBException(nRetCode);
	}

}

void XRecordset::MovePrevious()
{
	RETCODE nRetCode = FetchData(-1, SQL_FETCH_PRIOR);
	if (nRetCode == SQL_NO_DATA) {
		m_bIsBOF =TRUE;
	} else if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Fetch Data Faild"), nRetCode);
		ThrowDBException(nRetCode);
	}

	
}

void XRecordset::MoveLast()
{
	RETCODE nRetCode = FetchData(-1, SQL_FETCH_LAST);
	if (nRetCode == SQL_NO_DATA) {
		m_bIsBOF = m_bIsEOF = TRUE;
	} else if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("Fetch Data Faild"), nRetCode);
		ThrowDBException(nRetCode);
	}

}

LONG XRecordset::GetFieldLength(int nField)
{
	SQLSMALLINT fSqlType, ibScale, fNullable;
	SQLUINTEGER cbColDef;
	
	SQLDescribeCol(m_hStmt, nField + 1, NULL, 0, 0, &fSqlType, &cbColDef, 
		&ibScale, &fNullable);
	return cbColDef;	
}

LONG XRecordset::GetRecordCount()
{
	LONG lRowCount;
	::SQLRowCount(m_hStmt, &lRowCount);
	return lRowCount;
}

BOOL XRecordset::GetFieldAttributes(int nField, LPCTSTR szFieldName, 
									int& nType, int& nLength)
{
	SQLRETURN nRetCode;
	SQLSMALLINT cbColNameLen, fSqlType, ibScale, fNullable;
	SQLUINTEGER cbColDef;
	
	nRetCode = SQLDescribeCol(m_hStmt, nField + 1, (SQLCHAR*)szFieldName, 
		MAX_COL_NAME_LEN, &cbColNameLen, &fSqlType, &cbColDef,
		&ibScale, &fNullable);
	
	nType = fSqlType;
	nLength = cbColDef;
	return nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO;	
}

int XRecordset::GetFieldCount()
{
	SQLSMALLINT nFieldCount = 0;
	SQLNumResultCols(m_hStmt, &nFieldCount);
	return nFieldCount;
}

void XRecordset::BuildUpdateSQL()
{
	switch(m_nEditMode) {
	case editMode:
		BuildEdit();
		break;
	case addnewMode:
		BuildAddNew();
		break;
	case deleteMode:
		BuildDelete();
		break;
	default:
		_xassert(FALSE);
		XODBC_TRACE(_T("Unknown edit mode"), -1);
	}

	m_pDatabase->ReplaceBrackets(m_strUpdateSQL.GetBuffer(0));
	m_strUpdateSQL.ReleaseBuffer();
}

void XRecordset::GetCursorName()
{
	//if (!m_strCursorName.IsEmpty())
	//	return;
	RETCODE nRetCode;
	UCHAR szCursorName[MAX_CURSOR_NAME+1];
	SWORD nLength = _countof(szCursorName)-1;
	_SQL_SYNC(::SQLGetCursorName(m_hStmt,
		szCursorName, _countof(szCursorName), &nLength));
	if (!CheckRetCode(nRetCode))
		ThrowDBException(nRetCode);
	m_strCursorName = (char*)szCursorName;
}

void XRecordset::BuildEdit()
{
	m_strUpdateSQL = _T("UPDATE ");
	m_strUpdateSQL += m_strTableName;
	m_strUpdateSQL += _T(" SET ");
	AppendNamesValues();
	// overwrite last ',' with ' '
	_xassert(m_strUpdateSQL[m_strUpdateSQL.GetLength()-1] == ',');
	m_strUpdateSQL.SetAt(m_strUpdateSQL.GetLength()-1, ' ');
	XString strWhere;
	GetCursorName();
	strWhere.Format(_T(" WHERE CURRENT OF %s"), (LPCTSTR )m_strCursorName);
	m_strUpdateSQL += strWhere;
}

void XRecordset::BuildAddNew()
{
	m_strUpdateSQL.Format(_T("INSERT INTO %s ("), (LPCTSTR )m_strTableName);
	AppendNames();
	// overwrite last ',' with ' '
	_xassert(m_strUpdateSQL[m_strUpdateSQL.GetLength()-1] == ',');
	m_strUpdateSQL.SetAt(m_strUpdateSQL.GetLength()-1, ')');
	m_strUpdateSQL += _T(" VALUES (");
	AppendValues();
	_xassert(m_strUpdateSQL[m_strUpdateSQL.GetLength()-1] == ',');
	m_strUpdateSQL.SetAt(m_strUpdateSQL.GetLength()-1, ')');
}

void XRecordset::BuildDelete()
{
	m_strUpdateSQL.Format(_T("DELETE FROM %s"), (LPCTSTR )m_strTableName);
	GetCursorName();
	XString strWhere;
	strWhere.Format(_T(" WHERE CURRENT OF %s"), (LPCTSTR )m_strCursorName);
	m_strUpdateSQL += strWhere;
}

void XRecordset::PrepareUpdateHstmt()
{
	RETCODE nRetCode;
	if (m_hStmtUpdate == SQL_NULL_HSTMT)
	{
		_SQL_SYNC(::SQLAllocStmt(m_pDatabase->m_hDbc, &m_hStmtUpdate));
		if (!CheckRetCode(nRetCode))
		{
			XODBC_TRACE(_T("Error: failure to allocate update statement."), nRetCode);
			ThrowDBException(nRetCode);
		}

	}

	else
	{
		_SQL_SYNC(::SQLFreeStmt(m_hStmtUpdate, SQL_CLOSE));
		if (!CheckRetCode(nRetCode))
			goto LErrRetCode;
		// Re-use (prepared) hstmt & param binding if optimizeBulkAdd option
		_SQL_SYNC(::SQLFreeStmt(m_hStmtUpdate, SQL_RESET_PARAMS));
		if (!CheckRetCode(nRetCode))
		{
LErrRetCode:
			// Bad hstmt, free it and allocate another one
			_SQL_SYNC(::SQLFreeStmt(m_hStmtUpdate, SQL_DROP));
			m_hStmtUpdate = SQL_NULL_HSTMT;
			_SQL_SYNC(::SQLAllocStmt(m_pDatabase->m_hDbc, &m_hStmtUpdate));
			if (!CheckRetCode(nRetCode))
			{
				XODBC_TRACE(_T("Error: failure to allocate update statement."), nRetCode);
				ThrowDBException(nRetCode);
			}

		}

	}

}

/* virtual */ void XRecordset::Edit()
{
	if (!m_bUpdatable) {
		XODBC_TRACE(_T("m_bUpdatable == FALSE"), -1);
		ThrowDBException(-1);
	}

	m_nEditMode = editMode;
}

/* virtual */ void XRecordset::AddNew()
{
	if (!m_bUpdatable) {
		XODBC_TRACE(_T("m_bUpdatable == FALSE"), -1);
		ThrowDBException(-1);
	}

	m_nEditMode = addnewMode;
	SetFieldNull();
}

/* virtual */ void XRecordset::Delete()
{
	if (!m_bUpdatable) {
		XODBC_TRACE(_T("m_bUpdatable == FALSE"), -1);
		ThrowDBException(-1);
	}

	m_nEditMode = deleteMode;
	Update();
}

/* virtual */ BOOL XRecordset::Update()
{
	if (m_nEditMode == noMode)
		return FALSE;
	PrepareUpdateHstmt();
	BuildUpdateSQL();
	ExecuteUpdateSQL();
	m_nEditMode = noMode;
	
	return TRUE;
}

void XRecordset::ExecuteUpdateSQL()
{
	RETCODE nRetCode;
	_ODBC_CALL(::SQLExecDirect(m_hStmtUpdate, 
		(UCHAR* )((LPCTSTR)m_strUpdateSQL), SQL_NTS));
	
	if (!CheckRetCode(nRetCode)) {
		XODBC_TRACE(_T("SQLExecDirect() failed"), nRetCode);
		XODBC_DUMPERR(SQL_HANDLE_STMT, m_hStmtUpdate);
		ThrowDBException(nRetCode);
	}

	
	SDWORD lRowsAffected = 0;
	_SQL_SYNC(::SQLRowCount(m_hStmtUpdate, &lRowsAffected));
	if (!CheckRetCode(nRetCode) || lRowsAffected == -1)
	{
		// Assume 1 row affected if # rows affected can't be determined
		lRowsAffected = 1;
	}

	else
	{
		if (lRowsAffected != 1)
		{
#ifdef _DEBUG
			XODBC_TRACE(_T("Warning: n rows affected by update operation (expected 1)."), nRetCode);
#endif
			ThrowDBException(-1);
		}

	}

	m_strUpdateSQL.Empty();
}

//////////////////////////////////////////////////////////////////////////
XFieldExchange::XFieldExchange(int nOperation, XRecordset* pRs)
{
	_xassert(pRs);
	m_nOperation	= nOperation;
	m_pRs			= pRs;
	m_pstr			= NULL;
	m_pstrSeparator	= NULL;
	m_nParamFields	= 0;
	m_hStmt			= SQL_NULL_HSTMT;
}

void XFieldExchange::Default(LPCTSTR lpszName , void* pValue, long* pnLen , 
							 int nCType, int cbValue, int cbPrecision)
{
	SQLRETURN nRetCode;
	LONG nLen;
	switch (m_nOperation) {
	case exBindFieldToColumn:
		m_nParamFields ++;
		_SQL_SYNC(::SQLBindCol(m_pRs->m_hStmt, (UWORD )m_nParamFields, 
			nCType,	pValue, cbPrecision, &nLen));
		if (!m_pRs->CheckRetCode(nRetCode)) {
			XODBC_TRACE(_T("bind field failed"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT , m_pRs->m_hStmt);
			m_pRs->ThrowDBException(nRetCode);
		}

		
		break;
	case exName:
		*m_pstr += lpszName;
		*m_pstr += *m_pstrSeparator;
		break;
	case exSetFieldNull:
		memset(pValue, 0, cbValue);
		break;
	default:
		_xassert(FALSE);
	}

}

//////////////////////////////////////////////////////////////////////////
// class XDBDateTime
XDBDateTime::XDBDateTime()
{
	memset(&m_timestamp, 0 , sizeof(m_timestamp));
	m_bValid = FALSE;
}

XDBDateTime::XDBDateTime(tm t)
{
	tm2SQL_TIMESTAMP(t, m_timestamp);
	m_bValid = TRUE;
}

XDBDateTime::XDBDateTime(SQL_TIMESTAMP_STRUCT ts)
{
	m_timestamp = ts;
	m_bValid = TRUE;
}

XDBDateTime::XDBDateTime(int nYear, int nMonth, int nDay, 
						 int nHour /* = 0 */, int nMinute /* = 0 */, 
						 int nSecond /* = 0 */, int nMicrosecond /* = 0 */)
{
	m_timestamp.year = nYear - 1900;
	m_timestamp.month = nMonth;
	m_timestamp.day = nDay;
	m_timestamp.hour = nHour;
	m_timestamp.minute = nMinute;
	m_timestamp.second = nSecond;
	m_timestamp.fraction = nMicrosecond;
	m_bValid = TRUE;
}

BOOL XDBDateTime::IsValidDateTime()
{
	return m_bValid;
}

XDBDateTime::operator SQL_TIMESTAMP_STRUCT&()
{
	return m_timestamp;
}

XDBDateTime::operator tm() const
{
	tm t;
	SQL_TIMESTAMP2tm(m_timestamp, t);
	return t;
}

XDBDateTime& XDBDateTime::operator+(const XDBDateTime& t)
{
	m_timestamp.year += t.m_timestamp.year;
	m_timestamp.month += t.m_timestamp.month;
	m_timestamp.day += t.m_timestamp.day;
	m_timestamp.hour += t.m_timestamp.hour;
	m_timestamp.minute += t.m_timestamp.minute;
	m_timestamp.second += t.m_timestamp.second;
	m_timestamp.fraction += m_timestamp.fraction;
	return *this;
}

XDBDateTime& XDBDateTime::operator-(const XDBDateTime& t)
{
	m_timestamp.year -= t.m_timestamp.year;
	m_timestamp.month -= t.m_timestamp.month;
	m_timestamp.day -= t.m_timestamp.day;
	m_timestamp.hour -= t.m_timestamp.hour;
	m_timestamp.minute -= t.m_timestamp.minute;
	m_timestamp.second -= t.m_timestamp.second;
	m_timestamp.fraction -= m_timestamp.fraction;
	return *this;
}

void XDBDateTime::tm2SQL_TIMESTAMP(const tm& tSrc, SQL_TIMESTAMP_STRUCT& tsDest) const
{
	tsDest.year = tSrc.tm_year;
	tsDest.month = tSrc.tm_mon + 1;
	tsDest.day = tSrc.tm_mday;
	tsDest.hour = tSrc.tm_hour;
	tsDest.minute = tSrc.tm_min;
	tsDest.second = tSrc.tm_sec;
	tsDest.fraction = 0;
}

void XDBDateTime::SQL_TIMESTAMP2tm(const SQL_TIMESTAMP_STRUCT& tsSrc, tm& tDest) const
{
	tDest.tm_year = tsSrc.year;
	tDest.tm_mon = tsSrc.month - 1; // January must be = 0		
	tDest.tm_mday = tsSrc.day;
	tDest.tm_hour = tsSrc.hour;
	tDest.tm_min = tsSrc.minute;
	tDest.tm_sec = tsSrc.second;
}

XDBDateTime& XDBDateTime::operator=(const tm& t)
{
	tm2SQL_TIMESTAMP(t, m_timestamp);
	return *this;
}

XDBDateTime& XDBDateTime::operator=(SQL_TIMESTAMP_STRUCT& tsDest)
{
	m_timestamp = tsDest;
	return *this;
}

XString XDBDateTime::Format(LPCTSTR pszFormat)
{
	_xassert(pszFormat != NULL);
	TCHAR szBuf[1024];
	tm t;
	SQL_TIMESTAMP2tm(m_timestamp, t);
	_tcsftime(szBuf, 1023, pszFormat, &t);
	
	return XString(szBuf);
}

void XDBDateTime::Invalidate()
{
	m_bValid = FALSE;
}

//////////////////////////////////////////////////////////////////////////
// RFX Functions
// text data
void RFX_Text(XFieldExchange* pFX, LPCTSTR szName, XString& value,
	int nMaxLength /* = 255*/, int nColumnType /* = SQL_VARCHAR*/ )
{
	SQLRETURN nRetCode;
	long nLen;
	XRecordset* pRs =pFX->m_pRs;
	XString strValue;
	switch(pFX->m_nOperation) {
		
	case XFieldExchange::exBindFieldToColumn:
		pFX->m_nParamFields ++;
		value.ReleaseBufferSetLength(nMaxLength);
		_SQL_SYNC(::SQLBindCol(pRs->m_hStmt, (UWORD )pFX->m_nParamFields, 
			SQL_C_CHAR,	value.GetBuffer(0), nMaxLength, &nLen));
		if (!pRs->CheckRetCode(nRetCode)) {
			XODBC_TRACE(_T("bind field failed"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT , pRs->m_hStmt);
			pRs->ThrowDBException(nRetCode);
		}

		break;
	/*
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		strValue.Format(_T("\'%s\'"), (LPCTSTR )value);
		*pFX->m_pstr += strValue;
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		break;
	*/
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		*pFX->m_pstr += _T('?');
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		pFX->m_nParamFields ++;
		_SQL_SYNC(::SQLBindParameter(pFX->m_hStmt,
			(UWORD)pFX->m_nParamFields, SQL_PARAM_INPUT,
			SQL_C_CHAR, SQL_VARCHAR, nMaxLength, 0, value.GetBuffer(0), 0, &nLen));
		if (!XCheckRetCode(SQL_HANDLE_STMT, pFX->m_hStmt, nRetCode)) {
			XODBC_TRACE(_T("Can't bind parameter"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT, pFX->m_pRs->m_hStmt);
			pFX->m_pRs->ThrowDBException(nRetCode);
		}

		break;
	
	case XFieldExchange::exSetFieldNull:
		value.Empty();
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_CHAR, value.GetLength(), nMaxLength);
	}

}

void RFX_Text(XFieldExchange* pFX, LPCTSTR szName, LPTSTR value,
	int nMaxLength, int nColumnType /* = SQL_VARCHAR*/)
{
	SQLRETURN nRetCode;
	XRecordset* pRs =pFX->m_pRs;
	long nLen;
	XString strValue;
	switch(pFX->m_nOperation) {
		case XFieldExchange::exBindFieldToColumn:
		pFX->m_nParamFields ++;
		_SQL_SYNC(::SQLBindCol(pRs->m_hStmt, (UWORD )pFX->m_nParamFields, 
			SQL_C_CHAR,	value, nMaxLength, &nLen));
		if (!pRs->CheckRetCode(nRetCode)) {
			XODBC_TRACE(_T("bind field failed"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT , pRs->m_hStmt);
			pRs->ThrowDBException(nRetCode);
		}

		
		break;
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		*pFX->m_pstr += _T('?');
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		pFX->m_nParamFields ++;
		_SQL_SYNC(::SQLBindParameter(pFX->m_hStmt,
			(UWORD)pFX->m_nParamFields, SQL_PARAM_INPUT,
			SQL_C_CHAR, SQL_VARCHAR, nMaxLength, 0, value, 0, &nLen));
		if (!XCheckRetCode(SQL_HANDLE_STMT, pFX->m_hStmt, nRetCode)) {
			XODBC_TRACE(_T("Can't bind parameter"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT, pFX->m_pRs->m_hStmt);
			pFX->m_pRs->ThrowDBException(nRetCode);
		}

		break;
		/*
		strValue.Format(_T("\'%s\'"), value);
		*pFX->m_pstr += strValue;
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		break;
		*/
		
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_CHAR, _tcslen(value), sizeof(value));
	}

}

// integer data
void RFX_Long(XFieldExchange* pFX, LPCTSTR szName, long& value)
{
	long nLen;
	XString strValue;
	switch(pFX->m_nOperation) {
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		strValue.Format(_T("%d"), value);
		*pFX->m_pstr += strValue;
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		break;
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_LONG, sizeof(value), sizeof(value));
	}

}

void RFX_Int(XFieldExchange* pFX, LPCTSTR szName, int& value)
{
	long nLen;
	XString strValue;
	switch(pFX->m_nOperation) {
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		strValue.Format(_T("%d"), value);
		*pFX->m_pstr += strValue;
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		break;
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_LONG, sizeof(value), sizeof(value));
	}

}

// float data
void RFX_Double(XFieldExchange* pFX, LPCTSTR szName, double& value)
{
	long nLen;
	XString strValue;
	switch(pFX->m_nOperation) {
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		
		strValue.Format(_T("%lf"), value);
		*pFX->m_pstr += strValue;
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		break;
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_DOUBLE, sizeof(value), sizeof(value));
	}

}

// date and time
void RFX_Date(XFieldExchange* pFX, LPCTSTR szName, XDBDateTime& value)
{
	SQLRETURN nRetCode;
	long nLen;
	XString strValue;
	switch(pFX->m_nOperation) {
	case XFieldExchange::exBindFieldToColumn:
		pFX->m_nParamFields ++;
		_SQL_SYNC(::SQLBindCol(pFX->m_pRs->m_hStmt, (UWORD )pFX->m_nParamFields, 
			SQL_C_TYPE_TIMESTAMP,	&(SQL_TIMESTAMP_STRUCT )value, 
			sizeof(SQL_TIMESTAMP_STRUCT), &nLen));
		if (!pFX->m_pRs->CheckRetCode(nRetCode)) {
			XODBC_TRACE(_T("bind field failed"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT , pFX->m_pRs->m_hStmt);
			pFX->m_pRs->ThrowDBException(nRetCode);
		}

		
		break;
	case XFieldExchange::exNameValue:
		*pFX->m_pstr += szName;
		*pFX->m_pstr += _T("=");
	case XFieldExchange::exValue:
		*pFX->m_pstr += _T("?");
		*pFX->m_pstr += *pFX->m_pstrSeparator;
		_SQL_SYNC(::SQLBindParameter(pFX->m_hStmt,
			(UWORD)pFX->m_nParamFields, SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
			sizeof(SQL_TIMESTAMP_STRUCT), 0, 
			&(SQL_TIMESTAMP_STRUCT)value, 
			sizeof(SQL_TIMESTAMP_STRUCT), &nLen));
		if (!XCheckRetCode(SQL_HANDLE_STMT, pFX->m_hStmt, nRetCode)) {
			XODBC_TRACE(_T("Can't bind parameter"), nRetCode);
			XODBC_DUMPERR(SQL_HANDLE_STMT, pFX->m_pRs->m_hStmt);
			pFX->m_pRs->ThrowDBException(nRetCode);
		}

		pFX->m_nParamFields ++;
		break;
	case XFieldExchange::exSetFieldNull:
		value.Invalidate();
		break;
	default:
		pFX->Default(szName, &value, &nLen , SQL_C_TYPE_TIMESTAMP, sizeof(SQL_TIMESTAMP_STRUCT), sizeof(value));
	}

}

#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif
