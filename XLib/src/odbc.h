#ifndef _ODBC_H_
#define _ODBC_H_

#define _SQL_SYNC(SQLFunc) \
	do \
	{ \
		nRetCode = SQLFunc; \
	} while (0)


#define _ODBC_CALL(SQLFunc) \
	do \
	{ \
	} while ((nRetCode = (SQLFunc)) == SQL_STILL_EXECUTING)

#endif // #ifndef _ODBC_H_
